#include "ui/widgets/actions_panel.hpp"

#include "ui/widgets/selectable_list.hpp"
#include "ui/widgets/widget_paint.hpp"

#include <ncursesw/curses.h>

#include <algorithm>
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

    const int width = rect().width;
    const int height = rect().height;
    const bool focused = state.focus_panel == FocusPanel::Actions;

    werase(window);
    paint::frame(window, focused ? theme.border_focused : theme.border);
    paint::title(window, (focused ? theme.accent : theme.header) | A_BOLD, "ACTIONS", width);

    const std::vector<std::string> actions = {
        "Launch",
        "Save Settings",
        "Reset Defaults",
        "View Logs",
        "Help",
        "Quit",
    };

    const int first_row = paint::content_y();
    const int last_row = paint::content_bottom(height);
    const int available_rows = std::max(last_row - first_row + 1, 0);
    const int max_rows = std::min(static_cast<int>(actions.size()), available_rows);
    const int content_x = paint::content_x();
    const int content_w = paint::content_width(width);

    const SelectableList list(actions, state.selected_action);
    list.render(window, {content_x, first_row, content_w, max_rows}, theme, focused);

    wnoutrefresh(window);
}

} // namespace ui
