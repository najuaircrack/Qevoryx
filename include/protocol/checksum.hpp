#pragma once

#include <cstdint>
#include <cstddef>

namespace protocol {

std::uint16_t internet_checksum(const void* data, std::size_t length) noexcept;

std::uint16_t tcp_checksum(
    const void* tcp_data,
    std::size_t tcp_length,
    std::uint32_t source,
    std::uint32_t destination) noexcept;

std::uint16_t udp_checksum(
    const void* udp_data,
    std::size_t udp_length,
    std::uint32_t source,
    std::uint32_t destination) noexcept;

std::uint16_t icmp_checksum(
    const void* icmp_data,
    std::size_t icmp_length) noexcept;

} // namespace protocol
