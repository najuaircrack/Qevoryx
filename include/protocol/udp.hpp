#pragma once

#include <cstdint>

#ifdef _MSC_VER
    #pragma pack(push, 1)
    #define PACKED_STRUCT struct
#else
    #define PACKED_STRUCT struct __attribute__((packed))
#endif

namespace protocol {

PACKED_STRUCT UdpHeader {
    std::uint16_t source_port;
    std::uint16_t destination_port;
    std::uint16_t length;
    std::uint16_t checksum;
};

#ifdef _MSC_VER
    #pragma pack(pop)
#endif

constexpr std::uint16_t UDP_HEADER_SIZE = sizeof(UdpHeader);

} // namespace protocol
