#include "config/cli_parser.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>

namespace config {

bool CliParser::has_tui_flag(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--tui") == 0) return true;
    }
    return false;
}

Config CliParser::parse(int argc, char** argv) {
    Config cfg;

    if (argc < 3) {
        print_usage(argv[0]);
        std::exit(1);
    }

    int positional = 0;
    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (arg[0] == '-' && arg[1] == '-' && arg[2] != '\0') {
            continue;
        }

        switch (positional) {
            case 0:
                cfg.target_ip = arg;
                break;
            case 1:
                cfg.target_port = static_cast<std::uint16_t>(std::stoi(arg));
                break;
            case 2:
                cfg.worker_count = static_cast<std::uint32_t>(std::stoi(arg));
                break;
            case 3: {
                int mode = std::stoi(arg);
                switch (mode) {
                    case 0: cfg.packet_mode = PacketMode::Mixed; break;
                    case 1: cfg.packet_mode = PacketMode::Tcp; break;
                    case 2: cfg.packet_mode = PacketMode::Udp; break;
                    case 3: cfg.packet_mode = PacketMode::Icmp; break;
                    case 4: cfg.packet_mode = PacketMode::Ack; break;
                    case 5: cfg.packet_mode = PacketMode::Rst; break;
                    case 6: cfg.packet_mode = PacketMode::SynAck; break;
                    default: cfg.packet_mode = PacketMode::Mixed; break;
                }
                break;
            }
            case 4:
                cfg.rate_limit = static_cast<std::uint32_t>(std::stoi(arg));
                break;
            default:
                break;
        }
        ++positional;
    }

    if (positional < 2) {
        print_usage(argv[0]);
        std::exit(1);
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

    if (cfg.worker_count > 10000) cfg.worker_count = 10000;

    return cfg;
}

void CliParser::print_usage(const char* program_name) {
    std::cout << "\n";
    std::cout << "  QEVORYX v4.0\n";
    std::cout << "  Usage: " << program_name << " <target_ip> <port> [threads] [mode] [rate] [flags]\n";
    std::cout << "\n";
    std::cout << "  Modes:\n";
    std::cout << "    0 - Mixed (TCP+UDP)\n";
    std::cout << "    1 - TCP SYN\n";
    std::cout << "    2 - UDP\n";
    std::cout << "    3 - ICMP Echo\n";
    std::cout << "    4 - TCP ACK\n";
    std::cout << "    5 - TCP RST\n";
    std::cout << "    6 - TCP SYN-ACK\n";
    std::cout << "\n";
    std::cout << "  Flags:\n";
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
