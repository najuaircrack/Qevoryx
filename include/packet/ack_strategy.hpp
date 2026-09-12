#pragma once

#include "packet/packet_strategy.hpp"

namespace packet {

class AckStrategy final : public PacketStrategy {
public:
    const char* name() const noexcept override { return "TCP ACK"; }
    bool build(const PacketContext& context, common::PacketBuffer& output) override;
};

} // namespace packet
