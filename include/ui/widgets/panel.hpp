#pragma once

#include "ui/tui_layout.hpp"
#include "ui/tui_state.hpp"
#include "ui/tui_theme.hpp"

#include <ncursesw/curses.h>
#include <ncursesw/panel.h>

namespace ui {

class Panel {
public:
    Panel() = default;
    virtual ~Panel();

    Panel(const Panel&) = delete;
    Panel& operator=(const Panel&) = delete;

    void resize(const Rect& rect);
    void show();
    void hide();

    virtual void render(const TuiState& state,
                        const ApplicationSnapshot& snapshot,
                        const TuiTheme& theme) = 0;

protected:
    WINDOW* window() const;
    const Rect& rect() const;

private:
    WINDOW* window_{nullptr};
    PANEL* panel_{nullptr};
    Rect rect_;
};

} // namespace ui
