#pragma once

#include "ui/tui_theme.hpp"

#include <ncursesw/curses.h>

namespace ui {
namespace logo {

// A tone is the quantized color of half a terminal cell.
enum Tone : unsigned char {
    N = 0, // none / transparent (terminal background shows through)
    L = 1, // light  (the white dragon/"Q" body)
    R = 2  // red    (claw slashes and eye accents)
};

// One terminal cell of the logo. `glyph` selects the block character and `fg`
// / `bg` the tone of its upper and lower halves:
//   glyph 0 -> ' '            (empty; nothing drawn)
//   glyph 1 -> upper half     (fg = top tone, bg = bottom tone)
//   glyph 2 -> lower half     (fg = bottom tone)
//   glyph 3 -> full block     (fg = single tone)
struct Cell {
    unsigned char glyph;
    Tone fg;
    Tone bg;
};

// A block of logo art. `cells` points at `width * height` cells, row-major.
struct Art {
    int width;
    int height;
    const Cell* cells;
};

} // namespace logo

// Draw logo art into `win` with its top-left cell at (top, left). Empty cells
// are skipped so whatever is behind the logo shows through the gaps.
void draw_logo(WINDOW* win, int top, int left,
               const logo::Art& art, const TuiTheme& theme);

} // namespace ui
