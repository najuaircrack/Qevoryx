#pragma once

#include <cstdint>

namespace protocol {

struct TcpHeader {
    std::uint16_t source_port;
    std::uint16_t destination_port;
    std::uint32_t sequence;
    std::uint32_t acknowledgement;
    std::uint8_t  data_offset_reserved;
    std::uint8_t  flags;
    std::uint16_t window;
    std::uint16_t checksum;
    std::uint16_t urgent_pointer;
} __attribute__((packed));

constexpr std::uint8_t TCP_DATA_OFFSET_5 = 0x50;
constexpr std::uint8_t TCP_DATA_OFFSET_6 = 0x60;
constexpr std::uint8_t TCP_FLAG_SYN = 0x02;
constexpr std::uint8_t TCP_FLAG_ACK = 0x10;
constexpr std::uint8_t TCP_FLAG_RST = 0x04;
constexpr std::uint8_t TCP_FLAG_SYN_ACK = 0x12;
constexpr std::uint16_t TCP_HEADER_SIZE = sizeof(TcpHeader);
constexpr std::uint8_t TCP_OPTION_MSS_KIND = 2;
constexpr std::uint8_t TCP_OPTION_MSS_LEN = 4;
constexpr std::uint16_t TCP_OPTION_MSS_VALUE = 1460;

} // namespace protocol
