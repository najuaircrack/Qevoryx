#pragma once

#include <cstdint>

namespace config {
struct Config;
}

namespace monitor {

struct MonitorSnapshot {
    std::uint64_t generated;
    std::uint64_t errors;
};

class Monitor {
public:
    virtual ~Monitor() = default;
    virtual void start(const config::Config& config) = 0;
    virtual void update(const MonitorSnapshot& snapshot) = 0;
    virtual void stop() = 0;
};

} // namespace monitor
