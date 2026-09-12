#include "protocol/checksum.hpp"
#include <arpa/inet.h>

namespace protocol {

std::uint16_t internet_checksum(const void* data, std::size_t length) noexcept {
    const auto* ptr = static_cast<const std::uint16_t*>(data);
    std::uint32_t sum = 0;
    int len = static_cast<int>(length);

    while (len > 1) { sum += *ptr++; len -= 2; }
    if (len > 0) sum += *reinterpret_cast<const std::uint8_t*>(ptr);
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
}

std::uint16_t tcp_checksum(
    const void* tcp_data,
    std::size_t tcp_length,
    std::uint32_t source,
    std::uint32_t destination) noexcept
{
    std::uint32_t sum = 0;
    sum += (source >> 16) & 0xFFFF;
    sum += source & 0xFFFF;
    sum += (destination >> 16) & 0xFFFF;
    sum += destination & 0xFFFF;
    sum += htons(6); // IPPROTO_TCP
    sum += htons(static_cast<std::uint16_t>(tcp_length));

    const auto* w = static_cast<const std::uint16_t*>(tcp_data);
    int nleft = static_cast<int>(tcp_length);
    while (nleft > 1) { sum += *w++; nleft -= 2; }
    if (nleft > 0) sum += *reinterpret_cast<const std::uint8_t*>(w);
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
}

} // namespace protocol
