#include "ui/widgets/configuration_panel.hpp"

#include "ui/widgets/value_editor.hpp"

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

} // namespace

void ConfigurationPanel::render(const TuiState& state,
                                const ApplicationSnapshot& snapshot,
                                const TuiTheme& theme) {
    WINDOW* window = this->window();
    if (window == nullptr) {
        return;
    }

    werase(window);
    box(window, 0, 0);

    wattron(window, theme.header | A_BOLD);
    mvwaddstr(window, 1, 2, "CONFIGURATION");
    wattroff(window, theme.header | A_BOLD);

    const auto row_labels = labels();
    const auto row_values = values(snapshot.config);
    const auto row_descriptions = descriptions();

    const int first_row = 3;
    const int row_height = 1;
    const int max_rows = std::min(static_cast<int>(row_labels.size()), rect().height - first_row - 1);

    for (int row = 0; row < max_rows; ++row) {
        const int y = first_row + row * row_height;
        const bool selected = state.focus_panel == FocusPanel::Configuration &&
                              state.selected_config_row == row;

        if (selected) {
            wattron(window, theme.selected);
            mvwhline(window, y, 1, ' ', rect().width - 2);
            wattroff(window, theme.selected);
        }

        const int marker_x = 2;
        const int label_x = 4;
        const int value_x = std::max(rect().width - 30, label_x + 12);
        const int description_x = value_x + 18;

        wattron(window, selected ? theme.selected : theme.secondary);
        mvwaddstr(window, y, marker_x, selected ? ">" : " ");
        mvwaddstr(window, y, label_x, row_labels[static_cast<std::size_t>(row)].c_str());
        wattroff(window, selected ? theme.selected : theme.secondary);

        const bool editing = state.input_mode == InputMode::Editing && state.edit_row == row;
        std::string value = row_values[static_cast<std::size_t>(row)];
        if (editing) {
            value = state.edit_buffer;
        }
        const ValueEditor editor(editor_mode(row), value);
        const int editor_width = std::max(std::min(rect().width - value_x - 2, 18), 0);
        if (editor_width > 0) {
            editor.render(window, {value_x, y, editor_width, 1}, theme, selected, editing);
        }

        if (description_x < rect().width - 2) {
            wattron(window, selected ? theme.selected : theme.muted);
            mvwaddstr(window, y, description_x, row_descriptions[static_cast<std::size_t>(row)].c_str());
            wattroff(window, selected ? theme.selected : theme.muted);
        }
    }

    wnoutrefresh(window);
}

} // namespace ui
