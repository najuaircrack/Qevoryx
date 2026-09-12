#pragma once

#include "packet/packet_strategy.hpp"

namespace packet {

class IcmpStrategy final : public PacketStrategy {
public:
    const char* name() const noexcept override { return "ICMP"; }
    bool build(const PacketContext& context, common::PacketBuffer& output) override;
};

} // namespace packet
