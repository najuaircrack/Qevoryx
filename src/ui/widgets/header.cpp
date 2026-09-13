#include "ui/widgets/header.hpp"

#include "common/constants.hpp"
#include "ui/logo.hpp"
#include "ui/logo_data.hpp"
#include "ui/widgets/widget_paint.hpp"

#include <ncursesw/curses.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string>

namespace ui {

namespace {

// Largest emblem whose square fits the header interior beside the wordmark.
const logo::Art* pick_emblem(int interior_height, int width, int reserve) {
    for (const logo::Art& art : logo::kEmblemSizes) {
        if (art.height <= interior_height && art.width + reserve <= width) {
            return &art;
        }
    }
    return nullptr;
}

// Shorten a path from the left with a leading ellipsis so the filename stays
// visible, e.g. "~/.config/qevoryx/settings.ini" -> "…/qevoryx/settings.ini".
std::string shorten_path(const std::string& path, int max_width, bool unicode) {
    if (max_width <= 0 || static_cast<int>(path.size()) <= max_width) {
        return path.substr(0, std::max(max_width, 0));
    }
    const std::string ellipsis = unicode ? "\xE2\x80\xA6" : "..."; // U+2026
    const int keep = max_width - static_cast<int>(ellipsis.size());
    if (keep <= 0) {
        return ellipsis.substr(0, static_cast<std::size_t>(max_width));
    }
    // Prefer to cut at a path separator for a clean break.
    std::string tail = path.substr(path.size() - static_cast<std::size_t>(keep));
    const std::size_t slash = tail.find('/');
    if (slash != std::string::npos && slash + 1 < tail.size()) {
        tail = tail.substr(slash);
    }
    return ellipsis + tail;
}

} // namespace

void HeaderWidget::render(const TuiState&,
                          const ApplicationSnapshot& snapshot,
                          const TuiTheme& theme) {
    WINDOW* window = this->window();
    if (window == nullptr) {
        return;
    }

    const int width = rect().width;
    const int height = rect().height;
    const int interior_height = height - 2;

    werase(window);
    paint::frame(window, theme.border);

    // --- Right-hand status column: divider + state + path -------------------
    // Reserve a fixed-width status block on the right and draw a divider before
    // it, matching the brand header mock.
    const int status_w = std::min(std::max(width / 3, 24), 40);
    const bool show_status = width >= 50;
    const int status_x = show_status ? width - status_w : width;
    const int divider_x = status_x - 2;

    // --- Left block: emblem + wordmark lockup -------------------------------
    const int reserve = (show_status ? width - divider_x + 4 : 4) + 26;
    const logo::Art* emblem =
        interior_height >= 2 ? pick_emblem(interior_height, width, reserve) : nullptr;

    int text_x = paint::content_x() + 1;
    if (emblem != nullptr) {
        const int emblem_x = 2;
        const int emblem_y = std::max((height - emblem->height) / 2, 1);
        draw_logo(window, emblem_y, emblem_x, *emblem, theme);
        text_x = emblem_x + emblem->width + 3;
    }

    const int lines = height >= 5 ? 3 : (height >= 4 ? 2 : 1);
    const int text_top = std::max((height - lines) / 2, 1);
    const int text_right = (show_status ? divider_x : width) - 1;
    const int text_room = std::max(text_right - text_x, 0);

    // Wordmark: "QEVORY" primary + "X" red brand accent (used sparingly).
    if (text_room >= 7) {
        wattron(window, theme.header | A_BOLD);
        mvwaddnstr(window, text_top, text_x, "QEVORY", 6);
        wattroff(window, theme.header | A_BOLD);
        wattron(window, theme.logo_red | A_BOLD);
        mvwaddnstr(window, text_top, text_x + 6, "X", 1);
        wattroff(window, theme.logo_red | A_BOLD);
    } else {
        paint::text(window, text_top, text_x, text_room, theme.header | A_BOLD, "QEVORYX");
    }

    if (lines >= 2) {
        paint::text(window, text_top + 1, text_x, text_room, theme.secondary,
                    "Terminal Control Panel");
    }
    if (lines >= 3) {
        paint::text(window, text_top + 2, text_x, text_room, theme.muted,
                    std::string("v") + common::VERSION);
    }

    // --- Status column ------------------------------------------------------
    if (show_status) {
        // Vertical divider between the wordmark and the status column.
        wattron(window, theme.border);
        for (int y = 1; y < height - 1; ++y) {
            mvwaddnstr(window, y, divider_x, theme.unicode_available ? "\xE2\x94\x82" : "|", 1);
        }
        wattroff(window, theme.border);

        const char* state_text = snapshot.paused ? "PAUSED" :
                                 snapshot.running ? "RUNNING" : "READY";
        const int state_color = snapshot.paused ? theme.warning :
                                snapshot.running ? theme.success : theme.success;
        const char* dot = theme.unicode_available ? "\xE2\x97\x8F" : "*"; // U+25CF

        const int state_room = std::max(width - 1 - status_x, 0);
        int cx = paint::text(window, text_top, status_x, state_room, state_color | A_BOLD, dot);
        paint::text(window, text_top, cx + 1, std::max(width - 1 - (cx + 1), 0),
                    state_color | A_BOLD, state_text);

        if (lines >= 2) {
            std::string settings_path = snapshot.settings_path;
            const char* home = std::getenv("HOME");
            if (home != nullptr && settings_path.rfind(home, 0) == 0) {
                settings_path = "~" + settings_path.substr(std::strlen(home));
            }
            const int path_room = std::max(width - 1 - status_x, 0);
            paint::text(window, text_top + 1, status_x, path_room, theme.muted,
                        shorten_path(settings_path, path_room, theme.unicode_available));
        }
    }

    wnoutrefresh(window);
}

} // namespace ui
