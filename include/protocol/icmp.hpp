#pragma once

#include <cstdint>

#ifdef _MSC_VER
    #pragma pack(push, 1)
    #define PACKED_STRUCT struct
#else
    #define PACKED_STRUCT struct __attribute__((packed))
#endif

namespace protocol {

PACKED_STRUCT IcmpHeader {
    std::uint8_t  type;
    std::uint8_t  code;
    std::uint16_t checksum;
    std::uint16_t id;
    std::uint16_t sequence;
};

#ifdef _MSC_VER
    #pragma pack(pop)
#endif

constexpr std::uint8_t ICMP_TYPE_ECHO_REQUEST = 8;
constexpr std::uint8_t ICMP_TYPE_ECHO_REPLY = 0;
constexpr std::uint8_t ICMP_CODE_ECHO = 0;
constexpr std::uint16_t ICMP_HEADER_SIZE = sizeof(IcmpHeader);

} // namespace protocol
