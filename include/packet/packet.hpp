#pragma once

#include "config/config.hpp"
#include "packet/packet_strategy.hpp"
#include <atomic>
#include <memory>

namespace packet {

class MixedStrategy final : public PacketStrategy {
public:
    MixedStrategy();

    const char* name() const noexcept override { return "Mixed"; }
    bool build(const PacketContext& context, common::PacketBuffer& output) override;

private:
    std::unique_ptr<PacketStrategy> strategies_[3];
    std::atomic<std::uint64_t> next_index_{0};
};

std::unique_ptr<PacketStrategy> create_strategy(config::PacketMode mode);

} // namespace packet
