#include "ui/widgets/actions_panel.hpp"

#include <ncursesw/curses.h>

#include <array>

namespace ui {

void ActionsPanel::render(const TuiState& state,
                          const ApplicationSnapshot&,
                          const TuiTheme& theme) {
    WINDOW* window = this->window();
    if (window == nullptr) {
        return;
    }

    werase(window);
    box(window, 0, 0);

    wattron(window, theme.header | A_BOLD);
    mvwaddstr(window, 1, 2, "ACTIONS");
    wattroff(window, theme.header | A_BOLD);

    const std::array<const char*, 6> actions = {{
        "Launch",
        "Save Settings",
        "Reset Defaults",
        "View Logs",
        "Help",
        "Quit",
    }};

    const int first_row = 3;
    const int max_rows = std::min(static_cast<int>(actions.size()), rect().height - first_row - 1);

    for (int row = 0; row < max_rows; ++row) {
        const int y = first_row + row;
        const bool selected = state.focus_panel == FocusPanel::Actions &&
                              state.selected_action == row;

        if (selected) {
            wattron(window, theme.selected);
            mvwhline(window, y, 1, ' ', rect().width - 2);
            wattroff(window, theme.selected);
        }

        wattron(window, selected ? theme.selected : theme.secondary);
        mvwaddstr(window, y, 2, selected ? "> " : "  ");
        mvwaddstr(window, y, 4, actions[static_cast<std::size_t>(row)]);
        wattroff(window, selected ? theme.selected : theme.secondary);
    }

    wnoutrefresh(window);
}

} // namespace ui
