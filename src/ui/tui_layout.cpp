#include "ui/tui_layout.hpp"

#include <ncursesw/curses.h>

#include <algorithm>

namespace ui {

void TuiLayout::update(TuiScreen screen) {
    screen_ = screen;

    const int width = std::max(COLS, 80);
    const int height = std::max(LINES, 24);

    header_ = {0, 0, width, 4};
    footer_ = {0, height - 2, width, 2};

    const int body_top = header_.height;
    const int body_bottom = height - footer_.height;
    const int body_height = std::max(body_bottom - body_top, 10);

    if (screen == TuiScreen::Help) {
        configuration_ = {0, body_top, width, body_height};
        actions_ = {};
        status_ = {};
        event_log_ = {};
        event_log_visible_ = false;
        return;
    }

    if (screen == TuiScreen::Runtime) {
        const int runtime_width = std::max(width * 55 / 100, 40);
        const int live_width = std::max(width - runtime_width, 30);
        const int log_height = height >= 30 ? std::min(8, body_height / 4) : 0;
        const int panel_height = std::max(body_height - log_height, 8);

        configuration_ = {0, body_top, runtime_width, panel_height};
        actions_ = {runtime_width, body_top, live_width, panel_height};
        status_ = {};
        event_log_ = {0, body_top + panel_height, width, log_height};
        event_log_visible_ = log_height > 0;
        return;
    }

    const bool compact = height < 30 || width < 100;
    const int log_height = compact ? (height >= 26 ? 4 : 0) : std::min(8, body_height / 4);
    const int status_height = compact ? 4 : 5;
    const int panel_height = std::max(body_height - log_height - status_height, 8);
    const int configuration_width = std::max(width * 58 / 100, 42);
    const int actions_width = std::max(width - configuration_width, 30);

    configuration_ = {0, body_top, configuration_width, panel_height};
    actions_ = {configuration_width, body_top, actions_width, panel_height};
    status_ = {0, body_top + panel_height, width, status_height};
    event_log_ = {0, status_.y + status_height, width, log_height};
    event_log_visible_ = log_height > 0;
}

Rect TuiLayout::header() const { return header_; }
Rect TuiLayout::configuration() const { return configuration_; }
Rect TuiLayout::actions() const { return actions_; }
Rect TuiLayout::status() const { return status_; }
Rect TuiLayout::event_log() const { return event_log_; }
Rect TuiLayout::footer() const { return footer_; }
bool TuiLayout::event_log_visible() const { return event_log_visible_; }

} // namespace ui
