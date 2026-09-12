#include "packet/packet.hpp"
#include "packet/tcp_syn_strategy.hpp"
#include "packet/udp_strategy.hpp"
#include "packet/icmp_strategy.hpp"
#include "packet/ack_strategy.hpp"
#include "packet/rst_strategy.hpp"
#include "packet/synack_strategy.hpp"
#include <stdexcept>

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
            return std::make_unique<MixedStrategy>();
        default:
            throw std::invalid_argument("Unsupported packet mode");
    }
}

MixedStrategy::MixedStrategy()
    : strategies_{
          std::make_unique<TcpSynStrategy>(),
          std::make_unique<UdpStrategy>(),
          std::make_unique<IcmpStrategy>()
      } {}

bool MixedStrategy::build(const PacketContext& context, common::PacketBuffer& output) {
    const std::uint64_t index = next_index_.fetch_add(1, std::memory_order_relaxed);
    return strategies_[index % 3]->build(context, output);
}

} // namespace packet
