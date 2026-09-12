#pragma once

#include "common/types.hpp"

namespace config {
struct Config;
}

namespace randomgen {
class FastRandom;
}

namespace packet {

struct PacketContext {
    const config::Config& config;
    randomgen::FastRandom& rng;
    std::uint32_t destination_ip;
};

class PacketStrategy {
public:
    virtual ~PacketStrategy() = default;
    virtual const char* name() const noexcept = 0;
    virtual bool build(const PacketContext& context, common::PacketBuffer& output) = 0;
};

} // namespace packet
