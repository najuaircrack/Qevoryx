#include "random/fast_random.hpp"

namespace randomgen {

FastRandom::FastRandom(std::uint64_t seed) {
    for (int i = 0; i < 4; i++) {
        seed += 0x9e3779b97f4a7c15;
        std::uint64_t z = seed;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
        z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
        z = (z ^ (z >> 31));
        state_[i] = z;
    }
}

std::uint64_t FastRandom::next() noexcept {
    const std::uint64_t result = rotl(state_[1] * 5, 7) * 9;
    const std::uint64_t t = state_[1] << 17;
    state_[2] ^= state_[0];
    state_[3] ^= state_[1];
    state_[1] ^= state_[2];
    state_[0] ^= state_[3];
    state_[2] ^= t;
    state_[3] = rotl(state_[3], 45);
    return result;
}

std::uint32_t FastRandom::next32() noexcept {
    return next() & 0xFFFFFFFF;
}

std::uint32_t FastRandom::range(std::uint32_t min, std::uint32_t max) noexcept {
    if (min > max) {
        return min;
    }
    const std::uint64_t span = static_cast<std::uint64_t>(max) - min + 1;
    if (span == 0) {
        return static_cast<std::uint32_t>(next());
    }
    return static_cast<std::uint32_t>(min + (next() % span));
}

bool FastRandom::coin_flip(std::uint8_t probability_pct) noexcept {
    return (next() & 0xFF) < probability_pct;
}

} // namespace randomgen
