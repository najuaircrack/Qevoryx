#include "ui/logo.hpp"

#include <ncursesw/curses.h>

namespace ui {
namespace {

const char* glyph_string(unsigned char code, bool unicode) {
    if (unicode) {
        switch (code) {
            case 1: return "\xE2\x96\x80"; // upper half block U+2580
            case 2: return "\xE2\x96\x84"; // lower half block U+2584
            case 3: return "\xE2\x96\x88"; // full block       U+2588
            default: return " ";
        }
    }
    // ASCII fallback for terminals without UTF-8.
    return code == 0 ? " " : "#";
}

short logo_pair(const TuiTheme& theme, logo::Tone fg, logo::Tone bg) {
    if (fg == logo::L && bg == logo::R) return theme.logo_light_on_red;
    if (fg == logo::R && bg == logo::L) return theme.logo_red_on_light;
    if (fg == logo::R) return theme.logo_red;
    return theme.logo_light;
}

} // namespace

void draw_logo(WINDOW* win, int top, int left,
               const logo::Art& art, const TuiTheme& theme) {
    if (win == nullptr || art.cells == nullptr) {
        return;
    }

    for (int y = 0; y < art.height; ++y) {
        for (int x = 0; x < art.width; ++x) {
            const logo::Cell& cell = art.cells[y * art.width + x];
            if (cell.glyph == 0) {
                continue; // transparent: leave the background untouched
            }
            const int attr = logo_pair(theme, cell.fg, cell.bg) | A_BOLD;
            wattron(win, attr);
            mvwaddstr(win, top + y, left + x, glyph_string(cell.glyph, theme.unicode_available));
            wattroff(win, attr);
        }
    }
}

} // namespace ui
