#pragma once

#include <cstdint>
#include <cstddef>

namespace common {

constexpr int MAX_THREADS = 10000;
constexpr std::size_t MAX_PACKET_SIZE = 1500;
constexpr std::size_t MAX_SPOOF_IPS = 2000000;
constexpr int SPOOF_BUF_BITS = 21;
constexpr std::uint32_t SPOOF_BUF_SIZE = 1u << SPOOF_BUF_BITS;
constexpr std::uint32_t SPOOF_MASK = SPOOF_BUF_SIZE - 1;
constexpr int SEND_BUF_SIZE = 1048576;
constexpr std::uint32_t DEFAULT_PAYLOAD_MIN = 512;
constexpr std::uint32_t DEFAULT_PAYLOAD_MAX = 1400;

} // namespace common
