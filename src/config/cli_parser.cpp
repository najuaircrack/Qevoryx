#include "config/cli_parser.hpp"
#include "common/platform.hpp"
#include "common/constants.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <charconv>
#include <optional>
#include <string_view>
#include <limits>
#include <system_error>

namespace config {

bool CliParser::has_tui_flag(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--tui") == 0) return true;
    }
    return false;
}

namespace {

std::optional<long long> parse_integer(const char* text) {
    if (text == nullptr || *text == '\0') {
        return std::nullopt;
    }

    long long value = 0;
    const char* end = text + std::strlen(text);
    const auto result = std::from_chars(text, end, value);
    if (result.ec != std::errc{} || result.ptr != end) {
        return std::nullopt;
    }
    return value;
}

bool is_valid_ipv4(const std::string& address) {
    struct in_addr addr {};
    return inet_pton(AF_INET, address.c_str(), &addr) == 1;
}

} // namespace

Config CliParser::parse(int argc, char** argv) {
    Config cfg;

    if (argc < 3) {
        print_usage(argv[0]);
        std::exit(1);
    }

    int positional = 0;
    std::string positional_values[5];
    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (arg[0] == '-' && arg[1] == '-' && arg[2] != '\0') {
            continue;
        }

        if (positional < 5) {
            positional_values[positional] = arg;
        }
        ++positional;
    }

    if (positional < 2) {
        print_usage(argv[0]);
        std::exit(1);
    }

    cfg.target_ip = positional_values[0];
    if (!is_valid_ipv4(cfg.target_ip)) {
        std::cerr << "  Invalid target IP: " << cfg.target_ip << std::endl;
        print_usage(argv[0]);
        std::exit(1);
    }

    const auto port = parse_integer(positional_values[1].c_str());
    if (!port || *port < 1 || *port > 65535) {
        std::cerr << "  Invalid port: " << positional_values[1] << std::endl;
        print_usage(argv[0]);
        std::exit(1);
    }
    cfg.target_port = static_cast<std::uint16_t>(*port);

    if (positional >= 3) {
        const auto workers = parse_integer(positional_values[2].c_str());
        if (!workers || *workers < 1 || *workers > common::MAX_THREADS) {
            std::cerr << "  Invalid worker count: " << positional_values[2] << std::endl;
            print_usage(argv[0]);
            std::exit(1);
        }
        cfg.worker_count = static_cast<std::uint32_t>(*workers);
    }

    if (positional >= 4) {
        const auto mode = parse_integer(positional_values[3].c_str());
        if (!mode || *mode < 0 || *mode > 6) {
            std::cerr << "  Invalid packet mode: " << positional_values[3] << std::endl;
            print_usage(argv[0]);
            std::exit(1);
        }
        switch (*mode) {
            case 0: cfg.packet_mode = PacketMode::Mixed; break;
            case 1: cfg.packet_mode = PacketMode::Tcp; break;
            case 2: cfg.packet_mode = PacketMode::Udp; break;
            case 3: cfg.packet_mode = PacketMode::Icmp; break;
            case 4: cfg.packet_mode = PacketMode::Ack; break;
            case 5: cfg.packet_mode = PacketMode::Rst; break;
            case 6: cfg.packet_mode = PacketMode::SynAck; break;
        }
    }

    if (positional >= 5) {
        const auto rate = parse_integer(positional_values[4].c_str());
        if (!rate || *rate < 0 || *rate > std::numeric_limits<std::uint32_t>::max()) {
            std::cerr << "  Invalid rate limit: " << positional_values[4] << std::endl;
            print_usage(argv[0]);
            std::exit(1);
        }
        cfg.rate_limit = static_cast<std::uint32_t>(*rate);
    }

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--real-ip") == 0) {
            cfg.use_spoof_ips = false;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                cfg.real_ip_interface = argv[++i];
            }
        } else if (std::strcmp(argv[i], "--spoof") == 0) {
            cfg.use_spoof_ips = true;
        } else if (std::strcmp(argv[i], "--interface") == 0 && i + 1 < argc) {
            cfg.real_ip_interface = argv[++i];
        }
    }

    return cfg;
}

void CliParser::print_usage(const char* program_name) {
    std::cout << "\n";
    std::cout << "  QEVORYX v4.0\n";
    std::cout << "  Usage: " << program_name << " <target_ip> <port> [threads] [mode] [rate] [flags]\n";
    std::cout << "\n";
    std::cout << "  Modes:\n";
    std::cout << "    0 - Mixed (TCP+UDP+ICMP)\n";
    std::cout << "    1 - TCP SYN\n";
    std::cout << "    2 - UDP\n";
    std::cout << "    3 - ICMP Echo\n";
    std::cout << "    4 - TCP ACK\n";
    std::cout << "    5 - TCP RST\n";
    std::cout << "    6 - TCP SYN-ACK\n";
    std::cout << "\n";
    std::cout << "  Flags:\n";
    std::cout << "    --help                    Show this help message\n";
    std::cout << "    --version                 Show version information\n";
    std::cout << "    --tui                     Launch interactive terminal UI\n";
    std::cout << "    --real-ip [interface]     Use the real interface IP (default)\n";
    std::cout << "    --spoof                   Explicitly enable spoofed source IPs\n";
    std::cout << "    --interface <name>        Specify network interface for real IP\n";
    std::cout << "\n";
    std::cout << "  Examples:\n";
    std::cout << "    sudo " << program_name << " 192.168.1.100 25565 5000 0 0\n";
    std::cout << "    sudo " << program_name << " 192.168.1.100 80 10000 1 0 --real-ip\n";
    std::cout << "    sudo " << program_name << " 192.168.1.100 53 5000 3 0 --interface wlan0\n";
    std::cout << "    sudo " << program_name << " --tui\n";
    std::cout << "\n";
}

} // namespace config
