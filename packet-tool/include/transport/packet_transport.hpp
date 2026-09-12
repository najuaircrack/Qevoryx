#pragma once

#include "common/types.hpp"

namespace transport {

class PacketTransport {
public:
    virtual ~PacketTransport() = default;
    virtual bool transmit(const common::PacketBuffer& packet) = 0;
    virtual void close() noexcept = 0;
};

} // namespace transport
