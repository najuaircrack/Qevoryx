#include "packet/packet.hpp"
#include "packet/tcp_syn_strategy.hpp"
#include "packet/udp_strategy.hpp"

namespace packet {

std::unique_ptr<PacketStrategy> create_strategy(config::PacketMode mode) {
    switch (mode) {
        case config::PacketMode::Tcp:
            return std::make_unique<TcpSynStrategy>();
        case config::PacketMode::Udp:
            return std::make_unique<UdpStrategy>();
        case config::PacketMode::Mixed:
        default:
            return nullptr; // caller handles alternating
    }
}

} // namespace packet
