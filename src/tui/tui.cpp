#include "tui/tui.hpp"
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <ifaddrs.h>

namespace tui {

void Tui::clear_screen() {
    std::cout << "\033[2J\033[1;1H";
}

void Tui::print_divider() {
    std::cout << "  ─────────────────────────────────────────────────\n";
}

std::string Tui::input_string(const std::string& prompt, const std::string& default_val) {
    std::cout << "  " << prompt << " [" << default_val << "]: ";
    std::string input;
    std::getline(std::cin, input);
    return input.empty() ? default_val : input;
}

int Tui::input_int(const std::string& prompt, int default_val) {
    std::cout << "  " << prompt << " [" << default_val << "]: ";
    std::string input;
    std::getline(std::cin, input);
    if (input.empty()) return default_val;
    try { return std::stoi(input); }
    catch (...) { return default_val; }
}

bool Tui::input_bool(const std::string& prompt, bool default_val) {
    std::string hint = default_val ? "[Y/n]" : "[y/N]";
    std::cout << "  " << prompt << " " << hint << ": ";
    std::string input;
    std::getline(std::cin, input);
    if (input.empty()) return default_val;
    return (input[0] == 'y' || input[0] == 'Y');
}

int Tui::select_from_list(const std::string& prompt, const std::string* options, int count) {
    std::cout << "\n  " << prompt << "\n";
    for (int i = 0; i < count; i++) {
        std::cout << "    [" << i << "] " << options[i] << "\n";
    }
    std::cout << "\n";
    int choice = input_int("  Select", 0);
    if (choice < 0 || choice >= count) choice = 0;
    return choice;
}

std::string Tui::mode_to_string(config::PacketMode mode) {
    switch (mode) {
        case config::PacketMode::Mixed:   return "Mixed (TCP+UDP+ICMP)";
        case config::PacketMode::Tcp:     return "TCP SYN";
        case config::PacketMode::Udp:     return "UDP";
        case config::PacketMode::Icmp:    return "ICMP Echo";
        case config::PacketMode::Ack:     return "TCP ACK";
        case config::PacketMode::Rst:     return "TCP RST";
        case config::PacketMode::SynAck:  return "TCP SYN-ACK";
        default: return "Unknown";
    }
}

void Tui::print_banner() {
    std::cout << "\n";
    std::cout << "  ╔════════════════════════════════════════════════╗\n";
    std::cout << "  ║           QEVORYX  -  v4.0                     ║\n";
    std::cout << "  ║        Interactive Configuration Mode           ║\n";
    std::cout << "  ╚════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

config::Config Tui::run() {
    config::Config cfg;
    clear_screen();
    print_banner();

    // ── Step 1: Target ──
    print_divider();
    std::cout << "  [1/6] TARGET\n";
    print_divider();
    cfg.target_ip = input_string("  Target IP", "127.0.0.1");
    cfg.target_port = input_int("  Target port", 25565);
    std::cout << "\n";

    // ── Step 2: Attack Mode ──
    print_divider();
    std::cout << "  [2/6] ATTACK MODE\n";
    print_divider();
    std::string modes[] = {
        "Mixed (TCP+UDP+ICMP)  — round-robin across strategies",
        "TCP SYN               — SYN flood with random seq",
        "UDP                   — random payload flood",
        "ICMP Echo             — ping flood",
        "TCP ACK               — ACK flood",
        "TCP RST               — RST flood",
        "TCP SYN-ACK           — SYN+ACK flood"
    };
    int mode_idx = select_from_list("  Select attack mode:", modes, 7);
    switch (mode_idx) {
        case 0: cfg.packet_mode = config::PacketMode::Mixed; break;
        case 1: cfg.packet_mode = config::PacketMode::Tcp; break;
        case 2: cfg.packet_mode = config::PacketMode::Udp; break;
        case 3: cfg.packet_mode = config::PacketMode::Icmp; break;
        case 4: cfg.packet_mode = config::PacketMode::Ack; break;
        case 5: cfg.packet_mode = config::PacketMode::Rst; break;
        case 6: cfg.packet_mode = config::PacketMode::SynAck; break;
    }
    std::cout << "\n";

    // ── Step 3: Workers ──
    print_divider();
    std::cout << "  [3/6] WORKERS\n";
    print_divider();
    cfg.worker_count = input_int("  Number of worker threads", 1000);
    if (cfg.worker_count > 10000) cfg.worker_count = 10000;
    std::cout << "\n";

    // ── Step 4: IP Mode ──
    print_divider();
    std::cout << "  [4/6] IP MODE\n";
    print_divider();
    cfg.use_spoof_ips = input_bool("  Use spoofed source IPs?", true);

    if (!cfg.use_spoof_ips) {
        // List available interfaces
        std::cout << "\n  Available network interfaces:\n";
        struct ifaddrs* ifaddr = nullptr;
        if (getifaddrs(&ifaddr) == 0) {
            int iface_num = 0;
            for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr == nullptr) continue;
                if (ifa->ifa_addr->sa_family != AF_INET) continue;

                auto* sa = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr);
                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &sa->sin_addr, ip_str, INET_ADDRSTRLEN);

                std::cout << "    [" << iface_num << "] " << ifa->ifa_name
                          << " (" << ip_str << ")\n";
                iface_num++;
            }
            freeifaddrs(ifaddr);
        }

        cfg.real_ip_interface = input_string("  Interface name", "eth0");
    }
    std::cout << "\n";

    // ── Step 5: Payload (for UDP/ICMP) ──
    print_divider();
    std::cout << "  [5/6] PAYLOAD\n";
    print_divider();
    if (cfg.packet_mode == config::PacketMode::Udp ||
        cfg.packet_mode == config::PacketMode::Mixed) {
        cfg.payload_min = input_int("  Min payload bytes", 512);
        cfg.payload_max = input_int("  Max payload bytes", 1400);
    } else {
        std::cout << "  Skipped (not applicable for this mode)\n";
    }
    std::cout << "\n";

    // ── Step 6: Rate Limit ──
    print_divider();
    std::cout << "  [6/6] RATE LIMIT\n";
    print_divider();
    cfg.rate_limit = input_int("  PPS limit per thread (0 = unlimited)", 0);
    std::cout << "\n";

    return cfg;
}

