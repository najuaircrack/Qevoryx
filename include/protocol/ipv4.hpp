#pragma once

#include <cstdint>

#ifdef _MSC_VER
    #pragma pack(push, 1)
    #define PACKED_STRUCT struct
#else
    #define PACKED_STRUCT struct __attribute__((packed))
#endif

namespace protocol {

PACKED_STRUCT IPv4Header {
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
};

#ifdef _MSC_VER
    #pragma pack(pop)
#endif

constexpr std::uint8_t IPv4_VERSION_IHL = 0x45;
constexpr std::uint16_t IPv4_HEADER_SIZE = sizeof(IPv4Header);
constexpr std::uint8_t IPPROTO_VALUE_TCP = 6;
constexpr std::uint8_t IPPROTO_VALUE_UDP = 17;

} // namespace protocol
