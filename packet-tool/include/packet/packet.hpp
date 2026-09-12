#pragma once

#include <memory>
#include "config/config.hpp"
#include "packet/packet_strategy.hpp"

namespace packet {

std::unique_ptr<PacketStrategy> create_strategy(config::PacketMode mode);

} // namespace packet
