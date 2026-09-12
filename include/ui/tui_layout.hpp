#pragma once

#include "ui/tui_state.hpp"

namespace ui {

struct Rect {
    int x{0};
    int y{0};
    int width{0};
    int height{0};
};

class TuiLayout {
public:
    void update(TuiScreen screen);

    Rect header() const;
    Rect configuration() const;
    Rect actions() const;
    Rect status() const;
    Rect event_log() const;
    Rect footer() const;

    bool event_log_visible() const;

private:
    TuiScreen screen_{TuiScreen::Main};
    Rect header_;
    Rect configuration_;
    Rect actions_;
    Rect status_;
    Rect event_log_;
    Rect footer_;
    bool event_log_visible_{true};
};

} // namespace ui
