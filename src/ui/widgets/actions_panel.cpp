#include "ui/widgets/actions_panel.hpp"

#include "ui/widgets/selectable_list.hpp"

#include <ncursesw/curses.h>

#include <string>
#include <vector>

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

    const std::vector<std::string> actions = {
        "Launch",
        "Save Settings",
        "Reset Defaults",
        "View Logs",
        "Help",
        "Quit",
    };

    const int first_row = 3;
    const int max_rows = std::max(std::min(static_cast<int>(actions.size()), rect().height - first_row - 1), 0);
    const SelectableList list(actions, state.selected_action);
    list.render(window, {2, first_row, std::max(rect().width - 4, 0), max_rows}, theme,
                state.focus_panel == FocusPanel::Actions);

    wnoutrefresh(window);
}

} // namespace ui
