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
        // No color support at all (a genuinely monochrome terminal). Pairs stay
        // 0, so text and the logo still render as the terminal's default
        // foreground; bold in the widgets keeps them emphasized.
        return;
    }

    start_color();
    use_default_colors();

    // Always render in color. When the terminal can redefine its palette we use
    // the exact brand colors; otherwise we fall back to the standard 8 ANSI
    // color names, which every color terminal (xterm, MSYS2/Windows, the Linux
    // console) supports. Bold brightens them on 8/16-color terminals.
    truecolor_available = can_change_color() && COLORS >= 256;

    constexpr short kBrandLight = 16;
    constexpr short kBrandRed = 17;
    constexpr short kBrandAccent = 18;
    constexpr short kBrandMuted = 19;
    constexpr short kBrandSecondary = 20;

    short light_color = COLOR_WHITE;
    short red_color = COLOR_RED;
    short accent_color = COLOR_CYAN;
    short muted_color = COLOR_WHITE;    // 8-color: no real gray, rely on A_DIM
    short secondary_color = COLOR_WHITE;
    if (truecolor_available) {
        const auto scale = [](int value) -> short {
            return static_cast<short>(value * 1000 / 255);
        };
        init_color(kBrandLight, scale(243), scale(243), scale(246));
        init_color(kBrandRed, scale(226), scale(32), scale(42));
        init_color(kBrandAccent, scale(56), scale(196), scale(222));
        init_color(kBrandMuted, scale(118), scale(124), scale(138));    // gray
        init_color(kBrandSecondary, scale(178), scale(184), scale(196)); // light gray
        light_color = kBrandLight;
        red_color = kBrandRed;
        accent_color = kBrandAccent;
        muted_color = kBrandMuted;
        secondary_color = kBrandSecondary;
    }

    // Semantic pairs. Foreground on the terminal's default background (-1).
    init_pair(1, muted_color, -1);      // Border (normal, muted)
    init_pair(2, light_color, -1);      // Primary (bright)
    init_pair(3, secondary_color, -1);  // Secondary (light gray)
    init_pair(4, muted_color, -1);      // Muted (gray)
    init_pair(5, accent_color, -1);     // Accent (cyan)
    init_pair(6, COLOR_GREEN, -1);      // Success
    init_pair(7, COLOR_YELLOW, -1);     // Warning
    init_pair(8, red_color, -1);        // Danger
    init_pair(9, COLOR_BLACK, accent_color); // Selection (dark on cyan)
    init_pair(10, light_color, -1);     // Header wordmark (bright white)
    init_pair(15, accent_color, -1);    // BorderFocused (cyan)

    // Logo pairs: white body, red accents, and the two vertical boundaries.
    init_pair(11, light_color, -1);
    init_pair(12, red_color, -1);
    init_pair(13, light_color, red_color);
    init_pair(14, red_color, light_color);

    border = static_cast<short>(COLOR_PAIR(1));
    border_focused = static_cast<short>(COLOR_PAIR(15));
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
        case TuiColorRole::BorderFocused: return border_focused;
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
