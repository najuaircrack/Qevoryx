#include "ui/widgets/footer.hpp"

#include "ui/widgets/widget_paint.hpp"

#include <ncursesw/curses.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace ui {

void FooterWidget::render(const TuiState& state,
                          const ApplicationSnapshot&,
                          const TuiTheme& theme) {
    WINDOW* window = this->window();
    if (window == nullptr) {
        return;
    }

    werase(window);
    paint::frame(window, theme.border);

    const int width = rect().width;
    const int content_x = paint::content_x() + 1;
    const int content_w = std::max(width - 2 * content_x, 0);
    if (content_w <= 0) {
        wnoutrefresh(window);
        return;
    }

    const char* up = theme.unicode_available ? "\xE2\x86\x91" : "^";
    const char* down = theme.unicode_available ? "\xE2\x86\x93" : "v";
    const char* left = theme.unicode_available ? "\xE2\x86\x90" : "<";
    const char* right = theme.unicode_available ? "\xE2\x86\x92" : ">";

    // Each hint is a (keys, label) pair; keys draw in accent, labels in muted.
    std::vector<std::pair<std::string, std::string>> hints;
    if (state.input_mode == InputMode::Editing) {
        hints = {
            {"Enter", "Confirm"}, {"Esc", "Cancel"},
            {std::string(left) + right, "Move"}, {"Bksp", "Delete"},
        };
    } else if (state.screen == TuiScreen::Runtime) {
        hints = {
            {"S", "Stop"},
            {"P", state.runtime_paused ? "Resume" : "Pause"},
            {"R", "Return"}, {"Q", "Quit"},
        };
    } else {
        hints = {
            {std::string(up) + down, "Move"},
            {std::string(left) + right, "Change"},
            {"Enter", "Edit"}, {"Tab", "Panel"},
            {"L", "Launch"}, {"S", "Save"},
            {"D", "Defaults"}, {"?", "Help"}, {"Q", "Quit"},
        };
    }

    // Lay out segments left to right, dropping any that would cross the border.
    // Separator is three spaces for clear grouping.
    const int y = 1;
    int x = content_x;
    const int right_edge = content_x + content_w;
    for (std::size_t i = 0; i < hints.size(); ++i) {
        const std::string& keys = hints[i].first;
        const std::string& label = hints[i].second;
        const int seg_len = static_cast<int>(keys.size()) + 1 + static_cast<int>(label.size());
        const int sep = (i == 0) ? 0 : 3;
        if (x + sep + seg_len > right_edge) {
            break; // no room; stop cleanly rather than overflow
        }
        x += sep;
        x = paint::text(window, y, x, right_edge - x, theme.accent | A_BOLD, keys);
        x = paint::text(window, y, x + 1, right_edge - (x + 1), theme.muted, label);
    }

    wnoutrefresh(window);
}

} // namespace ui
