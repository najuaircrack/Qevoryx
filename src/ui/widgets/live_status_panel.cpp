#include "ui/widgets/live_status_panel.hpp"

#include "ui/widgets/widget_paint.hpp"

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

    const int width = rect().width;
    const int height = rect().height;

    werase(window);
    paint::frame(window, theme.border);
    paint::title(window, theme.header | A_BOLD, "LIVE STATUS", width);

    const int content_x = paint::content_x() + 1;
    const int content_w = paint::content_width(width);
    const int first_row = paint::content_y();
    const int last_row = paint::content_bottom(height);
    const int value_x = content_x + 15;
    const int value_w = std::max(content_x + content_w - value_x, 0);

    const char* dot = theme.unicode_available ? "\xE2\x97\x8F" : "*"; // U+25CF
    const int state_color = snapshot.paused ? theme.warning :
                            snapshot.running ? theme.success : theme.warning;
    const char* state_text = snapshot.paused ? "PAUSED" :
                             snapshot.running ? "RUNNING" : "STOPPED";

    if (first_row <= last_row) {
        int cx = paint::text(window, first_row, content_x, content_w, state_color | A_BOLD, dot);
        paint::text(window, first_row, cx + 1, std::max(content_x + content_w - (cx + 1), 0),
                    state_color | A_BOLD, state_text);
    }
    if (first_row + 1 <= last_row) {
        paint::text(window, first_row + 1, content_x, 15, theme.secondary, "Generated");
        paint::text(window, first_row + 1, value_x, value_w, theme.primary | A_BOLD,
                    std::to_string(snapshot.generated));
    }
    if (first_row + 2 <= last_row) {
        paint::text(window, first_row + 2, content_x, 15, theme.secondary, "Errors");
        paint::text(window, first_row + 2, value_x, value_w,
                    (snapshot.errors > 0 ? theme.danger : theme.primary) | A_BOLD,
                    std::to_string(snapshot.errors));
    }

    wnoutrefresh(window);
}

} // namespace ui
