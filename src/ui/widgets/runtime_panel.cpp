#include "ui/widgets/runtime_panel.hpp"

#include <ncursesw/curses.h>

#include <array>
#include <string>
#include <utility>

namespace ui {

void RuntimePanel::render(const TuiState&,
                          const ApplicationSnapshot& snapshot,
                          const TuiTheme& theme) {
    WINDOW* window = this->window();
    if (window == nullptr) {
        return;
    }

    werase(window);
    box(window, 0, 0);

    wattron(window, theme.header | A_BOLD);
    mvwaddstr(window, 1, 2, "RUNTIME");
    wattroff(window, theme.header | A_BOLD);

    const std::array<std::pair<const char*, std::string>, 4> rows = {{
        {"Profile", snapshot.config.packet_mode == config::PacketMode::Tcp ? "TCP SYN" :
                    snapshot.config.packet_mode == config::PacketMode::Udp ? "UDP" :
                    snapshot.config.packet_mode == config::PacketMode::Icmp ? "ICMP" :
                    snapshot.config.packet_mode == config::PacketMode::Ack ? "TCP ACK" :
                    snapshot.config.packet_mode == config::PacketMode::Rst ? "TCP RST" :
                    snapshot.config.packet_mode == config::PacketMode::SynAck ? "TCP SYN ACK" : "Mixed"},
        {"Workers", std::to_string(snapshot.config.worker_count)},
        {"Target", snapshot.config.target_ip + ":" + std::to_string(snapshot.config.target_port)},
        {"Rate", snapshot.config.rate_limit == 0 ? "Unlimited" : std::to_string(snapshot.config.rate_limit) + " per worker"},
    }};

    const int first_row = 3;
    for (std::size_t row = 0; row < rows.size(); ++row) {
        const int y = first_row + static_cast<int>(row);
        wattron(window, theme.secondary);
        mvwaddstr(window, y, 3, rows[row].first);
        wattroff(window, theme.secondary);

        wattron(window, theme.primary | A_BOLD);
        mvwaddstr(window, y, 18, rows[row].second.c_str());
        wattroff(window, theme.primary | A_BOLD);
    }

    wnoutrefresh(window);
}

} // namespace ui
