#include "ui/widgets/value_editor.hpp"

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

    const short color = selected ? theme.selected : theme.primary;
    wattron(window, color | (editing ? A_BOLD : A_NORMAL));

    if (mode_ == Mode::Enum || mode_ == Mode::Boolean) {
        const std::string text = "< " + value_ + " >";
        mvwaddstr(window, rect.y, rect.x, text.substr(0, static_cast<std::size_t>(rect.width)).c_str());
    } else {
        const std::string text = "[ " + value_ + (editing ? "_" : "") + " ]";
        mvwaddstr(window, rect.y, rect.x, text.substr(0, static_cast<std::size_t>(rect.width)).c_str());
    }

    wattroff(window, color | (editing ? A_BOLD : A_NORMAL));
}

} // namespace ui
