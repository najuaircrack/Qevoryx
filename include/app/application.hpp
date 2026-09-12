#pragma once

#include "config/config.hpp"
#include <thread>
#include <vector>

namespace app {

class Application {
public:
    explicit Application(config::Config config);
    int run();

    static void request_stop();
    static std::uint64_t generated_packets();
    static std::uint64_t error_count();

private:
    bool initialize();
    bool create_workers();
    void start_monitor();
    void wait_for_shutdown();
    void shutdown();

    config::Config config_;
    std::vector<std::thread> workers_;
};

} // namespace app
