#include "ui/widgets/footer.hpp"

#include <ncursesw/curses.h>

#include <algorithm>
#include <string>

namespace ui {

void FooterWidget::render(const TuiState& state,
                          const ApplicationSnapshot&,
                          const TuiTheme& theme) {
    WINDOW* window = this->window();
    if (window == nullptr) {
        return;
    }

    werase(window);
    box(window, 0, 0);

    std::string hint;
    const char* up = theme.unicode_available ? "\xE2\x86\x91" : "^";
    const char* down = theme.unicode_available ? "\xE2\x86\x93" : "v";
    const char* left = theme.unicode_available ? "\xE2\x86\x90" : "<";
    const char* right = theme.unicode_available ? "\xE2\x86\x92" : ">";

    if (state.input_mode == InputMode::Editing) {
        hint = std::string("Enter Confirm Esc Cancel ") + left + right + " Move Backspace Delete";
    } else if (state.screen == TuiScreen::Runtime) {
        hint = state.runtime_paused
                   ? "S Stop P Resume R Return Q Quit"
                   : "S Stop P Pause R Return Q Quit";
    } else {
        hint = std::string(up) + down + " Move " + left + right +
               " Change Enter Edit Tab Panel L Launch S Save D Defaults ? Help Q Quit";
    }

    wattron(window, theme.muted);
    const int available_width = std::max(rect().width - 6, 0);
    mvwaddstr(window, 1, 3, hint.substr(0, static_cast<std::size_t>(available_width)).c_str());
    wattroff(window, theme.muted);

    wnoutrefresh(window);
}

} // namespace ui
