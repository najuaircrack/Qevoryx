#pragma once

#include <cstdint>

namespace protocol {

struct IPv4Header {
    std::uint8_t  version_ihl;
    std::uint8_t  tos;
    std::uint16_t total_length;
    std::uint16_t identification;
    std::uint16_t flags_fragment;
    std::uint8_t  ttl;
    std::uint8_t  protocol;
    std::uint16_t checksum;
    std::uint32_t source;
    std::uint32_t destination;
} __attribute__((packed));

constexpr std::uint8_t IPv4_VERSION_IHL = 0x45;
constexpr std::uint16_t IPv4_HEADER_SIZE = sizeof(IPv4Header);
constexpr std::uint8_t IPPROTO_VALUE_TCP = 6;
constexpr std::uint8_t IPPROTO_VALUE_UDP = 17;

} // namespace protocol
