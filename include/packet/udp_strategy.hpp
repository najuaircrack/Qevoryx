#pragma once

#include "packet/packet_strategy.hpp"

namespace packet {

class UdpStrategy final : public PacketStrategy {
public:
    const char* name() const noexcept override { return "UDP"; }
    bool build(const PacketContext& context, common::PacketBuffer& output) override;
};

} // namespace packet
