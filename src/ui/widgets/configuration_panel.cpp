#include "ui/widgets/configuration_panel.hpp"

#include "ui/widgets/value_editor.hpp"
#include "ui/widgets/widget_paint.hpp"

#include <ncursesw/curses.h>

#include <algorithm>
#include <string>
#include <vector>

namespace ui {
namespace {

std::vector<std::string> labels() {
    return {
        "Target IP",
        "Target Port",
        "Traffic Profile",
        "Workers",
        "Source Mode",
        "Interface",
        "Payload Minimum",
        "Payload Maximum",
    };
}

std::vector<std::string> values(const config::Config& config) {
    return {
        config.target_ip,
        std::to_string(config.target_port),
        config.packet_mode == config::PacketMode::Mixed ? "Mixed" :
        config.packet_mode == config::PacketMode::Tcp ? "TCP SYN" :
        config.packet_mode == config::PacketMode::Udp ? "UDP" :
        config.packet_mode == config::PacketMode::Icmp ? "ICMP" :
        config.packet_mode == config::PacketMode::Ack ? "TCP ACK" :
        config.packet_mode == config::PacketMode::Rst ? "TCP RST" : "TCP SYN ACK",
        std::to_string(config.worker_count),
        config.use_spoof_ips ? "Spoofed" : "Real interface",
        config.real_ip_interface,
        std::to_string(config.payload_min),
        std::to_string(config.payload_max),
    };
}

std::vector<std::string> descriptions() {
    return {
        "Destination address",
        "Destination port",
        "Packet profile",
        "Worker threads",
        "Source selection",
        "Network interface",
        "UDP payload lower bound",
        "UDP payload upper bound",
    };
}

ValueEditor::Mode editor_mode(int row) {
    switch (row) {
        case 2: return ValueEditor::Mode::Enum;
        case 4: return ValueEditor::Mode::Boolean;
        case 1:
        case 3:
        case 6:
        case 7: return ValueEditor::Mode::Integer;
        default: return ValueEditor::Mode::String;
    }
}

// Column layout derived entirely from the content width. The value column never
// drops; the description column is removed progressively when space is tight.
struct Columns {
    int marker_x;
    int label_x;
    int label_w;
    int value_x;
    int value_w;
    int desc_x;
    int desc_w;
    bool show_desc;
};

Columns compute_columns(int content_x, int content_w) {
    constexpr int kMarkerW = 2;   // "> " gutter before the label
    constexpr int kLabelW = 16;   // fits "Traffic Profile" / "Payload Maximum"
    constexpr int kValueW = 20;   // fits "[ Real interface ]"
    constexpr int kGap = 2;
    constexpr int kMinDescW = 10;

    Columns c{};
    c.marker_x = content_x;
    c.label_x = content_x + kMarkerW;
    const int usable = std::max(content_w - kMarkerW, 0);
    c.label_w = std::min(kLabelW, usable);

    const int after_label = c.label_w + kGap;
    c.value_x = c.label_x + after_label;
    c.value_w = std::max(std::min(kValueW, usable - after_label), 0);

    const int used = kMarkerW + after_label + c.value_w + kGap;
    const int remaining = content_w - used;
    c.show_desc = remaining >= kMinDescW;
    c.desc_x = content_x + used;
    c.desc_w = c.show_desc ? remaining : 0;
    return c;
}

} // namespace

void ConfigurationPanel::render(const TuiState& state,
                                const ApplicationSnapshot& snapshot,
                                const TuiTheme& theme) {
    WINDOW* window = this->window();
    if (window == nullptr) {
        return;
    }

    const int width = rect().width;
    const int height = rect().height;
    const bool focused = state.focus_panel == FocusPanel::Configuration;

    werase(window);
    paint::frame(window, focused ? theme.border_focused : theme.border);
    paint::title(window, (focused ? theme.accent : theme.header) | A_BOLD,
                 "CONFIGURATION", width);

    const auto row_labels = labels();
    const auto row_values = values(snapshot.config);
    const auto row_descriptions = descriptions();

    const int content_x = paint::content_x();
    const int content_w = paint::content_width(width);
    const int first_row = paint::content_y();
    const int last_row = paint::content_bottom(height);
    const int available_rows = std::max(last_row - first_row + 1, 0);
    const int max_rows = std::min(static_cast<int>(row_labels.size()), available_rows);
    if (content_w <= 0) {
        wnoutrefresh(window);
        return;
    }

    const Columns cols = compute_columns(content_x, content_w);
    const char* marker = theme.unicode_available ? "\xE2\x96\xB6" : ">"; // U+25B6

    for (int row = 0; row < max_rows; ++row) {
        const int y = first_row + row;
        const bool selected = focused && state.selected_config_row == row;
        const bool editing = state.input_mode == InputMode::Editing && state.edit_row == row;

        // Selection highlight fills the inner width only, never the border.
        if (selected) {
            paint::fill_row(window, y, width, theme.selected);
        }

        const int label_attr = selected ? theme.selected : theme.secondary;

        // Marker sits at content_x; label follows the 2-col marker gutter.
        wattron(window, label_attr | (selected ? A_BOLD : 0));
        mvwaddnstr(window, y, cols.marker_x, selected ? marker : " ", 1);
        wattroff(window, label_attr | (selected ? A_BOLD : 0));

        paint::text(window, y, cols.label_x, cols.label_w, label_attr,
                    row_labels[static_cast<std::size_t>(row)]);

        // Value editor renders its own bracketed field within value_w.
        if (cols.value_w > 0) {
            std::string value = editing ? state.edit_buffer
                                        : row_values[static_cast<std::size_t>(row)];
            const ValueEditor editor(editor_mode(row), value);
            editor.render(window, {cols.value_x, y, cols.value_w, 1}, theme, selected, editing);
        }

        // Optional description, only when it fits without crossing the border.
        if (cols.show_desc) {
            const int desc_attr = selected ? theme.selected : theme.muted;
            paint::text(window, y, cols.desc_x, cols.desc_w, desc_attr,
                        row_descriptions[static_cast<std::size_t>(row)]);
        }
    }

    wnoutrefresh(window);
}

} // namespace ui
