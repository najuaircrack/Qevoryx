#include "ui/widgets/event_log_panel.hpp"

#include "ui/tui_state.hpp"
#include "ui/widgets/widget_paint.hpp"

#include <ncursesw/curses.h>

#include <algorithm>
#include <string>

namespace ui {
namespace {

int level_color(Severity severity, const TuiTheme& theme) {
    switch (severity) {
        case Severity::Info: return theme.accent;
        case Severity::Warning: return theme.warning;
        case Severity::Error: return theme.danger;
        case Severity::Success: return theme.success;
    }
    return theme.secondary;
}

} // namespace

void EventLogPanel::render(const TuiState& state,
                           const ApplicationSnapshot& snapshot,
                           const TuiTheme& theme) {
    WINDOW* window = this->window();
    if (window == nullptr) {
        return;
    }

    const int width = rect().width;
    const int height = rect().height;
    const bool focused = state.focus_panel == FocusPanel::EventLog;

    werase(window);
    paint::frame(window, focused ? theme.border_focused : theme.border);
    paint::title(window, (focused ? theme.accent : theme.header) | A_BOLD, "EVENT LOG", width);

    const int content_x = paint::content_x();
    const int content_w = paint::content_width(width);
    const int first_row = paint::content_y();
    const int last_row = paint::content_bottom(height);
    const int available_rows = std::max(last_row - first_row + 1, 0);
    if (content_w <= 0 || available_rows <= 0) {
        wnoutrefresh(window);
        return;
    }

    const int entry_count = std::min(static_cast<int>(snapshot.events.size()), available_rows);
    const int maximum_start = std::max(static_cast<int>(snapshot.events.size()) - entry_count, 0);
    const int start = std::max(maximum_start - state.event_log_offset, 0);

    // Scroll indicator in the top-right of the border area.
    if (focused && content_w > 2) {
        const char* arrow = state.event_log_offset > 0
                                ? (theme.unicode_available ? "\xE2\x86\x91" : "^")
                                : (theme.unicode_available ? "\xE2\x80\xA2" : "-");
        wattron(window, theme.accent);
        mvwaddnstr(window, paint::title_y(), width - paint::content_x() - 1, arrow, 1);
        wattroff(window, theme.accent);
    }

    const int display_count = std::min(entry_count,
                                       static_cast<int>(snapshot.events.size()) - start);

    // Column geometry: timestamp | level | message, all inside content_w.
    const int ts_w = 8;   // "HH:MM:SS"
    const int level_w = 6; // "ERROR "
    const int gap = 2;

    for (int row = 0; row < display_count; ++row) {
        const auto& entry = snapshot.events[static_cast<std::size_t>(start + row)];
        const int y = first_row + row;
        int x = content_x;

        x = paint::text(window, y, x, ts_w, theme.muted, entry.timestamp);
        x = content_x + ts_w + gap;

        if (x < content_x + content_w) {
            paint::text(window, y, x, level_w, level_color(entry.severity, theme) | A_BOLD,
                        severity_label(entry.severity));
        }
        const int msg_x = content_x + ts_w + gap + level_w + gap;
        const int msg_w = std::max(content_x + content_w - msg_x, 0);
        if (msg_w > 0) {
            paint::text(window, y, msg_x, msg_w, theme.secondary, entry.message);
        }
    }

    wnoutrefresh(window);
}

} // namespace ui
