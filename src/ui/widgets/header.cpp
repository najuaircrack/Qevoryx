#include "ui/widgets/header.hpp"

#include "common/constants.hpp"
#include "ui/logo.hpp"
#include "ui/logo_data.hpp"

#include <ncursesw/curses.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string>

namespace ui {

namespace {

void draw_clipped(WINDOW* window, int y, int x, int attr, const std::string& text,
                  int max_x) {
    const int available = max_x - x;
    if (available <= 0) {
        return;
    }
    wattron(window, attr);
    mvwaddstr(window, y, x, text.substr(0, static_cast<std::size_t>(available)).c_str());
    wattroff(window, attr);
}

// Largest emblem whose square fits the header interior beside the wordmark, or
// nullptr when even the smallest will not fit.
const logo::Art* pick_emblem(int interior_height, int width, int reserve) {
    for (const logo::Art& art : logo::kEmblemSizes) {
        if (art.height <= interior_height && art.width + reserve <= width) {
            return &art;
        }
    }
    return nullptr;
}

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
    const int interior_height = height - 2; // inside the top/bottom border

    // Reserve room for the wordmark (~26 cols) plus the right-hand status block.
    const logo::Art* emblem = interior_height >= 2 ? pick_emblem(interior_height, width, 30)
                                                   : nullptr;

    int text_x = 3;
    if (emblem != nullptr) {
        const int emblem_x = 2;
        const int emblem_y = std::max((height - emblem->height) / 2, 1);
        draw_logo(window, emblem_y, emblem_x, *emblem, theme);
        text_x = emblem_x + emblem->width + 3;
    }

    // Reserve space on the right for the run state / settings path.
    const int state_x = std::max(width - 32, text_x + 12);
    const int text_right = std::min(state_x - 1, width - 2);

    const int lines = height >= 5 ? 3 : (height >= 4 ? 2 : 1);
    const int text_top = std::max((height - lines) / 2, 1);

    draw_clipped(window, text_top, text_x, theme.header | A_BOLD, "QEVORYX", text_right);
    if (lines >= 2) {
        draw_clipped(window, text_top + 1, text_x, theme.secondary,
                     "Terminal Control Panel", text_right);
    }
    if (lines >= 3) {
        draw_clipped(window, text_top + 2, text_x, theme.muted,
                     std::string("v") + common::VERSION, text_right);
    }

    // Run state and settings path, right-aligned.
    if (width >= text_x + 24) {
        const char* state_text = snapshot.paused ? "PAUSED" :
                                 snapshot.running ? "RUNNING" : "READY";
        const int state_color = snapshot.paused ? theme.warning :
                                snapshot.running ? theme.success : theme.accent;

        wattron(window, state_color | A_BOLD);
        mvwaddstr(window, text_top, state_x, state_text);
        wattroff(window, state_color | A_BOLD);

        if (lines >= 2) {
            std::string settings_path = snapshot.settings_path;
            const char* home = std::getenv("HOME");
            if (home != nullptr && settings_path.rfind(home, 0) == 0) {
                settings_path = "~" + settings_path.substr(std::strlen(home));
            }
            const int available_width = std::max(width - state_x - 2, 0);
            wattron(window, theme.muted);
            mvwaddstr(window, text_top + 1, state_x,
                      settings_path.substr(0, static_cast<std::size_t>(available_width)).c_str());
            wattroff(window, theme.muted);
        }
    }

    wnoutrefresh(window);
}

} // namespace ui
