#include "ui/widgets/modal.hpp"

#include "ui/widgets/widget_paint.hpp"

#include <ncursesw/curses.h>

#include <algorithm>

namespace ui {

void Modal::set_content(const std::string& title,
                        const std::vector<std::string>& lines,
                        const std::string& cancel_label,
                        const std::string& confirm_label,
                        ModalKind kind) {
    title_ = title;
    lines_ = lines;
    cancel_label_ = cancel_label;
    confirm_label_ = confirm_label;
    kind_ = kind;
}

void Modal::render(const TuiState& state,
                   const ApplicationSnapshot&,
                   const TuiTheme& theme) {
    WINDOW* window = this->window();
    if (window == nullptr) {
        return;
    }

    const int width = rect().width;
    const int height = rect().height;

    // Border color reflects the modal's intent.
    const int border_attr = kind_ == ModalKind::Danger ? theme.danger :
                            kind_ == ModalKind::Warning ? theme.warning : theme.border_focused;
    const int accent_attr = kind_ == ModalKind::Danger ? theme.danger :
                            kind_ == ModalKind::Warning ? theme.warning : theme.accent;

    werase(window);
    paint::frame(window, border_attr);
    paint::title(window, accent_attr | A_BOLD, title_, width);

    const int content_x = paint::content_x() + 1;
    const int content_w = std::max(width - 2 * content_x, 0);
    const int first_row = paint::content_y();
    const int last_row = paint::content_bottom(height) - 2; // leave room for buttons
    const int max_lines = std::min(static_cast<int>(lines_.size()), std::max(last_row - first_row + 1, 0));
    for (int row = 0; row < max_lines; ++row) {
        paint::text(window, first_row + row, content_x, content_w, theme.secondary,
                    lines_[static_cast<std::size_t>(row)]);
    }

    const int button_y = std::max(height - 2, first_row);
    const std::string cancel = "[ " + cancel_label_ + " ]";
    const std::string confirm = "[ " + confirm_label_ + " ]";
    const int total = static_cast<int>(cancel.size() + confirm.size()) + 4;
    const int start_x = std::max((width - total) / 2, content_x);

    const bool cancel_selected = !state.modal_confirm_selected;
    paint::text(window, button_y, start_x, static_cast<int>(cancel.size()),
                cancel_selected ? (theme.selected | A_BOLD) : theme.secondary, cancel);

    const int confirm_x = start_x + static_cast<int>(cancel.size()) + 4;
    paint::text(window, button_y, confirm_x, static_cast<int>(confirm.size()),
                state.modal_confirm_selected ? (accent_attr | A_BOLD) : theme.secondary, confirm);

    wnoutrefresh(window);
}

} // namespace ui
