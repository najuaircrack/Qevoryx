#pragma once

namespace ui {

enum class TuiEventType {
    NavigateUp,
    NavigateDown,
    NavigateLeft,
    NavigateRight,

    Activate,
    Cancel,
    Toggle,

    NextPanel,
    PreviousPanel,

    Launch,
    Save,
    Reset,
    Help,
    Quit,

    Resize
};

struct TuiEvent {
    TuiEventType type;
    int value{0};
};

} // namespace ui
