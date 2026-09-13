#include "ui/renderer.hpp"

#include "ui/widgets/widget_paint.hpp"

#include <ncursesw/curses.h>
#include <ncursesw/panel.h>

#include <algorithm>
#include <string>

namespace ui {
namespace {
constexpr int kMinCols = 80;
constexpr int kMinLines = 24;
} // namespace

Renderer::Renderer() = default;
Renderer::~Renderer() = default;

void Renderer::initialize() {
}

void Renderer::shutdown() {
}

void Renderer::render(const TuiState& state,
                      const TuiLayout& layout,
                      const TuiTheme& theme,
                      const ApplicationSnapshot& snapshot) {
    // On any terminal resize, force a single full repaint so no stale borders
    // or text survive from the previous size. This is not done per frame.
    if (COLS != last_cols_ || LINES != last_lines_) {
        last_cols_ = COLS;
        last_lines_ = LINES;
        clearok(curscr, TRUE);
        werase(stdscr);
        wnoutrefresh(stdscr);
    }

    if (COLS < kMinCols || LINES < kMinLines) {
        hide_all();
        render_too_small(theme);
        update_panels();
        doupdate();
        return;
    }

    if (state.screen == TuiScreen::Help) {
        render_help(state, layout, theme, snapshot);
    } else if (state.screen == TuiScreen::Runtime) {
        render_runtime(state, layout, theme, snapshot);
    } else {
        render_main(state, layout, theme, snapshot);
    }

    render_modal(state, layout, theme, snapshot);
    update_panels();
    doupdate();
}

void Renderer::hide_all() {
    header_.hide();
    configuration_.hide();
    actions_.hide();
    status_.hide();
    event_log_.hide();
    footer_.hide();
    help_.hide();
    runtime_.hide();
    live_status_.hide();
    modal_.hide();
}

void Renderer::render_too_small(const TuiTheme& theme) {
    werase(stdscr);
    const int mid = LINES / 2;
    const auto centered = [&](int y, int attr, const std::string& text) {
        if (y < 0 || y >= LINES) {
            return;
        }
        const int x = std::max((COLS - static_cast<int>(text.size())) / 2, 0);
        wattron(stdscr, attr);
        mvwaddnstr(stdscr, y, x, text.c_str(), std::max(COLS - x, 0));
        wattroff(stdscr, attr);
    };
    centered(mid - 2, theme.header | A_BOLD, "QEVORYX");
    centered(mid, theme.warning | A_BOLD, "Terminal window too small.");
    centered(mid + 2, theme.secondary,
             "Minimum: " + std::to_string(kMinCols) + " x " + std::to_string(kMinLines));
    centered(mid + 3, theme.muted,
             "Current: " + std::to_string(COLS) + " x " + std::to_string(LINES));
    centered(mid + 5, theme.muted, "Please enlarge the terminal.");
    wnoutrefresh(stdscr);
}

void Renderer::render_main(const TuiState& state,
                           const TuiLayout& layout,
                           const TuiTheme& theme,
                           const ApplicationSnapshot& snapshot) {
    header_.resize(layout.header());
    configuration_.resize(layout.configuration());
    actions_.resize(layout.actions());
    status_.resize(layout.status());
    footer_.resize(layout.footer());

    if (layout.event_log_visible()) {
        event_log_.resize(layout.event_log());
        event_log_.show();
    } else {
        event_log_.hide();
    }

    help_.hide();
    runtime_.hide();
    live_status_.hide();

    header_.show();
    configuration_.show();
    actions_.show();
    status_.show();
    footer_.show();

    header_.render(state, snapshot, theme);
    configuration_.render(state, snapshot, theme);
    actions_.render(state, snapshot, theme);
    status_.render(state, snapshot, theme);
    if (layout.event_log_visible()) {
        event_log_.render(state, snapshot, theme);
    }
    footer_.render(state, snapshot, theme);
}

void Renderer::render_runtime(const TuiState& state,
                              const TuiLayout& layout,
                              const TuiTheme& theme,
                              const ApplicationSnapshot& snapshot) {
    header_.resize(layout.header());
    runtime_.resize(layout.configuration());
    live_status_.resize(layout.actions());
    footer_.resize(layout.footer());

    if (layout.event_log_visible()) {
        event_log_.resize(layout.event_log());
        event_log_.show();
    } else {
        event_log_.hide();
    }

    configuration_.hide();
    actions_.hide();
    status_.hide();
    help_.hide();

    header_.show();
    runtime_.show();
    live_status_.show();
    footer_.show();

    header_.render(state, snapshot, theme);
    runtime_.render(state, snapshot, theme);
    live_status_.render(state, snapshot, theme);
    if (layout.event_log_visible()) {
        event_log_.render(state, snapshot, theme);
    }
    footer_.render(state, snapshot, theme);
}

void Renderer::render_help(const TuiState& state,
                           const TuiLayout& layout,
                           const TuiTheme& theme,
                           const ApplicationSnapshot& snapshot) {
    header_.resize(layout.header());
    help_.resize(layout.configuration());
    footer_.resize(layout.footer());

    configuration_.hide();
    actions_.hide();
    status_.hide();
    event_log_.hide();
    runtime_.hide();
    live_status_.hide();

    header_.show();
    help_.show();
    footer_.show();

    header_.render(state, snapshot, theme);
    help_.render(state, snapshot, theme);
    footer_.render(state, snapshot, theme);
}

void Renderer::render_modal(const TuiState& state,
                            const TuiLayout&,
                            const TuiTheme& theme,
                            const ApplicationSnapshot& snapshot) {
    if (!state.show_reset_confirmation && !state.show_launch_confirmation) {
        modal_.hide();
        return;
    }

    const int width = std::min(std::max(COLS - 20, 40), 70);
    const int height = std::min(std::max(LINES - 12, 8), 14);
    const int x = std::max((COLS - width) / 2, 0);
    const int y = std::max((LINES - height) / 2, 0);

    modal_.resize({x, y, width, height});
    if (state.show_launch_confirmation) {
        modal_.set_content(
            "CONFIRM LIVE TRAFFIC",
            {"Only continue for systems you are authorized to test.",
             "Type YES and press Enter to launch.",
             "> " + state.confirm_buffer},
            "Esc Cancel",
            "YES required",
            ModalKind::Danger);
    } else {
        modal_.set_content(
            "RESET SETTINGS",
            {"Reset all settings to safe defaults?"},
            "Cancel",
            "Reset",
            ModalKind::Warning);
    }
    modal_.show();
    modal_.render(state, snapshot, theme);
}

} // namespace ui
