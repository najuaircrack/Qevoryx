#include "ui/tui_theme.hpp"

#include <ncursesw/curses.h>

namespace ui {

void TuiTheme::initialize() {
    colors_available = has_colors();
    if (!colors_available) {
        return;
    }

    start_color();
    use_default_colors();

    init_pair(1, COLOR_CYAN, -1);
    init_pair(2, COLOR_WHITE, -1);
    init_pair(3, COLOR_WHITE, -1);
    init_pair(4, COLOR_BLACK, -1);
    init_pair(5, COLOR_CYAN, -1);
    init_pair(6, COLOR_GREEN, -1);
    init_pair(7, COLOR_YELLOW, -1);
    init_pair(8, COLOR_RED, -1);
    init_pair(9, COLOR_BLACK, COLOR_CYAN);
    init_pair(10, COLOR_CYAN, -1);

    border = static_cast<short>(COLOR_PAIR(1));
    primary = static_cast<short>(COLOR_PAIR(2));
    secondary = static_cast<short>(COLOR_PAIR(3));
    muted = static_cast<short>(COLOR_PAIR(4));
    accent = static_cast<short>(COLOR_PAIR(5));
    success = static_cast<short>(COLOR_PAIR(6));
    warning = static_cast<short>(COLOR_PAIR(7));
    danger = static_cast<short>(COLOR_PAIR(8));
    selected = static_cast<short>(COLOR_PAIR(9));
    header = static_cast<short>(COLOR_PAIR(10));
}

short TuiTheme::pair(TuiColorRole role) const {
    switch (role) {
        case TuiColorRole::Border: return border;
        case TuiColorRole::Primary: return primary;
        case TuiColorRole::Secondary: return secondary;
        case TuiColorRole::Muted: return muted;
        case TuiColorRole::Accent: return accent;
        case TuiColorRole::Success: return success;
        case TuiColorRole::Warning: return warning;
        case TuiColorRole::Danger: return danger;
        case TuiColorRole::Selected: return selected;
        case TuiColorRole::Header: return header;
    }
    return primary;
}

} // namespace ui
