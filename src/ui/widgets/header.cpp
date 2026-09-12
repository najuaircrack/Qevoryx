#include "ui/widgets/header.hpp"

#include <ncursesw/curses.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string>

namespace ui {
namespace {

const std::array<std::string, 4> unicode_logo_rows = {{
    "  \u2584\u2584\u2588\u2584\u2584\u2584 ",
    " \u2584\u2588 \u2580\u2588\u2588\u2588\u2588",
    " \u2580\u2588  \u2584\u2584\u2584\u2588",
    "  \u2580\u2580\u2588\u2588\u2580\u2588\u2584",
}};

const std::array<std::string, 4> ascii_logo_rows = {{
    "  ___  ",
    " / _ \\",
    "| |_| |",
    " \\___/ ",
}};

} // namespace

void HeaderWidget::render(const TuiState&,
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
    const auto& logo_rows = theme.unicode_available ? unicode_logo_rows : ascii_logo_rows;

    wattron(window, theme.accent);
    for (std::size_t row = 0; row < logo_rows.size(); ++row) {
        mvwaddstr(window, logo_y + static_cast<int>(row), logo_x, logo_rows[row].c_str());
    }
    wattroff(window, theme.accent);

    const int text_x = logo_x + 11;
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
        const char* state_text = snapshot.paused ? "PAUSED" :
                                 snapshot.running ? "RUNNING" : "READY";
        const short state_color = snapshot.paused ? theme.warning :
                                  snapshot.running ? theme.success : theme.accent;
        const int state_x = std::max(width - 12, text_x + 10);

        wattron(window, state_color | A_BOLD);
        mvwaddstr(window, 1, state_x, state_text);
        wattroff(window, state_color | A_BOLD);

        wattron(window, theme.muted);
        std::string settings_path = snapshot.settings_path;
        const char* home = std::getenv("HOME");
        if (home != nullptr && settings_path.rfind(home, 0) == 0) {
            settings_path = "~" + settings_path.substr(std::strlen(home));
        }
        const int available_width = std::max(width - state_x - 2, 0);
        mvwaddstr(window, 2, state_x,
                  settings_path.substr(0, static_cast<std::size_t>(available_width)).c_str());
        wattroff(window, theme.muted);
    }

    wnoutrefresh(window);
}

} // namespace ui
