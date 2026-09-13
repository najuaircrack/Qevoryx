#include "ui/widgets/selectable_list.hpp"

#include "ui/widgets/widget_paint.hpp"

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

    const char* marker = theme.unicode_available ? "\xE2\x96\xB6" : ">"; // U+25B6
    const int max_rows = std::min(static_cast<int>(items_.size()), rect.height);
    for (int row = 0; row < max_rows; ++row) {
        const int y = rect.y + row;
        const bool selected = focused && selected_ == row;

        if (selected) {
            wattron(window, theme.selected);
            mvwhline(window, y, rect.x, ' ', rect.width);
            wattroff(window, theme.selected);
        }

        const int attr = selected ? (theme.selected | A_BOLD) : theme.secondary;
        // Marker column (2 wide) then the label, all clipped to rect.width.
        paint::text(window, y, rect.x, 2, attr, selected ? std::string(marker) + " " : "  ");
        const int label_x = rect.x + 2;
        const int label_w = std::max(rect.width - 2, 0);
        paint::text(window, y, label_x, label_w, attr, items_[static_cast<std::size_t>(row)]);
    }
}

} // namespace ui
