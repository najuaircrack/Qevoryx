#pragma once

#include "config/config.hpp"
#include "ui/application_controller.hpp"
#include "ui/renderer.hpp"
#include "ui/tui_event.hpp"
#include "ui/tui_layout.hpp"
#include "ui/tui_state.hpp"
#include "ui/tui_theme.hpp"

#include <optional>

namespace ui {

class TuiApplication {
public:
    explicit TuiApplication(ApplicationController& controller);
    ~TuiApplication();

    int run();

private:
    void initialize();
    void shutdown();

    std::optional<TuiEvent> poll_event();
    void process_event(const TuiEvent& event);
    void render();

    ApplicationController& controller_;
    TuiState state_;
    TuiTheme theme_;
    TuiLayout layout_;
    Renderer renderer_;
    config::Config config_;
};

} // namespace ui
