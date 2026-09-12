#include "packet/packet.hpp"
#include "packet/tcp_syn_strategy.hpp"
#include "packet/udp_strategy.hpp"
#include "packet/icmp_strategy.hpp"
#include "packet/ack_strategy.hpp"
#include "packet/rst_strategy.hpp"
#include "packet/synack_strategy.hpp"

namespace packet {

std::unique_ptr<PacketStrategy> create_strategy(config::PacketMode mode) {
    switch (mode) {
        case config::PacketMode::Tcp:
            return std::make_unique<TcpSynStrategy>();
        case config::PacketMode::Udp:
            return std::make_unique<UdpStrategy>();
        case config::PacketMode::Icmp:
            return std::make_unique<IcmpStrategy>();
        case config::PacketMode::Ack:
            return std::make_unique<AckStrategy>();
        case config::PacketMode::Rst:
            return std::make_unique<RstStrategy>();
        case config::PacketMode::SynAck:
            return std::make_unique<SynAckStrategy>();
        case config::PacketMode::Mixed:
        default:
            return nullptr; // caller handles alternating
    }
}

} // namespace packet
