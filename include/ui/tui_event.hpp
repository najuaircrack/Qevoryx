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
    Insert,
    Backspace,
    Delete,

    NextPanel,
    PreviousPanel,

    Launch,
    Save,
    Reset,
    Help,
    Quit,
    Stop,
    Pause,
    Resume,
    Return,

    Resize
};

struct TuiEvent {
    TuiEventType type;
    int value{0};
};

} // namespace ui
