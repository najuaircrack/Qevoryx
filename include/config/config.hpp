#pragma once

#include <string>
#include <cstdint>

namespace config {

enum class PacketMode {
    Mixed,
    Tcp,
    Udp,
    Icmp,
    Ack,
    Rst,
    SynAck
};

enum class MonitorMode {
    Console,
    Silent
};

struct Config {
    std::string target_ip;
    std::uint16_t target_port{25565};
    std::uint32_t worker_count{1};
    PacketMode packet_mode{PacketMode::Mixed};
    MonitorMode monitor_mode{MonitorMode::Console};
    std::uint32_t payload_min{0};
    std::uint32_t payload_max{512};
    std::uint32_t rate_limit{1000};
    bool use_spoof_ips{false};
    std::string real_ip_interface{"eth0"};
};

} // namespace config
