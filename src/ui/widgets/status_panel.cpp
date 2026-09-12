#include "ui/widgets/status_panel.hpp"

#include <ncursesw/curses.h>

#include <algorithm>
#include <string>

namespace ui {

void StatusPanel::render(const TuiState& state,
                         const ApplicationSnapshot& snapshot,
                         const TuiTheme& theme) {
    WINDOW* window = this->window();
    if (window == nullptr) {
        return;
    }

    werase(window);
    box(window, 0, 0);

    wattron(window, theme.header | A_BOLD);
    mvwaddstr(window, 1, 2, "STATUS");
    wattroff(window, theme.header | A_BOLD);

    const int first_row = 3;
    const int column_width = std::max(rect().width / 3, 1);
    const char* bullet = theme.unicode_available ? "\xE2\x97\x8F" : "*";

    wattron(window, theme.success);
    mvwaddstr(window, first_row, 3, (std::string(bullet) + " Configuration loaded").c_str());
    wattroff(window, theme.success);

    wattron(window, theme.success);
    mvwaddstr(window, first_row, 3 + column_width, (std::string(bullet) + " Settings ready").c_str());
    wattroff(window, theme.success);

    wattron(window, snapshot.running ? theme.success : theme.accent);
    mvwaddstr(window, first_row, 3 + column_width * 2,
              (std::string(bullet) + (snapshot.running ? " Running" : " Ready")).c_str());
    wattroff(window, snapshot.running ? theme.success : theme.accent);

    if (snapshot.config.payload_min == snapshot.config.payload_max) {
        wattron(window, theme.warning);
        mvwaddstr(window, first_row + 1, 3,
                  ("! Payload range is fixed (" + std::to_string(snapshot.config.payload_min) + " - " +
                   std::to_string(snapshot.config.payload_max) + ")").c_str());
        wattroff(window, theme.warning);
    }

    if (!state.error_message.empty()) {
        wattron(window, theme.danger);
        mvwaddstr(window, first_row + 1, 3 + column_width, state.error_message.c_str());
        wattroff(window, theme.danger);
    }

    wnoutrefresh(window);
}

} // namespace ui
