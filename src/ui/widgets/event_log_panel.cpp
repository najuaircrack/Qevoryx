#include "ui/widgets/event_log_panel.hpp"

#include "ui/tui_state.hpp"

#include <ncursesw/curses.h>

#include <algorithm>
#include <string>

namespace ui {

void EventLogPanel::render(const TuiState&,
                           const ApplicationSnapshot& snapshot,
                           const TuiTheme& theme) {
    WINDOW* window = this->window();
    if (window == nullptr) {
        return;
    }

    werase(window);
    box(window, 0, 0);

    wattron(window, theme.header | A_BOLD);
    mvwaddstr(window, 1, 2, "EVENT LOG");
    wattroff(window, theme.header | A_BOLD);

    const int first_row = 3;
    const int available_rows = std::max(rect().height - first_row - 1, 0);
    const int entry_count = std::min(static_cast<int>(snapshot.events.size()), available_rows);
    const int start = static_cast<int>(snapshot.events.size()) - entry_count;

    for (int row = 0; row < entry_count; ++row) {
        const auto& entry = snapshot.events[static_cast<std::size_t>(start + row)];
        const int y = first_row + row;

        short color = theme.secondary;
        switch (entry.severity) {
            case Severity::Info: color = theme.secondary; break;
            case Severity::Warning: color = theme.warning; break;
            case Severity::Error: color = theme.danger; break;
            case Severity::Success: color = theme.success; break;
        }

        wattron(window, color);
        mvwaddstr(window, y, 3, entry.timestamp.c_str());
        mvwaddstr(window, y, 13, severity_label(entry.severity));
        mvwaddstr(window, y, 21, entry.message.c_str());
        wattroff(window, color);
    }

    wnoutrefresh(window);
}

} // namespace ui
