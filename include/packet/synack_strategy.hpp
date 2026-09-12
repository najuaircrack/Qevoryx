#pragma once

#include "packet/packet_strategy.hpp"

namespace packet {

class SynAckStrategy final : public PacketStrategy {
public:
    const char* name() const noexcept override { return "TCP SYN-ACK"; }
    bool build(const PacketContext& context, common::PacketBuffer& output) override;
};

} // namespace packet
