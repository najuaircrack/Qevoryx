#include "packet/tcp_syn_strategy.hpp"
#include "packet/packet_strategy.hpp"
#include "config/config.hpp"
#include "protocol/ipv4.hpp"
#include "protocol/tcp.hpp"
#include "protocol/checksum.hpp"
#include "random/fast_random.hpp"
#include "common/constants.hpp"
#include "common/platform.hpp"
#include <cstring>

namespace packet {

bool TcpSynStrategy::build(const PacketContext& ctx, common::PacketBuffer& output) {
    output.clear();

    auto* iph = reinterpret_cast<protocol::IPv4Header*>(output.ptr());
    auto* tcph = reinterpret_cast<protocol::TcpHeader*>(output.ptr() + protocol::IPv4_HEADER_SIZE);

    std::uint32_t src_ip = ctx.rng.next();
    std::uint16_t sport = htons(ctx.rng.range(1024, 65535));
    std::uint32_t seq = ctx.rng.next32();
    const std::uint32_t dst_ip = ctx.destination_ip;

    // IP header
    iph->version_ihl = protocol::IPv4_VERSION_IHL;
    iph->tos = ctx.rng.range(0, 255);
    iph->identification = htons(ctx.rng.next32() & 0xFFFF);
    iph->flags_fragment = 0;
    iph->ttl = ctx.rng.range(64, 255);
    iph->protocol = protocol::IPPROTO_VALUE_TCP;
    iph->source = src_ip;
    iph->destination = dst_ip;

    // TCP header
    tcph->source_port = sport;
    tcph->destination_port = htons(ctx.config.target_port);
    tcph->sequence = htonl(seq);
    tcph->acknowledgement = 0;
    tcph->data_offset_reserved = protocol::TCP_DATA_OFFSET_5;
    tcph->flags = protocol::TCP_FLAG_SYN;
    tcph->window = htons(ctx.rng.range(1024, 65535));
    tcph->checksum = 0;
    tcph->urgent_pointer = 0;

    int pkt_len = protocol::IPv4_HEADER_SIZE + protocol::TCP_HEADER_SIZE;

    // TCP options (MSS, ~30% chance)
    if (ctx.rng.coin_flip(30)) {
        if (pkt_len + 4 <= common::MAX_PACKET_SIZE) {
            auto* opt = output.ptr() + protocol::IPv4_HEADER_SIZE + protocol::TCP_HEADER_SIZE;
            opt[0] = protocol::TCP_OPTION_MSS_KIND;
            opt[1] = protocol::TCP_OPTION_MSS_LEN;
            opt[2] = (protocol::TCP_OPTION_MSS_VALUE >> 8) & 0xFF;
            opt[3] = protocol::TCP_OPTION_MSS_VALUE & 0xFF;
            tcph->data_offset_reserved = protocol::TCP_DATA_OFFSET_6;
            pkt_len += 4;
        }
    }

    iph->total_length = htons(pkt_len);
    iph->checksum = 0;
    iph->checksum = protocol::internet_checksum(reinterpret_cast<const std::uint8_t*>(iph), 20);
    tcph->checksum = protocol::tcp_checksum(
        output.ptr() + protocol::IPv4_HEADER_SIZE,
        pkt_len - protocol::IPv4_HEADER_SIZE,
            src_ip, dst_ip);

    output.size = pkt_len;
    return true;
}

} // namespace packet
