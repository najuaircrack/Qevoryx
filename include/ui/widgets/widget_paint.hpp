#pragma once

#include "ui/tui_theme.hpp"

#include <ncursesw/curses.h>

#include <algorithm>
#include <cstddef>
#include <string>

namespace ui {
namespace paint {

// Padding between a panel border and its content, on each side.
constexpr int kBorderWidth = 1;
constexpr int kPaddingX = 1;

// Left column where content begins, relative to the window origin.
inline int content_x() { return kBorderWidth + kPaddingX; }

// First row available for a title (just inside the top border).
inline int title_y() { return kBorderWidth; }

// First row available for body content, below the title and its blank line.
inline int content_y() { return kBorderWidth + 2; }

// Usable content width inside the borders and padding.
inline int content_width(int rect_width) {
    return std::max(rect_width - 2 * (kBorderWidth + kPaddingX), 0);
}

// Last content row that stays clear of the bottom border.
inline int content_bottom(int rect_height) {
    return rect_height - kBorderWidth - 1;
}

// Draw the box border in the given attribute (muted normally, accent focused).
inline void frame(WINDOW* window, int attr) {
    wattron(window, attr);
    box(window, 0, 0);
    wattroff(window, attr);
}

// Draw a left-aligned panel title at content_x on the title row.
inline void title(WINDOW* window, int attr, const std::string& text, int rect_width) {
    const int room = content_width(rect_width);
    if (room <= 0) {
        return;
    }
    wattron(window, attr);
    mvwaddnstr(window, title_y(), content_x(), text.c_str(), room);
    wattroff(window, attr);
}

// Count UTF-8 codepoints (a good display-width proxy for the single-width
// glyphs this UI uses: ASCII, box-drawing, arrows, bullets, half-blocks).
inline int display_columns(const std::string& value) {
    int columns = 0;
    for (unsigned char byte : value) {
        if ((byte & 0xC0) != 0x80) { // not a UTF-8 continuation byte
            ++columns;
        }
    }
    return columns;
}

// Byte length of the longest prefix of `value` that fits in `max_columns`
// display columns without splitting a codepoint.
inline std::size_t prefix_bytes_for_columns(const std::string& value, int max_columns) {
    if (max_columns <= 0) {
        return 0;
    }
    int columns = 0;
    std::size_t bytes = 0;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if ((static_cast<unsigned char>(value[i]) & 0xC0) != 0x80) {
            if (columns == max_columns) {
                break;
            }
            ++columns;
        }
        bytes = i + 1;
    }
    return bytes;
}

// Write `text` at (y, x), clipped to `max_width` display columns without
// splitting a multi-byte glyph, in `attr`. Returns the x column just past the
// text drawn (for chaining).
inline int text(WINDOW* window, int y, int x, int max_width, int attr,
                const std::string& value) {
    if (max_width <= 0) {
        return x;
    }
    const std::size_t keep = prefix_bytes_for_columns(value, max_width);
    if (keep == 0) {
        return x;
    }
    const std::string clipped = value.substr(0, keep);
    wattron(window, attr);
    mvwaddnstr(window, y, x, clipped.c_str(), static_cast<int>(keep));
    wattroff(window, attr);
    return x + display_columns(clipped);
}

// Fill one content row with spaces in `attr` (used for selection highlight),
// staying strictly inside the left and right borders.
inline void fill_row(WINDOW* window, int y, int rect_width, int attr) {
    const int inner_width = std::max(rect_width - 2 * kBorderWidth, 0);
    if (inner_width <= 0) {
        return;
    }
    wattron(window, attr);
    mvwhline(window, y, kBorderWidth, ' ', inner_width);
    wattroff(window, attr);
}

} // namespace paint
} // namespace ui
