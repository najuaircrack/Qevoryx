#include "protocol/checksum.hpp"
#include "common/platform.hpp"
#include <cstring>
#include <array>

namespace protocol {

namespace {

std::uint32_t checksum_sum(const std::uint8_t* data, std::size_t length) noexcept {
    std::uint32_t sum = 0;

    for (std::size_t i = 0; i + 1 < length; i += 2) {
        sum += (static_cast<std::uint32_t>(data[i]) << 8) | data[i + 1];
    }

    if (length & 1) {
        sum += static_cast<std::uint32_t>(data[length - 1]) << 8;
    }

    return sum;
}

std::uint16_t fold_checksum(std::uint32_t sum) noexcept {
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return static_cast<std::uint16_t>(~sum);
}

} // namespace

std::uint16_t internet_checksum(const void* data, std::size_t length) noexcept {
    return fold_checksum(checksum_sum(static_cast<const std::uint8_t*>(data), length));
}

std::uint16_t tcp_checksum(const void* tcp_header, std::size_t tcp_length,
                           std::uint32_t src_ip, std::uint32_t dst_ip) noexcept {
    if (tcp_length > 65535) return 0;

    std::array<std::uint8_t, 12> pseudo{};
    std::memcpy(pseudo.data() + 0, &src_ip, sizeof(src_ip));
    std::memcpy(pseudo.data() + 4, &dst_ip, sizeof(dst_ip));
    pseudo[8] = 0;
    pseudo[9] = 6;
    const std::uint16_t network_length = htons(static_cast<std::uint16_t>(tcp_length));
    std::memcpy(pseudo.data() + 10, &network_length, sizeof(network_length));

    const auto* tcp_bytes = static_cast<const std::uint8_t*>(tcp_header);
    const std::uint32_t sum = checksum_sum(pseudo.data(), pseudo.size()) +
                              checksum_sum(tcp_bytes, tcp_length);
    return fold_checksum(sum);
}

std::uint16_t udp_checksum(const void* udp_header, std::size_t udp_length,
                           std::uint32_t src_ip, std::uint32_t dst_ip) noexcept {
    if (udp_length > 65535) return 0;

    std::array<std::uint8_t, 12> pseudo{};
    std::memcpy(pseudo.data() + 0, &src_ip, sizeof(src_ip));
    std::memcpy(pseudo.data() + 4, &dst_ip, sizeof(dst_ip));
    pseudo[8] = 0;
    pseudo[9] = 17;
    const std::uint16_t network_length = htons(static_cast<std::uint16_t>(udp_length));
    std::memcpy(pseudo.data() + 10, &network_length, sizeof(network_length));

    const auto* udp_bytes = static_cast<const std::uint8_t*>(udp_header);
    const std::uint32_t sum = checksum_sum(pseudo.data(), pseudo.size()) +
                              checksum_sum(udp_bytes, udp_length);
    return fold_checksum(sum);
}

std::uint16_t icmp_checksum(const void* icmp_header, std::size_t icmp_length) noexcept {
    return internet_checksum(icmp_header, icmp_length);
}

} // namespace protocol
