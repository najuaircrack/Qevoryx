#pragma once

#include "config/config.hpp"

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace ui {

enum class TuiScreen {
    Main,
    Runtime,
    Help
};

enum class FocusPanel {
    Configuration,
    Actions,
    Status,
    EventLog
};

enum class InputMode {
    Navigation,
    Editing,
    Modal
};

enum class Severity {
    Info,
    Warning,
    Error,
    Success
};

struct UiEventLogEntry {
    std::string timestamp;
    Severity severity{Severity::Info};
    std::string message;
};

struct ApplicationSnapshot {
    config::Config config;
    bool running{false};
    bool ready{true};
    std::uint64_t generated{0};
    std::uint64_t errors{0};
    std::vector<UiEventLogEntry> events;
};

struct TuiState {
    TuiScreen screen{TuiScreen::Main};
    FocusPanel focus_panel{FocusPanel::Configuration};
    InputMode input_mode{InputMode::Navigation};

    int selected_config_row{0};
    int selected_action{0};

    bool show_help{false};
    bool show_reset_confirmation{false};

    bool running{true};
    bool dirty{true};

    std::string error_message;
};

} // namespace ui
