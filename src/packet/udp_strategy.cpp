#include "packet/udp_strategy.hpp"
#include "packet/packet_strategy.hpp"
#include "config/config.hpp"
#include "protocol/ipv4.hpp"
#include "protocol/udp.hpp"
#include "random/fast_random.hpp"
#include "common/constants.hpp"
#include "common/platform.hpp"
#include <cstring>

namespace packet {

bool UdpStrategy::build(const PacketContext& ctx, common::PacketBuffer& output) {
    output.clear();

    auto* iph = reinterpret_cast<protocol::IPv4Header*>(output.ptr());
    auto* udph = reinterpret_cast<protocol::UdpHeader*>(output.ptr() + protocol::IPv4_HEADER_SIZE);

    std::uint32_t src_ip = ctx.rng.next();
    std::uint16_t sport = htons(ctx.rng.range(1024, 65535));
    struct in_addr dst_addr;
    inet_pton(AF_INET, ctx.config.target_ip.c_str(), &dst_addr);

    int data_len = ctx.rng.range(ctx.config.payload_min, ctx.config.payload_max);
    std::size_t payload_offset = protocol::IPv4_HEADER_SIZE + protocol::UDP_HEADER_SIZE;

    // Fill payload with random bytes — unrolled 8 bytes at a time
    std::uint8_t* payload = output.ptr() + payload_offset;
    int i = 0;
    for (; i + 8 <= data_len; i += 8) {
        std::uint64_t rnd = ctx.rng.next();
        std::memcpy(payload + i, &rnd, 8);
    }
    for (; i < data_len; i++) {
        payload[i] = ctx.rng.next() & 0xFF;
    }

    int pkt_len = protocol::IPv4_HEADER_SIZE + protocol::UDP_HEADER_SIZE + data_len;

    // IP header
    iph->version_ihl = protocol::IPv4_VERSION_IHL;
    iph->tos = 0;
    iph->total_length = htons(pkt_len);
    iph->identification = htons(ctx.rng.next32() & 0xFFFF);
    iph->flags_fragment = 0;
    iph->ttl = ctx.rng.range(64, 255);
    iph->protocol = protocol::IPPROTO_VALUE_UDP;
    iph->checksum = 0;
    iph->source = src_ip;
    iph->destination = dst_addr.s_addr;

    // UDP header
    udph->source_port = sport;
    udph->destination_port = htons(ctx.config.target_port);
    udph->length = htons(protocol::UDP_HEADER_SIZE + data_len);
    udph->checksum = 0;

    output.size = pkt_len;
    return true;
}

} // namespace packet
