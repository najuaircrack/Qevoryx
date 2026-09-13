#include "ui/widgets/help_panel.hpp"

#include "ui/widgets/widget_paint.hpp"

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

    const int width = rect().width;
    const int height = rect().height;

    werase(window);
    paint::frame(window, theme.border);
    paint::title(window, theme.header | A_BOLD, "HELP", width);

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

    const int content_x = paint::content_x() + 1;
    const int content_w = paint::content_width(width);
    const int first_row = paint::content_y();
    const int last_row = paint::content_bottom(height);
    const int max_rows = std::min(static_cast<int>(rows.size()),
                                  std::max(last_row - first_row + 1, 0));

    const int desc_x = content_x + 13;
    const int desc_w = std::max(content_x + content_w - desc_x, 0);

    for (int row = 0; row < max_rows; ++row) {
        const int y = first_row + row;
        const bool heading = rows[static_cast<std::size_t>(row)].second[0] == '\0';

        if (heading) {
            paint::text(window, y, content_x, content_w, theme.accent | A_BOLD,
                        rows[static_cast<std::size_t>(row)].first);
        } else {
            paint::text(window, y, content_x, 12, theme.primary | A_BOLD,
                        rows[static_cast<std::size_t>(row)].first);
            paint::text(window, y, desc_x, desc_w, theme.secondary,
                        rows[static_cast<std::size_t>(row)].second);
        }
    }

    wnoutrefresh(window);
}

} // namespace ui
