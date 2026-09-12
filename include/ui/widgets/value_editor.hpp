#pragma once

#include "config/config.hpp"
#include "ui/tui_layout.hpp"
#include "ui/tui_state.hpp"
#include "ui/tui_theme.hpp"

#include <ncursesw/curses.h>
#include <string>

namespace ui {

class ValueEditor {
public:
    enum class Mode {
        String,
        Integer,
        Boolean,
        Enum
    };

    ValueEditor(Mode mode, std::string value);

    void render(WINDOW* window,
                const Rect& rect,
                const TuiTheme& theme,
                bool selected,
                bool editing) const;

private:
    Mode mode_;
    std::string value_;
};

} // namespace ui
