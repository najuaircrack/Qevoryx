#pragma once

#include "ui/tui_layout.hpp"
#include "ui/tui_state.hpp"
#include "ui/tui_theme.hpp"

#include <ncursesw/curses.h>
#include <string>
#include <vector>

namespace ui {

class SelectableList {
public:
    SelectableList(std::vector<std::string> items, int selected);

    void render(WINDOW* window,
                const Rect& rect,
                const TuiTheme& theme,
                bool focused) const;

private:
    std::vector<std::string> items_;
    int selected_;
};

} // namespace ui
