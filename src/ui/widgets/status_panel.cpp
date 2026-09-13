#include "ui/widgets/status_panel.hpp"

#include <ncursesw/curses.h>

#include <algorithm>
#include <string>

namespace ui {
namespace {

std::string profile_name(config::PacketMode mode) {
    switch (mode) {
        case config::PacketMode::Mixed: return "Mixed";
        case config::PacketMode::Tcp: return "TCP SYN";
        case config::PacketMode::Udp: return "UDP";
        case config::PacketMode::Icmp: return "ICMP";
        case config::PacketMode::Ack: return "TCP ACK";
        case config::PacketMode::Rst: return "TCP RST";
        case config::PacketMode::SynAck: return "TCP SYN ACK";
    }
    return "Mixed";
}

} // namespace

void StatusPanel::render(const TuiState& state,
                         const ApplicationSnapshot& snapshot,
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
    mvwaddstr(window, 1, 2, "STATUS");
    wattroff(window, theme.header | A_BOLD);

    // Run-state chip on the title row, right-aligned.
    const char* state_text = snapshot.paused ? "PAUSED" :
                             snapshot.running ? "RUNNING" : "READY";
    const int state_color = snapshot.paused ? theme.warning :
                            snapshot.running ? theme.success : theme.accent;
    const int state_x = std::max(width - static_cast<int>(std::string(state_text).size()) - 2, 10);
    wattron(window, state_color | A_BOLD);
    mvwaddstr(window, 1, state_x, state_text);
    wattroff(window, state_color | A_BOLD);

    const auto& config = snapshot.config;
    const char* bullet = theme.unicode_available ? "\xE2\x80\xA2" : "-"; // bullet

    // A live summary of the loaded configuration, laid out in three columns and
    // only as many rows as fit inside the border.
    struct Field {
        const char* label;
        std::string value;
    };
    const Field fields[] = {
        {"Target", config.target_ip + ":" + std::to_string(config.target_port)},
        {"Profile", profile_name(config.packet_mode)},
        {"Workers", std::to_string(config.worker_count)},
        {"Source", config.use_spoof_ips ? "Spoofed" : ("Real " + config.real_ip_interface)},
        {"Rate", config.rate_limit == 0 ? "Unlimited"
                                        : std::to_string(config.rate_limit) + "/worker"},
        {"Payload", std::to_string(config.payload_min) + "-" + std::to_string(config.payload_max)},
    };
    constexpr int field_count = static_cast<int>(sizeof(fields) / sizeof(fields[0]));

    const int columns = width >= 78 ? 3 : (width >= 52 ? 2 : 1);
    const int column_width = std::max((width - 4) / columns, 12);
    const int first_row = 2; // directly under the title; status is a short strip
    const int content_rows = std::max(height - first_row - 1, 0); // keep off bottom border

    // When an error is present, reserve the last content row for it.
    const bool has_error = !state.error_message.empty() && content_rows > 0;
    const int field_rows = std::max(has_error ? content_rows - 1 : content_rows, 0);
    const int shown = std::min(field_count, field_rows * columns);

    for (int i = 0; i < shown; ++i) {
        const int row = i / columns;
        const int col = i % columns;
        const int y = first_row + row;
        const int x = 3 + col * column_width;

        wattron(window, theme.muted);
        mvwaddstr(window, y, x, bullet);
        mvwaddstr(window, y, x + 2, fields[i].label);
        wattroff(window, theme.muted);

        const int value_x = x + 2 + 8;
        const int value_room = std::max(std::min(column_width - (value_x - x) - 1, width - value_x - 1), 0);
        if (value_room > 0) {
            wattron(window, theme.primary | A_BOLD);
            mvwaddstr(window, y, value_x,
                      fields[i].value.substr(0, static_cast<std::size_t>(value_room)).c_str());
            wattroff(window, theme.primary | A_BOLD);
        }
    }

    // Surface the most recent error on the reserved last row.
    if (has_error) {
        const int y = first_row + field_rows;
        wattron(window, theme.danger | A_BOLD);
        const int room = std::max(width - 6, 0);
        mvwaddstr(window, y, 3,
                  ("! " + state.error_message).substr(0, static_cast<std::size_t>(room)).c_str());
        wattroff(window, theme.danger | A_BOLD);
    }

    wnoutrefresh(window);
}

} // namespace ui
