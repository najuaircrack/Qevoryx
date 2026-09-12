#pragma once

#include "monitor/monitor.hpp"

namespace monitor {

class NullMonitor final : public Monitor {
public:
    void start(const config::Config&) override {}
    void update(const MonitorSnapshot&) override {}
    void stop() override {}
};

} // namespace monitor
