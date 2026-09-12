#include "monitor/monitor_factory.hpp"
#include "monitor/console_monitor.hpp"
#include "monitor/null_monitor.hpp"

namespace monitor {

std::unique_ptr<Monitor> create_monitor(config::MonitorMode mode) {
    switch (mode) {
        case config::MonitorMode::Console:
            return std::make_unique<ConsoleMonitor>();
        case config::MonitorMode::Silent:
        default:
            return std::make_unique<NullMonitor>();
    }
}

} // namespace monitor
