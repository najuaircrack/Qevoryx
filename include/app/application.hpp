#pragma once

#include "config/config.hpp"
#include <thread>
#include <vector>

namespace app {

class Application {
public:
    explicit Application(config::Config config, bool install_signal_handlers = true);
    int run();

    static void request_stop();
    static void pause();
    static void resume();
    static std::uint64_t generated_packets();
    static std::uint64_t error_count();

private:
    bool initialize();
    bool create_workers();
    void start_monitor();
    void wait_for_shutdown();
    void shutdown();

    config::Config config_;
    bool install_signal_handlers_;
    std::vector<std::thread> workers_;
};

} // namespace app
