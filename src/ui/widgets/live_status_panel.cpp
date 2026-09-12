#include "ui/widgets/live_status_panel.hpp"

#include <ncursesw/curses.h>

#include <string>

namespace ui {

void LiveStatusPanel::render(const TuiState&,
                             const ApplicationSnapshot& snapshot,
                             const TuiTheme& theme) {
    WINDOW* window = this->window();
    if (window == nullptr) {
        return;
    }

    werase(window);
    box(window, 0, 0);

    wattron(window, theme.header | A_BOLD);
    mvwaddstr(window, 1, 2, "LIVE STATUS");
    wattroff(window, theme.header | A_BOLD);

    const int first_row = 3;
    const short state_color = snapshot.running ? theme.success : theme.warning;
    const char* state_text = snapshot.running ? "RUNNING" : "STOPPED";

    wattron(window, state_color | A_BOLD);
    mvwaddstr(window, first_row, 3, state_text);
    wattroff(window, state_color | A_BOLD);

    wattron(window, theme.secondary);
    mvwaddstr(window, first_row + 1, 3, "Generated");
    mvwaddstr(window, first_row + 2, 3, "Errors");
    wattroff(window, theme.secondary);

    wattron(window, theme.primary | A_BOLD);
    mvwaddstr(window, first_row + 1, 18, std::to_string(snapshot.generated).c_str());
    mvwaddstr(window, first_row + 2, 18, std::to_string(snapshot.errors).c_str());
    wattroff(window, theme.primary | A_BOLD);

    wnoutrefresh(window);
}

} // namespace ui
