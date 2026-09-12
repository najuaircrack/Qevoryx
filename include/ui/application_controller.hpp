#pragma once

#include "config/config.hpp"
#include "ui/tui_state.hpp"

namespace ui {

class ApplicationController {
public:
    virtual ~ApplicationController() = default;

    virtual ApplicationSnapshot snapshot() const = 0;
    virtual void launch(const config::Config& config) = 0;
    virtual void stop() = 0;
    virtual void save(const config::Config& config) = 0;
    virtual void reset() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
};

} // namespace ui
