#include "protocol/checksum.hpp"
#include "common/platform.hpp"
#include <cstring>

namespace protocol {

std::uint16_t internet_checksum(const void* data, std::size_t length) noexcept {
    const auto* ptr = static_cast<const std::uint16_t*>(data);
    std::uint32_t sum = 0;

    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }

    if (length == 1) {
        sum += *reinterpret_cast<const std::uint8_t*>(ptr);
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return static_cast<std::uint16_t>(~sum);
}

std::uint16_t tcp_checksum(const void* tcp_header, std::size_t tcp_length,
                           std::uint32_t src_ip, std::uint32_t dst_ip) noexcept {
    // Pseudo header + TCP data fit comfortably in 128 bytes
    std::uint8_t buffer[128];

    // Build pseudo header in first 12 bytes
    auto* pseudo = reinterpret_cast<std::uint32_t*>(buffer);
    pseudo[0] = src_ip;
    pseudo[1] = dst_ip;
    buffer[8] = 0;
    buffer[9] = 6; // IPPROTO_TCP
    auto* len_ptr = reinterpret_cast<std::uint16_t*>(buffer + 10);
    len_ptr[0] = htons(static_cast<std::uint16_t>(tcp_length));

    // Copy TCP data after pseudo header
    std::memcpy(buffer + 12, tcp_header, tcp_length);

    return internet_checksum(buffer, 12 + tcp_length);
}

std::uint16_t udp_checksum(const void* udp_header, std::size_t udp_length,
                           std::uint32_t src_ip, std::uint32_t dst_ip) noexcept {
    std::uint8_t buffer[128];

    auto* pseudo = reinterpret_cast<std::uint32_t*>(buffer);
    pseudo[0] = src_ip;
    pseudo[1] = dst_ip;
    buffer[8] = 0;
    buffer[9] = 17; // IPPROTO_UDP
    auto* len_ptr = reinterpret_cast<std::uint16_t*>(buffer + 10);
    len_ptr[0] = htons(static_cast<std::uint16_t>(udp_length));

    std::memcpy(buffer + 12, udp_header, udp_length);

    return internet_checksum(buffer, 12 + udp_length);
}

std::uint16_t icmp_checksum(const void* icmp_header, std::size_t icmp_length) noexcept {
    return internet_checksum(icmp_header, icmp_length);
}

} // namespace protocol
