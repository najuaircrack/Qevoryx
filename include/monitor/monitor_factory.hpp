#pragma once

#include <memory>
#include "config/config.hpp"
#include "monitor/monitor.hpp"

namespace monitor {

std::unique_ptr<Monitor> create_monitor(config::MonitorMode mode);

} // namespace monitor
