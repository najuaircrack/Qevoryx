#include "ui/widgets/panel.hpp"

namespace ui {

Panel::~Panel() {
    if (panel_ != nullptr) {
        del_panel(panel_);
    }
    if (window_ != nullptr) {
        delwin(window_);
    }
}

void Panel::resize(const Rect& rect) {
    if (rect.width <= 0 || rect.height <= 0) {
        hide();
        return;
    }

    if (window_ != nullptr) {
        if (panel_ != nullptr) {
            del_panel(panel_);
            panel_ = nullptr;
        }
        delwin(window_);
        window_ = nullptr;
    }

    rect_ = rect;
    window_ = newwin(rect.height, rect.width, rect.y, rect.x);
    if (window_ == nullptr) {
        return;
    }

    keypad(window_, TRUE);
    box(window_, 0, 0);
    panel_ = new_panel(window_);
}

void Panel::show() {
    if (panel_ != nullptr) {
        show_panel(panel_);
    }
}

void Panel::hide() {
    if (panel_ != nullptr) {
        hide_panel(panel_);
    }
}

WINDOW* Panel::window() const { return window_; }
const Rect& Panel::rect() const { return rect_; }

} // namespace ui
