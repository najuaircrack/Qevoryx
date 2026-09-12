#include "config/cli_parser.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>

namespace config {

Config CliParser::parse(int argc, char** argv) {
    Config cfg;

    if (argc < 3) {
        print_usage(argv[0]);
        std::exit(1);
    }

    cfg.target_ip = argv[1];
    cfg.target_port = static_cast<std::uint16_t>(std::stoi(argv[2]));

    if (argc >= 4) cfg.worker_count = static_cast<std::uint32_t>(std::stoi(argv[3]));
    if (argc >= 5) {
        int mode = std::stoi(argv[4]);
        switch (mode) {
            case 0: cfg.packet_mode = PacketMode::Mixed; break;
            case 1: cfg.packet_mode = PacketMode::Tcp; break;
            case 2: cfg.packet_mode = PacketMode::Udp; break;
            default: cfg.packet_mode = PacketMode::Mixed; break;
        }
    }
    if (argc >= 6) cfg.rate_limit = static_cast<std::uint32_t>(std::stoi(argv[5]));

    if (cfg.worker_count > 10000) cfg.worker_count = 10000;

    return cfg;
}

void CliParser::print_usage(const char* program_name) {
    std::cout << "\n";
    std::cout << "  PACKET GENERATOR v3.0\n";
    std::cout << "  Usage: " << program_name << " <target_ip> <port> [threads] [mode] [rate]\n";
    std::cout << "\n";
    std::cout << "  Modes:\n";
    std::cout << "    0 - Mixed (TCP+UDP)\n";
    std::cout << "    1 - TCP SYN\n";
    std::cout << "    2 - UDP\n";
    std::cout << "\n";
    std::cout << "  Example:\n";
    std::cout << "    sudo " << program_name << " 192.168.1.100 25565 5000 0 0\n";
    std::cout << "\n";
}

} // namespace config
