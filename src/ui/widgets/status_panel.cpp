#include "ui/widgets/status_panel.hpp"

#include "ui/widgets/widget_paint.hpp"

#include <ncursesw/curses.h>

#include <algorithm>
#include <string>
#include <vector>

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

    const int width = rect().width;
    const int height = rect().height;

    werase(window);
    paint::frame(window, theme.border);
    paint::title(window, theme.header | A_BOLD, "STATUS", width);

    const int content_x = paint::content_x();
    const int content_w = paint::content_width(width);
    const int first_row = paint::content_y();
    const int last_row = paint::content_bottom(height);
    if (content_w <= 0 || first_row > last_row) {
        wnoutrefresh(window);
        return;
    }

    const char* dot = theme.unicode_available ? "\xE2\x97\x8F" : "*"; // U+25CF
    const char* warn = theme.unicode_available ? "\xE2\x9A\xA0" : "!"; // U+26A0
    const char* err = theme.unicode_available ? "\xE2\x9C\x95" : "x";  // U+2715

    // An "indicator" is a colored glyph followed by mostly-neutral text.
    struct Indicator {
        const char* glyph;
        int glyph_color;
        std::string text;
        int text_color;
    };
    std::vector<Indicator> items;
    items.push_back({dot, theme.success, "Configuration loaded", theme.secondary});
    items.push_back({dot, theme.success, "Settings ready", theme.secondary});
    const bool running = snapshot.running;
    items.push_back({dot, running ? theme.success : theme.accent,
                     running ? (snapshot.paused ? "Paused" : "Running") : "Ready",
                     theme.secondary});

    // First content row: the indicator strip, spread across up to three columns.
    const int columns = width >= 78 ? 3 : (width >= 52 ? 2 : 1);
    const int column_w = std::max(content_w / columns, 14);
    for (std::size_t i = 0; i < items.size(); ++i) {
        const int col = static_cast<int>(i) % columns;
        const int rowoff = static_cast<int>(i) / columns;
        const int y = first_row + rowoff;
        if (y > last_row) {
            break;
        }
        const int x = content_x + col * column_w;
        const int cell_room = std::max(std::min(column_w - 1, width - 1 - x), 0);
        if (cell_room <= 2) {
            continue;
        }
        int cx = paint::text(window, y, x, cell_room, items[i].glyph_color | A_BOLD, items[i].glyph);
        paint::text(window, y, cx + 1, std::max(x + cell_room - (cx + 1), 0),
                    items[i].text_color, items[i].text);
    }

    // Following rows: a compact live config summary, then any warning/error.
    int y = first_row + (static_cast<int>(items.size()) + columns - 1) / columns;
    const auto& config = snapshot.config;
    if (y <= last_row) {
        const std::string summary =
            "Target " + config.target_ip + ":" + std::to_string(config.target_port) +
            "   Profile " + profile_name(config.packet_mode) +
            "   Workers " + std::to_string(config.worker_count);
        paint::text(window, y, content_x, content_w, theme.muted, summary);
        ++y;
    }

    if (config.payload_min == config.payload_max && y <= last_row) {
        int cx = paint::text(window, y, content_x, content_w, theme.warning | A_BOLD, warn);
        paint::text(window, y, cx + 1, std::max(content_x + content_w - (cx + 1), 0),
                    theme.secondary,
                    "Payload range is fixed (" + std::to_string(config.payload_min) + ")");
        ++y;
    }

    if (!state.error_message.empty() && y <= last_row) {
        int cx = paint::text(window, y, content_x, content_w, theme.danger | A_BOLD, err);
        paint::text(window, y, cx + 1, std::max(content_x + content_w - (cx + 1), 0),
                    theme.danger, state.error_message);
    }

    wnoutrefresh(window);
}

} // namespace ui
