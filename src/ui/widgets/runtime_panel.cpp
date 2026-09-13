#include "ui/widgets/runtime_panel.hpp"

#include "ui/widgets/widget_paint.hpp"

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

    const int width = rect().width;
    const int height = rect().height;

    werase(window);
    paint::frame(window, theme.border);
    paint::title(window, theme.header | A_BOLD, "RUNTIME", width);

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

    const int content_x = paint::content_x() + 1;
    const int content_w = paint::content_width(width);
    const int first_row = paint::content_y();
    const int last_row = paint::content_bottom(height);
    const int value_x = content_x + 15;
    const int value_w = std::max(content_x + content_w - value_x, 0);

    for (std::size_t row = 0; row < rows.size(); ++row) {
        const int y = first_row + static_cast<int>(row);
        if (y > last_row) {
            break;
        }
        paint::text(window, y, content_x, 15, theme.secondary, rows[row].first);
        paint::text(window, y, value_x, value_w, theme.primary | A_BOLD, rows[row].second);
    }

    wnoutrefresh(window);
}

} // namespace ui
