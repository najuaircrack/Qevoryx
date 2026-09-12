#pragma once

#include <cstdint>

namespace randomgen {

class FastRandom {
public:
    explicit FastRandom(std::uint64_t seed);

    std::uint64_t next() noexcept;
    std::uint32_t next32() noexcept;
    std::uint32_t range(std::uint32_t min, std::uint32_t max) noexcept;
    bool coin_flip(std::uint8_t probability_pct = 50) noexcept;

private:
    std::uint64_t state_[4];

    static std::uint64_t rotl(const std::uint64_t x, int k) noexcept {
        return (x << k) | (x >> (64 - k));
    }
};

} // namespace randomgen
