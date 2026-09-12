#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include "common/constants.hpp"

namespace common {

struct PacketBuffer {
    alignas(64) std::uint8_t data[MAX_PACKET_SIZE];
    std::size_t size{0};

    void clear() noexcept {
        size = 0;
    }

    std::uint8_t* ptr() noexcept { return data; }
    const std::uint8_t* ptr() const noexcept { return data; }
};

struct PacketStatistics {
    std::uint64_t generated{0};
    std::uint64_t errors{0};

    void reset() noexcept {
        generated = 0;
        errors = 0;
    }
};

} // namespace common
