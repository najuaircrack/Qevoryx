#include "transport/test_transport.hpp"

namespace transport {

bool TestTransport::transmit(const common::PacketBuffer& packet) {
    last_packet_ = packet;
    count_++;
    return true;
}

void TestTransport::close() noexcept {}

} // namespace transport
