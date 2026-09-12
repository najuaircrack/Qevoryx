#include "protocol/checksum.hpp"
#include "common/platform.hpp"

namespace protocol {

std::uint16_t internet_checksum(const void* data, std::size_t length) {
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
                           std::uint32_t src_ip, std::uint32_t dst_ip) {
    struct PseudoHeader {
        std::uint32_t src;
        std::uint32_t dst;
        std::uint8_t zero;
        std::uint8_t protocol;
        std::uint16_t tcp_length;
    };

    PseudoHeader pseudo{};
    pseudo.src = src_ip;
    pseudo.dst = dst_ip;
    pseudo.zero = 0;
    pseudo.protocol = 6; // IPPROTO_TCP
    pseudo.tcp_length = htons(static_cast<std::uint16_t>(tcp_length));

    // Calculate total checksum data length
    std::size_t total_len = sizeof(PseudoHeader) + tcp_length;

    // Allocate on stack and copy
    auto* buffer = static_cast<std::uint8_t*>(__builtin_alloca(total_len));
    std::memcpy(buffer, &pseudo, sizeof(PseudoHeader));
    std::memcpy(buffer + sizeof(PseudoHeader), tcp_header, tcp_length);

    std::uint16_t result = internet_checksum(buffer, total_len);
    return result;
}

std::uint16_t udp_checksum(const void* udp_header, std::size_t udp_length,
                           std::uint32_t src_ip, std::uint32_t dst_ip) {
    struct PseudoHeader {
        std::uint32_t src;
        std::uint32_t dst;
        std::uint8_t zero;
        std::uint8_t protocol;
        std::uint16_t udp_length;
    };

    PseudoHeader pseudo{};
    pseudo.src = src_ip;
    pseudo.dst = dst_ip;
    pseudo.zero = 0;
    pseudo.protocol = 17; // IPPROTO_UDP
    pseudo.udp_length = htons(static_cast<std::uint16_t>(udp_length));

    std::size_t total_len = sizeof(PseudoHeader) + udp_length;

    auto* buffer = static_cast<std::uint8_t*>(__builtin_alloca(total_len));
    std::memcpy(buffer, &pseudo, sizeof(PseudoHeader));
    std::memcpy(buffer + sizeof(PseudoHeader), udp_header, udp_length);

    std::uint16_t result = internet_checksum(buffer, total_len);
    return result;
}

std::uint16_t icmp_checksum(const void* icmp_header, std::size_t icmp_length) {
    return internet_checksum(icmp_header, icmp_length);
}

} // namespace protocol
