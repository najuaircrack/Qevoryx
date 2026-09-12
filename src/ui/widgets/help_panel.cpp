#include "ui/widgets/help_panel.hpp"

#include <ncursesw/curses.h>

#include <array>
#include <utility>

namespace ui {

void HelpPanel::render(const TuiState&,
                       const ApplicationSnapshot&,
                       const TuiTheme& theme) {
    WINDOW* window = this->window();
    if (window == nullptr) {
        return;
    }

    werase(window);
    box(window, 0, 0);

    wattron(window, theme.header | A_BOLD);
    mvwaddstr(window, 1, 3, "HELP");
    wattroff(window, theme.header | A_BOLD);

    const std::array<std::pair<const char*, const char*>, 17> rows = {{
        {"Navigation", ""},
        {"Up Down", "Move within a panel"},
        {"Left Right", "Change a value or choice"},
        {"Tab", "Switch panels"},
        {"Editing", ""},
        {"Enter", "Edit a value or activate an action"},
        {"Esc", "Cancel editing or close a dialog"},
        {"Backspace", "Delete the character before the cursor"},
        {"Actions", ""},
        {"L", "Launch the configured test"},
        {"S", "Save settings"},
        {"D", "Restore safe defaults"},
        {"?", "Show this help"},
        {"Q", "Quit Qevoryx"},
        {"Runtime", ""},
        {"P", "Pause or resume"},
        {"R", "Return to the configuration panel"},
    }};

    const int first_row = 3;
    const int max_rows = std::min(static_cast<int>(rows.size()), rect().height - first_row - 1);

    for (int row = 0; row < max_rows; ++row) {
        const int y = first_row + row;
        const bool heading = rows[static_cast<std::size_t>(row)].second[0] == '\0';

        wattron(window, heading ? theme.header | A_BOLD : theme.secondary);
        mvwaddstr(window, y, 4, rows[static_cast<std::size_t>(row)].first);
        if (!heading) {
            mvwaddstr(window, y, 16, rows[static_cast<std::size_t>(row)].second);
        }
        wattroff(window, heading ? theme.header | A_BOLD : theme.secondary);
    }

    wnoutrefresh(window);
}

} // namespace ui
