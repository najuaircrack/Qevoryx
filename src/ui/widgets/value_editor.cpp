#include "ui/widgets/value_editor.hpp"

#include <algorithm>
#include <cstddef>

namespace ui {

ValueEditor::ValueEditor(Mode mode, std::string value)
    : mode_(mode), value_(std::move(value)) {}

void ValueEditor::render(WINDOW* window,
                         const Rect& rect,
                         const TuiTheme& theme,
                         bool selected,
                         bool editing) const {
    if (window == nullptr || rect.width <= 0 || rect.height <= 0) {
        return;
    }

    if (selected) {
        wattron(window, theme.selected);
        mvwhline(window, rect.y, rect.x, ' ', rect.width);
        wattroff(window, theme.selected);
    }

    const int attr = (selected ? theme.selected : theme.primary) | (editing ? A_BOLD : A_NORMAL);
    wattron(window, attr);

    std::string text;
    if (mode_ == Mode::Enum || mode_ == Mode::Boolean) {
        text = "< " + value_ + " >";
    } else {
        text = "[ " + value_ + (editing ? "_" : "") + " ]";
    }
    // Clip to the assigned width so the editor never draws past its rect.
    mvwaddnstr(window, rect.y, rect.x, text.c_str(), rect.width);

    wattroff(window, attr);
}

} // namespace ui
