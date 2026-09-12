#include "ui/widgets/modal.hpp"

#include <ncursesw/curses.h>

#include <algorithm>

namespace ui {

void Modal::set_content(const std::string& title,
                        const std::vector<std::string>& lines,
                        const std::string& cancel_label,
                        const std::string& confirm_label) {
    title_ = title;
    lines_ = lines;
    cancel_label_ = cancel_label;
    confirm_label_ = confirm_label;
}

void Modal::render(const TuiState&,
                   const ApplicationSnapshot&,
                   const TuiTheme& theme) {
    WINDOW* window = this->window();
    if (window == nullptr) {
        return;
    }

    werase(window);
    box(window, 0, 0);

    const int width = rect().width;
    const int height = rect().height;

    wattron(window, theme.header | A_BOLD);
    mvwaddstr(window, 1, 3, title_.substr(0, static_cast<std::size_t>(width - 6)).c_str());
    wattroff(window, theme.header | A_BOLD);

    const int first_row = 3;
    const int max_lines = std::min(static_cast<int>(lines_.size()), height - first_row - 3);
    for (int row = 0; row < max_lines; ++row) {
        wattron(window, theme.secondary);
        mvwaddstr(window, first_row + row, 3,
                  lines_[static_cast<std::size_t>(row)].substr(0, static_cast<std::size_t>(width - 6)).c_str());
        wattroff(window, theme.secondary);
    }

    const int button_y = height - 3;
    const int cancel_width = static_cast<int>(cancel_label_.size()) + 4;
    const int confirm_width = static_cast<int>(confirm_label_.size()) + 4;
    const int total_width = cancel_width + confirm_width + 4;
    const int start_x = std::max((width - total_width) / 2, 3);

    wattron(window, theme.secondary);
    mvwaddstr(window, button_y, start_x, ("[ " + cancel_label_ + " ]").c_str());
    wattroff(window, theme.secondary);

    wattron(window, theme.danger | A_BOLD);
    mvwaddstr(window, button_y, start_x + cancel_width + 4, ("[ " + confirm_label_ + " ]").c_str());
    wattroff(window, theme.danger | A_BOLD);

    wnoutrefresh(window);
}

} // namespace ui
