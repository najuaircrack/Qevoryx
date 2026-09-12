#include "ui/widgets/header.hpp"

#include <ncursesw/curses.h>

#include <array>
#include <string>

namespace ui {
namespace {

const std::array<std::string, 4> logo_rows = {{
    "  \u2584\u2584\u2588\u2584\u2584\u2584 ",
    " \u2584\u2588 \u2580\u2588\u2588\u2588\u2588",
    " \u2580\u2588  \u2584\u2584\u2584\u2588",
    "  \u2580\u2580\u2588\u2588\u2580\u2588\u2584",
}};

} // namespace

void HeaderWidget::render(const TuiState& state,
                          const ApplicationSnapshot& snapshot,
                          const TuiTheme& theme) {
    WINDOW* window = this->window();
    if (window == nullptr) {
        return;
    }

    werase(window);
    box(window, 0, 0);

    const int width = rect().width;
    const int height = rect().height;
    const int logo_x = 3;
    const int logo_y = 1;

    wattron(window, theme.accent);
    for (std::size_t row = 0; row < logo_rows.size(); ++row) {
        mvwaddstr(window, logo_y + static_cast<int>(row), logo_x, logo_rows[row].c_str());
    }
    wattroff(window, theme.accent);

    const int text_x = logo_x + 11;
    const int text_width = std::max(width - text_x - 3, 20);

    wattron(window, theme.header | A_BOLD);
    mvwaddstr(window, 1, text_x, "QEVORYX");
    wattroff(window, theme.header | A_BOLD);

    wattron(window, theme.secondary);
    mvwaddstr(window, 2, text_x, "Terminal Control Panel");
    wattroff(window, theme.secondary);

    wattron(window, theme.muted);
    mvwaddstr(window, 3, text_x, "v4.0.7");
    wattroff(window, theme.muted);

    if (height >= 5) {
        const char* state_text = snapshot.running ? "RUNNING" : "READY";
        const short state_color = snapshot.running ? theme.success : theme.accent;
        const int state_x = std::max(width - 12, text_x + 10);

        wattron(window, state_color | A_BOLD);
        mvwaddstr(window, 1, state_x, state_text);
        wattroff(window, state_color | A_BOLD);

        wattron(window, theme.muted);
        mvwaddstr(window, 2, state_x, "Settings ready");
        wattroff(window, theme.muted);
    }

    if (!state.error_message.empty() && height >= 6) {
        wattron(window, theme.danger);
        mvwaddstr(window, height - 2, 2, state.error_message.substr(0, static_cast<std::size_t>(width - 4)).c_str());
        wattroff(window, theme.danger);
    }

    wnoutrefresh(window);
}

} // namespace ui
