#include "ui/tui_layout.hpp"

#include <ncursesw/curses.h>

#include <algorithm>

namespace ui {

void TuiLayout::update(TuiScreen screen) {
    screen_ = screen;

    const int width = std::max(COLS, 1);
    const int height = std::max(LINES, 1);

    // The header grows with the terminal so a larger, more legible logo emblem
    // fits beside the wordmark. Heights are chosen so the configuration panel is
    // never starved more than the original fixed 6-row header did: a taller
    // header is only used when there are enough extra rows to pay for it. The
    // header widget then draws the largest emblem that fits the interior, down
    // to a small 8x4 square at height 6, or a plain wordmark at height 3.
    int header_height;
    if (height >= 40) {
        header_height = 10;
    } else if (height >= 35) {
        header_height = 9;
    } else if (height >= 31) {
        header_height = 7;
    } else if (height >= 24) {
        header_height = 6;
    } else {
        header_height = 3;
    }
    header_height = std::min(header_height, height);
    header_ = {0, 0, width, header_height};
    footer_ = {0, std::max(height - 2, header_height), width, std::min(2, std::max(height - header_height, 0))};

    const int body_top = header_.height;
    const int body_bottom = height - footer_.height;
    const int body_height = std::max(body_bottom - body_top, 0);

    if (screen == TuiScreen::Help) {
        configuration_ = {0, body_top, width, body_height};
        actions_ = {};
        status_ = {};
        event_log_ = {};
        event_log_visible_ = false;
        return;
    }

    if (screen == TuiScreen::Runtime) {
        const int runtime_width = std::max(width * 55 / 100, 1);
        const int live_width = std::max(width - runtime_width, 0);
        const int log_height = height >= 30 ? std::min(8, body_height / 4) : 0;
        const int panel_height = std::max(body_height - log_height, 0);

        configuration_ = {0, body_top, runtime_width, panel_height};
        actions_ = {runtime_width, body_top, live_width, panel_height};
        status_ = {};
        event_log_ = {0, body_top + panel_height, width, log_height};
        event_log_visible_ = log_height > 0;
        return;
    }

    const bool compact = height < 30 || width < 100;
    const int log_height = compact ? (height >= 26 ? 5 : 0) : std::min(8, body_height / 4);
    const int status_height = std::min(compact ? 4 : 5, body_height);
    const int panel_height = std::max(body_height - log_height - status_height, 0);
    const int configuration_width = width >= 72 ? std::max(width * 58 / 100, 42) : std::max(width / 2, 1);
    const int actions_width = std::max(width - configuration_width, 0);

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
