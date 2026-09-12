#pragma once

#include "transport/packet_transport.hpp"

namespace transport {

class TestTransport final : public PacketTransport {
public:
    bool transmit(const common::PacketBuffer& packet) override;
    void close() noexcept override;

    const common::PacketBuffer& last_packet() const noexcept { return last_packet_; }
    std::size_t transmit_count() const noexcept { return count_; }

private:
    common::PacketBuffer last_packet_;
    std::size_t count_{0};
};

} // namespace transport
