#pragma once

#include "monitor/monitor.hpp"

namespace monitor {

class ConsoleMonitor final : public Monitor {
public:
    void start(const config::Config& config) override;
    void update(const MonitorSnapshot& snapshot) override;
    void stop() override;

private:
    std::uint64_t previous_generated_{0};
    std::uint64_t maximum_rate_{0};
};

} // namespace monitor
