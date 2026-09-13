#include "ui/tui_theme.hpp"

#include <ncursesw/curses.h>

#include <algorithm>
#include <cctype>
#include <clocale>
#include <cstring>
#include <string>

namespace ui {

void TuiTheme::initialize() {
    const char* locale_name = setlocale(LC_ALL, nullptr);
    std::string normalized;
    if (locale_name != nullptr) {
        normalized = locale_name;
        std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                       [](unsigned char character) {
                           return static_cast<char>(std::tolower(character));
                       });
    }
    unicode_available = normalized.find("utf-8") != std::string::npos ||
                        normalized.find("utf8") != std::string::npos;

    colors_available = has_colors();
    if (!colors_available) {
        return;
    }

    start_color();
    use_default_colors();

    // Prefer exact brand colors when the terminal can redefine its palette;
    // otherwise fall back to the standard 8-color names.
    truecolor_available = can_change_color() && COLORS >= 256;

    constexpr short kBrandLight = 16;
    constexpr short kBrandRed = 17;
    constexpr short kBrandAccent = 18;

    short light_color = COLOR_WHITE;
    short red_color = COLOR_RED;
    short accent_color = COLOR_CYAN;
    if (truecolor_available) {
        const auto scale = [](int value) -> short {
            return static_cast<short>(value * 1000 / 255);
        };
        init_color(kBrandLight, scale(243), scale(243), scale(246));
        init_color(kBrandRed, scale(226), scale(32), scale(42));
        init_color(kBrandAccent, scale(17), scale(168), scale(205));
        light_color = kBrandLight;
        red_color = kBrandRed;
        accent_color = kBrandAccent;
    }

    init_pair(1, accent_color, -1);
    init_pair(2, light_color, -1);
    init_pair(3, light_color, -1);
    init_pair(4, COLOR_BLACK, -1);
    init_pair(5, accent_color, -1);
    init_pair(6, COLOR_GREEN, -1);
    init_pair(7, COLOR_YELLOW, -1);
    init_pair(8, red_color, -1);
    init_pair(9, COLOR_BLACK, accent_color);
    init_pair(10, accent_color, -1);

    // Logo pairs: white body, red accents, and the two vertical boundaries.
    init_pair(11, light_color, -1);
    init_pair(12, red_color, -1);
    init_pair(13, light_color, red_color);
    init_pair(14, red_color, light_color);

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
    logo_light = static_cast<short>(COLOR_PAIR(11));
    logo_red = static_cast<short>(COLOR_PAIR(12));
    logo_light_on_red = static_cast<short>(COLOR_PAIR(13));
    logo_red_on_light = static_cast<short>(COLOR_PAIR(14));
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
