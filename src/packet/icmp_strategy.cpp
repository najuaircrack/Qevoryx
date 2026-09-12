#include "packet/icmp_strategy.hpp"
#include "packet/packet_strategy.hpp"
#include "config/config.hpp"
#include "protocol/ipv4.hpp"
#include "protocol/icmp.hpp"
#include "protocol/checksum.hpp"
#include "random/fast_random.hpp"
#include <cstring>
#include <arpa/inet.h>

namespace packet {

bool IcmpStrategy::build(const PacketContext& ctx, common::PacketBuffer& output) {
    output.clear();

    auto* iph = reinterpret_cast<protocol::IPv4Header*>(output.ptr());
    auto* icmph = reinterpret_cast<protocol::IcmpHeader*>(output.ptr() + protocol::IPv4_HEADER_SIZE);

    std::uint32_t src_ip = ctx.rng.next();
    std::uint16_t id = ctx.rng.next32() & 0xFFFF;
    std::uint16_t seq = ctx.rng.range(0, 65535);
    struct in_addr dst_addr;
    inet_pton(AF_INET, ctx.config.target_ip.c_str(), &dst_addr);

    int data_len = ctx.rng.range(32, 64);
    std::size_t payload_offset = protocol::IPv4_HEADER_SIZE + protocol::ICMP_HEADER_SIZE;

    // Fill payload with random bytes
    std::uint8_t* payload = output.ptr() + payload_offset;
    for (int i = 0; i < data_len; i++) {
        payload[i] = ctx.rng.next() & 0xFF;
    }

    // ICMP header
    icmph->type = protocol::ICMP_TYPE_ECHO_REQUEST;
    icmph->code = protocol::ICMP_CODE_ECHO;
    icmph->checksum = 0;
    icmph->id = htons(id);
    icmph->sequence = htons(seq);

    // ICMP checksum
    int icmp_len = protocol::ICMP_HEADER_SIZE + data_len;
    icmph->checksum = protocol::icmp_checksum(
        output.ptr() + protocol::IPv4_HEADER_SIZE, icmp_len);

    // IP header
    int pkt_len = protocol::IPv4_HEADER_SIZE + icmp_len;
    iph->version_ihl = protocol::IPv4_VERSION_IHL;
    iph->tos = 0;
    iph->total_length = htons(pkt_len);
    iph->identification = htons(ctx.rng.next32() & 0xFFFF);
    iph->flags_fragment = 0;
    iph->ttl = ctx.rng.range(64, 255);
    iph->protocol = 1; // ICMP
    iph->checksum = 0;
    iph->source = src_ip;
    iph->destination = dst_addr.s_addr;

    output.size = pkt_len;
    return true;
}

} // namespace packet
