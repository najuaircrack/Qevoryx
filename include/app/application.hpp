#pragma once

#include "config/config.hpp"

namespace app {

class Application {
public:
    explicit Application(config::Config config);
    int run();

private:
    bool initialize();
    void create_workers();
    void start_monitor();
    void wait_for_shutdown();
    void shutdown();

    config::Config config_;
};

} // namespace app
