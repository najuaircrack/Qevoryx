#include "packet/ack_strategy.hpp"
#include "packet/packet_strategy.hpp"
#include "config/config.hpp"
#include "protocol/ipv4.hpp"
#include "protocol/tcp.hpp"
#include "protocol/checksum.hpp"
#include "random/fast_random.hpp"
#include "common/platform.hpp"
#include <cstring>

namespace packet {

bool AckStrategy::build(const PacketContext& ctx, common::PacketBuffer& output) {
    output.clear();

    auto* iph = reinterpret_cast<protocol::IPv4Header*>(output.ptr());
    auto* tcph = reinterpret_cast<protocol::TcpHeader*>(output.ptr() + protocol::IPv4_HEADER_SIZE);

    std::uint32_t src_ip = ctx.rng.next();
    std::uint16_t sport = htons(ctx.rng.range(1024, 65535));
    std::uint32_t seq = ctx.rng.next32();
    std::uint32_t ack = ctx.rng.next32();
    struct in_addr dst_addr;
    inet_pton(AF_INET, ctx.config.target_ip.c_str(), &dst_addr);

    // IP header
    iph->version_ihl = protocol::IPv4_VERSION_IHL;
    iph->tos = ctx.rng.range(0, 255);
    iph->total_length = htons(protocol::IPv4_HEADER_SIZE + protocol::TCP_HEADER_SIZE);
    iph->identification = htons(ctx.rng.next32() & 0xFFFF);
    iph->flags_fragment = 0;
    iph->ttl = ctx.rng.range(64, 255);
    iph->protocol = protocol::IPPROTO_VALUE_TCP;
    iph->checksum = 0;
    iph->source = src_ip;
    iph->destination = dst_addr.s_addr;

    // TCP header — ACK only
    tcph->source_port = sport;
    tcph->destination_port = htons(ctx.config.target_port);
    tcph->sequence = htonl(seq);
    tcph->acknowledgement = htonl(ack);
    tcph->data_offset_reserved = protocol::TCP_DATA_OFFSET_5;
    tcph->flags = protocol::TCP_FLAG_ACK;
    tcph->window = htons(ctx.rng.range(1024, 65535));
    tcph->checksum = 0;
    tcph->urgent_pointer = 0;

    // TCP checksum
    tcph->checksum = protocol::tcp_checksum(
        output.ptr() + protocol::IPv4_HEADER_SIZE,
        protocol::TCP_HEADER_SIZE,
        src_ip, dst_addr.s_addr);

    output.size = protocol::IPv4_HEADER_SIZE + protocol::TCP_HEADER_SIZE;
    return true;
}

} // namespace packet
