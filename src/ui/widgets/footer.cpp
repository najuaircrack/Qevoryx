#include "ui/widgets/footer.hpp"

#include <ncursesw/curses.h>

#include <string>

namespace ui {

void FooterWidget::render(const TuiState& state,
                          const ApplicationSnapshot& snapshot,
                          const TuiTheme& theme) {
    WINDOW* window = this->window();
    if (window == nullptr) {
        return;
    }

    werase(window);
    box(window, 0, 0);

    std::string hint;
    if (state.input_mode == InputMode::Editing) {
        hint = "Enter Confirm   Esc Cancel   ?? Move   Backspace Delete";
    } else if (state.screen == TuiScreen::Runtime) {
        hint = "S Stop   P Pause   R Return   Q Quit";
    } else {
        hint = "?? Navigate   ?? Change   Enter Edit   Tab Panel   L Launch   S Save   D Defaults   ? Help   Q Quit";
    }

    wattron(window, theme.muted);
    mvwaddstr(window, 1, 3, hint.substr(0, static_cast<std::size_t>(rect().width - 6)).c_str());
    wattroff(window, theme.muted);

    wnoutrefresh(window);
}

} // namespace ui
