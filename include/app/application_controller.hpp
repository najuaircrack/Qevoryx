#pragma once

#include "config/config.hpp"
#include "ftxui/state.hpp"

#include <memory>

namespace app {

class ApplicationController {
public:
    virtual ~ApplicationController() = default;

    virtual ui::ApplicationSnapshot snapshot() const = 0;
    virtual void launch(const config::Config& config) = 0;
    virtual void stop() = 0;
    virtual void save(const config::Config& config) = 0;
    virtual void reset() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
};

std::unique_ptr<ApplicationController> create_application_controller(config::Config config);

} // namespace app
