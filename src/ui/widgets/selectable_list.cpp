#include "ui/widgets/selectable_list.hpp"

#include <algorithm>

namespace ui {

SelectableList::SelectableList(std::vector<std::string> items, int selected)
    : items_(std::move(items)), selected_(selected) {}

void SelectableList::render(WINDOW* window,
                            const Rect& rect,
                            const TuiTheme& theme,
                            bool focused) const {
    if (window == nullptr || rect.width <= 0 || rect.height <= 0) {
        return;
    }

    const int max_rows = std::min(static_cast<int>(items_.size()), rect.height);
    for (int row = 0; row < max_rows; ++row) {
        const int y = rect.y + row;
        const bool selected = focused && selected_ == row;

        if (selected) {
            wattron(window, theme.selected);
            mvwhline(window, y, rect.x, ' ', rect.width);
            wattroff(window, theme.selected);
        }

        wattron(window, selected ? theme.selected : theme.secondary);
        mvwaddstr(window, y, rect.x, selected ? "> " : "  ");
        mvwaddstr(window, y, rect.x + 2,
                  items_[static_cast<std::size_t>(row)].substr(0, static_cast<std::size_t>(rect.width - 2)).c_str());
        wattroff(window, selected ? theme.selected : theme.secondary);
    }
}

} // namespace ui
