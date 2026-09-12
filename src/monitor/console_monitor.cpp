#include "monitor/console_monitor.hpp"
#include "config/config.hpp"
#include <iostream>
#include <chrono>
#include <cstdio>
#include <cinttypes>

namespace monitor {

void ConsoleMonitor::start(const config::Config& config) {
    std::cout << "\n";
    std::cout << "  ========================================\n";
    std::cout << "    PACKET GENERATOR v4.0\n";
    std::cout << "  ========================================\n";
    std::cout << "    Target:    " << config.target_ip << ":" << config.target_port << "\n";
    std::cout << "    Workers:   " << config.worker_count << "\n";
    std::cout << "    Mode:      ";
    switch (config.packet_mode) {
        case config::PacketMode::Mixed:   std::cout << "Mixed (TCP+UDP+ICMP)"; break;
        case config::PacketMode::Tcp:     std::cout << "TCP SYN"; break;
        case config::PacketMode::Udp:     std::cout << "UDP"; break;
        case config::PacketMode::Icmp:    std::cout << "ICMP Echo"; break;
        case config::PacketMode::Ack:     std::cout << "TCP ACK"; break;
        case config::PacketMode::Rst:     std::cout << "TCP RST"; break;
        case config::PacketMode::SynAck:  std::cout << "TCP SYN-ACK"; break;
    }
    std::cout << "\n";
    std::cout << "  ========================================\n\n";
}

void ConsoleMonitor::update(const MonitorSnapshot& snapshot) {
    std::uint64_t delta = snapshot.generated - previous_generated_;
    previous_generated_ = snapshot.generated;
    if (delta > maximum_rate_) maximum_rate_ = delta;

    printf("\r  PPS: %-12" PRIu64 " | Total: %-14" PRIu64 " | Max: %-12" PRIu64,
           delta, snapshot.generated, maximum_rate_);
    fflush(stdout);
}

void ConsoleMonitor::stop() {
    std::cout << "\n\n";
    std::cout << "  ========================================\n";
    std::cout << "    FINAL STATISTICS\n";
    std::cout << "  ========================================\n";
    std::cout << "    Total packets: " << previous_generated_ << "\n";
    std::cout << "  ========================================\n";
}

} // namespace monitor