bool Tui::confirm_launch(const config::Config& cfg) {
    print_divider();
    std::cout << "\n";
    std::cout << "  ╔════════════════════════════════════════════════╗\n";
    std::cout << "  ║              CONFIGURATION SUMMARY              ║\n";
    std::cout << "  ╠════════════════════════════════════════════════╣\n";
    std::cout << "  ║  Target:     " << cfg.target_ip << ":" << cfg.target_port;
    int pad = 33 - (int)cfg.target_ip.length() - (int)std::to_string(cfg.target_port).length();
    for (int i = 0; i < pad; i++) std::cout << " ";
    std::cout << "║\n";
    std::cout << "  ║  Mode:       " << mode_to_string(cfg.packet_mode);
    pad = 33 - (int)mode_to_string(cfg.packet_mode).length();
    for (int i = 0; i < pad; i++) std::cout << " ";
    std::cout << "║\n";
    std::cout << "  ║  Workers:    " << cfg.worker_count;
    pad = 33 - (int)std::to_string(cfg.worker_count).length();
    for (int i = 0; i < pad; i++) std::cout << " ";
    std::cout << "║\n";
    std::cout << "  ║  IP Mode:    " << (cfg.use_spoof_ips ? "Spoofed" : cfg.real_ip_interface);
    pad = 33 - (int)(cfg.use_spoof_ips ? 7 : cfg.real_ip_interface.length());
    for (int i = 0; i < pad; i++) std::cout << " ";
    std::cout << "║\n";
    std::cout << "  ║  Rate Limit: " << (cfg.rate_limit == 0 ? "Unlimited" : std::to_string(cfg.rate_limit));
    pad = 33 - (int)(cfg.rate_limit == 0 ? 9 : std::to_string(cfg.rate_limit).length());
    for (int i = 0; i < pad; i++) std::cout << " ";
    std::cout << "║\n";
    std::cout << "  ╚════════════════════════════════════════════════╝\n";
    std::cout << "\n";

    return input_bool("  Launch with these settings?", true);
}

} // namespace tui
