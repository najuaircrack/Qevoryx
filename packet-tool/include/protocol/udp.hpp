#pragma once

#include <cstdint>

namespace protocol {

struct UdpHeader {
    std::uint16_t source_port;
    std::uint16_t destination_port;
    std::uint16_t length;
    std::uint16_t checksum;
} __attribute__((packed));

constexpr std::uint16_t UDP_HEADER_SIZE = sizeof(UdpHeader);

} // namespace protocol
