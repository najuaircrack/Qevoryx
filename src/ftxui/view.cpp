#include "view.hpp"

#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "config/config.hpp"
#include "intro_data.hpp"
#include "logo.hpp"
#include "theme.hpp"
#include "ftxui/state.hpp"

namespace qevoryx::frontend {
using namespace ftxui;

using ui::ApplicationSnapshot;
using ui::C2FocusPanel;
using ui::FocusPanel;
using ui::InputMode;
using ui::Severity;
using ui::TuiMode;
using ui::TuiState;

const char* profile_value(config::PacketMode mode) {
    switch (mode) {
        case config::PacketMode::Mixed: return "mixed";
        case config::PacketMode::Tcp: return "tcp_syn";
        case config::PacketMode::Udp: return "udp";
        case config::PacketMode::Icmp: return "icmp_echo";
        case config::PacketMode::Ack: return "tcp_ack";
        case config::PacketMode::Rst: return "tcp_rst";
        case config::PacketMode::SynAck: return "tcp_syn_ack";
    }
    return "mixed";
}

inline const char* profile_label(config::PacketMode mode) {
    switch (mode) {
        case config::PacketMode::Mixed: return "Mixed";
        case config::PacketMode::Tcp: return "TCP SYN";
        case config::PacketMode::Udp: return "UDP";
        case config::PacketMode::Icmp: return "ICMP Echo";
        case config::PacketMode::Ack: return "TCP ACK";
        case config::PacketMode::Rst: return "TCP RST";
        case config::PacketMode::SynAck: return "TCP SYN ACK";
    }
    return "Mixed";
}

inline const char* c2_mode_label(int index) {
    switch (index % 7) {
        case 0: return "Mixed";
        case 1: return "TCP SYN";
        case 2: return "UDP";
        case 3: return "ICMP Echo";
        case 4: return "TCP ACK";
        case 5: return "TCP RST";
        case 6: return "TCP SYN ACK";
    }
    return "Mixed";
}

inline const char* source_value(bool spoofed) {
    return spoofed ? "spoofed" : "real";
}

inline const char* source_label(bool spoofed) {
    return spoofed ? "Spoofed" : "Real interface";
}

inline int interface_index(const ApplicationSnapshot& snapshot) {
    for (std::size_t index = 0; index < snapshot.interfaces.size(); ++index) {
        if (snapshot.interfaces[index].name == snapshot.config.real_ip_interface) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

inline std::string interface_display(const ApplicationSnapshot& snapshot) {
    if (snapshot.interfaces.empty()) return "No IPv4 interfaces";
    const int index = interface_index(snapshot);
    if (index < 0) return snapshot.config.real_ip_interface + " (unavailable)";
    return "< " + snapshot.interfaces[static_cast<std::size_t>(index)].name + " >";
}

std::vector<std::string> config_values(const config::Config& config) {
    return {
        config.target_ip,
        std::to_string(config.target_port),
        profile_value(config.packet_mode),
        std::to_string(config.worker_count),
        std::to_string(config.rate_limit),
        source_value(config.use_spoof_ips),
        config.real_ip_interface,
        std::to_string(config.payload_min),
        std::to_string(config.payload_max),
    };
}

inline Element panel(const std::string& title, Element body, bool focused) {
    auto border_color = focused ? theme::BorderFocus() : theme::Border();
    return window(text(" " + title + " ") | bold | color(border_color), std::move(body)) |
           color(border_color);
}

inline Element header(const ApplicationSnapshot& snapshot, int width, int height) {
    const auto& art = height >= 45 && width >= 150 ? logo::Emblem24
                      : height >= 38 && width >= 105 ? logo::Emblem20
                      : height >= 30 && width >= 88  ? logo::Emblem16
                      : height >= 26 && width >= 76  ? logo::Emblem12
                                                     : logo::Emblem8;
    const std::string settings = snapshot.settings_path.empty()
                                     ? std::string("Settings path unavailable")
                                     : snapshot.settings_path;
    const Color state_color = snapshot.running
                                  ? (snapshot.paused ? theme::Warning() : theme::Success())
                                  : theme::Accent();
    const std::string state_text = snapshot.running
                                       ? (snapshot.paused ? "PAUSED" : "RUNNING")
                                       : "READY";
    auto wordmark = vbox({
        hbox({text("QEVORY") | bold | color(theme::Primary()),
              text("X") | bold | color(theme::BrandRed())}),
        text("Terminal Control Panel") | color(theme::Secondary()),
        text("Backend connected") | color(theme::Muted()),
    });
    auto status = vbox({
        hbox({text("● ") | color(state_color), text(state_text) | bold | color(state_color)}),
        text(settings) | color(theme::Muted()),
    });
    auto row = hbox({
        render_emblem(art), text("  "), wordmark, filler(),
        separator() | color(theme::Border()), text(" "), status,
    });
    return window(text(""), row) | color(theme::Border()) |
           size(HEIGHT, EQUAL, static_cast<int>(art.size()) + 2);
}

inline Element config_panel(const ApplicationSnapshot& snapshot, const TuiState& state, int width) {
    const auto values = config_values(snapshot.config);
    auto display_values = values;
    display_values[6] = interface_display(snapshot);
    const std::vector<std::string> labels = {
        "Target IP", "Target Port", "Traffic Profile", "Workers",
        "Rate Limit", "Source Mode", "Interface", "Payload Min", "Payload Max",
    };
    std::vector<std::string> descriptions = {
        "Destination IPv4 address", "Destination port", "Packet profile token",
        "Worker threads", "Packets per second, 0 disables", "Source address selection",
        "Network interface", "UDP payload lower bound", "UDP payload upper bound",
    };
    const int selected_interface = interface_index(snapshot);
    descriptions[6] = selected_interface < 0
                          ? "Use Left/Right to choose an IPv4 interface"
                          : snapshot.interfaces[static_cast<std::size_t>(selected_interface)].address +
                                "  Left/Right to switch";
    Elements rows;
    for (int index = 0; index < config_row_count; ++index) {
        const bool selected = state.focus_panel == FocusPanel::Configuration &&
                              state.selected_config_row == index;
        const bool editing = selected && state.input_mode == InputMode::Editing;
        const std::string& value = editing ? state.edit_buffer : display_values[index];
        auto marker = text(selected ? "▶ " : "  ") |
                      color(selected ? theme::Accent() : theme::Muted());
        auto label = text(labels[index]) |
                     color(selected ? theme::Primary() : theme::Secondary()) |
                     size(WIDTH, EQUAL, 16);
        auto field = text((editing ? "│ " : "[ ") + value + (editing ? " │" : " ]")) |
                     color(selected ? theme::Accent() : theme::Primary()) |
                     size(WIDTH, EQUAL, 20);
        Element row = hbox({marker, label, text(" "), field});
        if (width >= 100) {
            row = hbox({marker, label, text(" "), field, text("  "),
                        text(descriptions[index]) | color(theme::Muted()) | flex});
        }
        if (selected) row = row | bgcolor(theme::SelectionBg());
        rows.push_back(std::move(row));
    }
    auto configuration_rows = vbox(std::move(rows)) |
                              focusPosition(0, state.selected_config_row);
    return panel("CONFIGURATION", yframe(std::move(configuration_rows)) | vscroll_indicator,
                 state.focus_panel == FocusPanel::Configuration) | flex;
}

inline std::vector<std::string> action_labels(const ApplicationSnapshot& snapshot) {
    return {
        "Launch", snapshot.running ? "Stop" : "Stop (idle)",
        snapshot.running && !snapshot.paused ? "Pause" : "Resume",
        "Save Settings", "Reset Defaults", "Focus Event Log", "Help", "Quit",
    };
}

inline Element actions_panel(const ApplicationSnapshot& snapshot, const TuiState& state) {
    const auto actions = action_labels(snapshot);
    Elements rows;
    for (int index = 0; index < action_count; ++index) {
        const bool selected = state.focus_panel == FocusPanel::Actions &&
                              state.selected_action == index;
        auto marker = text(selected ? "▶ " : "  ") |
                      color(selected ? theme::Accent() : theme::Muted());
        auto row = hbox({marker, text(actions[index]) | bold |
                                    color(selected ? theme::Primary() : theme::Secondary()),
                         filler()});
        if (selected) row = row | bgcolor(theme::SelectionBg());
        rows.push_back(std::move(row));
    }
    return panel("ACTIONS", vbox(std::move(rows)), state.focus_panel == FocusPanel::Actions) |
           size(WIDTH, EQUAL, 26);
}

inline Element status_panel(const ApplicationSnapshot& snapshot) {
    auto dot = [](Color tone) { return text("● ") | color(tone); };
    auto row = hbox({
        dot(snapshot.running ? theme::Success() : theme::Accent()),
        text(snapshot.running ? "Runtime active" : "Runtime idle") | color(theme::Secondary()),
        text("    "), dot(theme::Success()), text("Backend connected") | color(theme::Secondary()),
        text("    "), dot(snapshot.paused ? theme::Warning() : theme::Success()),
        text(snapshot.paused ? "Paused" : "Flow enabled") | color(theme::Secondary()),
    });
    auto counters = hbox({
        text("Generated: ") | color(theme::Muted()),
        text(std::to_string(snapshot.generated)) | bold | color(theme::Primary()),
        text("    Errors: ") | color(theme::Muted()),
        text(std::to_string(snapshot.errors)) | bold | color(theme::Primary()), filler(),
    });
    auto summary = text("Target " + snapshot.config.target_ip + ":" +
                        std::to_string(snapshot.config.target_port) + "   Profile " +
                        profile_label(snapshot.config.packet_mode) + "   Workers " +
                        std::to_string(snapshot.config.worker_count)) |
                   color(theme::Muted());
    return panel("STATUS", vbox({row, counters, summary}), false);
}

inline Color severity_color(Severity severity) {
    switch (severity) {
        case Severity::Info: return theme::Accent();
        case Severity::Warning: return theme::Warning();
        case Severity::Error: return theme::Error();
        case Severity::Success: return theme::Success();
    }
    return theme::Accent();
}

inline const char* severity_text(Severity severity) {
    switch (severity) {
        case Severity::Info: return "INFO ";
        case Severity::Warning: return "WARN ";
        case Severity::Error: return "ERROR";
        case Severity::Success: return "OK   ";
    }
    return "INFO ";
}

inline Element event_log_panel(const ApplicationSnapshot& snapshot, const TuiState& state) {
    Elements rows;
    const std::size_t count = snapshot.events.size();
    if (count == 0) {
        rows.push_back(text("No events recorded") | color(theme::Muted()));
    } else {
        const int safe_offset = (state.event_log_offset > static_cast<int>(count))
                                    ? 0 : state.event_log_offset;
        const std::size_t first = count > visible_log_rows
                                      ? count - visible_log_rows - safe_offset
                                      : 0;
        const std::size_t last = count - safe_offset;
        for (std::size_t index = first; index < last; ++index) {
            const auto& event = snapshot.events[index];
            rows.push_back(hbox({
                text(event.timestamp) | color(theme::Muted()), text("  "),
                text(severity_text(event.severity)) | bold | color(severity_color(event.severity)),
                text(" "), text(event.message) | color(theme::Secondary()),
            }));
        }
    }
    const int safe_offset = (state.event_log_offset > static_cast<int>(count))
                                ? 0 : state.event_log_offset;
    const std::string position = safe_offset == 0
                                     ? "latest"
                                     : "-" + std::to_string(safe_offset);
    auto footer = hbox({
        text(state.focus_panel == FocusPanel::EventLog ? "▲▼ Scroll" : "Latest events") |
            color(theme::Muted()),
        filler(), text(position) | color(theme::Muted()),
    });
    return panel("EVENT LOG", vbox({vbox(std::move(rows)),
                                    separator() | color(theme::Border()), footer}),
                 state.focus_panel == FocusPanel::EventLog);
}

inline Element footer(const ApplicationSnapshot& snapshot, const TuiState& state) {
    auto segment = [](const std::string& key, const std::string& description) {
        return hbox({text(key) | bold | color(theme::Accent()), text(" "),
                     text(description) | color(theme::Muted()), text("   ")});
    };
    auto mode_badge = hbox({
        text("F2 ") | bold | color(state.tui_mode == TuiMode::Local ? theme::Accent() : theme::Muted()),
        text("LOCAL  ") | color(state.tui_mode == TuiMode::Local ? theme::Primary() : theme::Muted()),
        text("F3 ") | bold | color(state.tui_mode == TuiMode::C2Server ? theme::Accent() : theme::Muted()),
        text("C2 SERVER  ") | color(state.tui_mode == TuiMode::C2Server ? theme::Primary() : theme::Muted()),
        separator() | color(theme::Border()), text(" "),
    });
    if (state.input_mode == InputMode::Editing) {
        return hbox({text(" "), mode_badge, segment("Enter", "Save"), segment("Esc", "Cancel"),
                     segment("Chars", "Edit value")}) | color(theme::Border());
    }
    if (state.focus_panel == FocusPanel::Configuration && state.selected_config_row == 6) {
        return hbox({text(" "), mode_badge, segment("↑↓", "Move"), segment("←→", "Switch interface"),
                     segment("Tab", "Panel"), segment("L", "Launch"), segment("S", "Save"),
                     segment("Q", "Quit")}) | color(theme::Border());
    }
    if (snapshot.running) {
        return hbox({text(" "), mode_badge, segment("↑↓", "Move/Select"), segment("P", "Pause/Resume"),
                     segment("X", "Stop"), segment("Tab", "Panel"), segment("?", "Help"),
                     segment("Q", "Quit")}) | color(theme::Border());
    }
    return hbox({text(" "), mode_badge, segment("↑↓", "Move/Select"), segment("←→", "Change"),
                 segment("Enter", "Edit"), segment("Tab", "Panel"), segment("L", "Launch"),
                 segment("S", "Save"), segment("?", "Help"),
                 segment("Q", "Quit")}) | color(theme::Border());
}

inline Element help_screen() {
    auto line = [](const std::string& key, const std::string& description) {
        return hbox({text(key) | bold | color(theme::Accent()) | size(WIDTH, EQUAL, 14),
                     text(description) | color(theme::Secondary())});
    };
    auto body = vbox({
        text("FTXUI control panel") | bold | color(theme::Primary()),
        separator() | color(theme::Border()),
        line("Up/Down", "Move within the focused panel"),
        line("Left/Right", "Cycle choices or adjust numeric values"),
        line("Space", "Cycle to the next choice"),
        line("Enter", "Edit a field or activate an action"),
        line("Interface", "Select Interface, then use Left/Right to switch adapters"),
        line("Tab", "Move focus between panels"),
        line("L", "Open the typed launch confirmation"),
        line("P", "Pause or resume a running test"),
        line("X", "Stop the running test"),
        line("S", "Save the current settings"),
        line("R", "Open the reset confirmation"),
        line("?", "Show or hide this help screen"),
        line("Q / Ctrl+C", "Stop the backend and exit"),
        separator() | color(theme::Border()),
        text("C2 Server Mode:") | bold | color(theme::Accent()),
        line("F2", "Switch to Local mode"),
        line("F3", "Switch to C2 Server mode"),
        line("F4", "Connect/disconnect remote Enterprise server"),
        line("Tab", "Cycle Server/Agents/Task/Campaigns/Malleable"),
        line("Up/Down", "Move through fields and actions"),
        line("Left/Right", "Adjust port/workers/rate/mode/source"),
        line("Enter", "Edit field or run selected action"),
        line("S (server)", "Start C2 server"),
        line("X (server)", "Stop C2 server"),
        line("A (task)", "Broadcast task to idle agents"),
        line("T (task)", "Stop all running tasks"),
        line("G (agents)", "Toggle targeted mode for selected agent"),
        line("A (agents)", "Send task to selected agent when targeted"),
        line("I", "Show agent install script"),
        line("PgUp/PgDn", "Scroll event log in C2 mode"),
        separator() | color(theme::Border()),
        text("Run traffic only against systems you own or have written permission to test.") |
            color(theme::Warning()),
    });
    return panel("HELP", body, true) | center | size(WIDTH, EQUAL, 72);
}

inline Element confirmation_dialog(const std::string& title, const std::string& prompt,
                                   const std::string& value, const std::string& error) {
    auto body = vbox({
        text(prompt) | color(theme::Secondary()),
        separator() | color(theme::Border()),
        hbox({text("Type: ") | color(theme::Muted()),
              text(value.empty() ? "│" : value + "│") | bold | color(theme::Accent())}),
        text(error.empty() ? "Enter confirms; Escape cancels." : error) |
            color(error.empty() ? theme::Muted() : theme::Error()),
    });
    return panel(title, body, true) | clear_under | center | size(WIDTH, EQUAL, 58);
}

inline Element remote_connect_dialog(const TuiState& state) {
    const char* step = "?";
    const char* prompt = "";
    std::string shown = state.edit_buffer;
    if (state.edit_row == 500) {
        step = "Step 1/3 — Server";
        prompt = "Enterprise server host/IP:";
    } else if (state.edit_row == 501) {
        step = "Step 2/3 — Port";
        prompt = "API port:";
    } else {
        step = "Step 3/3 — Token";
        prompt = "Operator token (hidden):";
        shown = std::string(state.edit_buffer.size(), '*');
    }
    auto body = vbox({
        text(step) | bold | color(theme::Accent()),
        text(prompt) | color(theme::Secondary()),
        separator() | color(theme::Border()),
        hbox({text("Type: ") | color(theme::Muted()),
              text(shown.empty() ? "│" : shown + "│") | bold | color(theme::Accent())}),
        text(state.error_message.empty() ? "Enter continues; Escape cancels." : state.error_message) |
            color(state.error_message.empty() ? theme::Muted() : theme::Error()),
    });
    return panel("REMOTE CONNECT", body, true) | clear_under | center | size(WIDTH, EQUAL, 58);
}

Element loading_screen(int frame, int width, int height) {
    const bool use_intro = width >= 100 && height >= 30;
    const auto& intro_art = logo::IntroFrames[
        static_cast<std::size_t>(frame) % logo::IntroFrames.size()
    ];
    const auto& art = use_intro ? intro_art
                      : height >= 24 && width >= 80  ? logo::Emblem16
                      : height >= 18 && width >= 60  ? logo::Emblem12
                                                     : logo::Emblem8;
    const float progress = static_cast<float>(frame) / 23.0f;
    const std::string status = frame < 6 ? "Initializing interface"
                           : frame < 12 ? "Loading configuration"
                           : frame < 18 ? "Connecting backend"
                                        : "Ready";
    return vbox({
        filler(),
        center(render_emblem(art)),
        text("QEVORY") | bold | color(theme::Primary()) | center,
        text("X") | bold | color(theme::BrandRed()) | center,
        text(status) | color(theme::Secondary()) | center,
        gauge(progress) | color(theme::Accent()) | size(WIDTH, EQUAL, 72) | center,
        filler(),
    }) | bgcolor(theme::Bg());
}

// ══════════════════════════════════════════════════════════════
//  COMPACT LAYOUT (small terminals)
// ══════════════════════════════════════════════════════════════

inline Element compact_header(const ApplicationSnapshot& snapshot) {
    const Color state_color = snapshot.running
                                  ? (snapshot.paused ? theme::Warning() : theme::Success())
                                  : theme::Accent();
    const std::string state_text = snapshot.running
                                       ? (snapshot.paused ? "PAUSED" : "RUNNING")
                                       : "READY";
    if (snapshot.c2.server_running) {
        return hbox({
            text("QEVORY") | bold | color(theme::Primary()),
            text("X") | bold | color(theme::BrandRed()), text("  "),
            text("● ") | color(theme::Success()),
            text("C2 ACTIVE") | bold | color(theme::Success()), filler(),
            text("Agents ") | color(theme::Muted()),
            text(std::to_string(snapshot.c2.agents.size())) | color(theme::Secondary()),
            text("  Pool ") | color(theme::Muted()),
            text(std::to_string(snapshot.c2.pool_active) + "/" +
                 std::to_string(snapshot.c2.pool_max)) | color(theme::Secondary()),
            text("  Campaigns ") | color(theme::Muted()),
            text(std::to_string(snapshot.c2.campaign_active)) | color(theme::Secondary()),
        }) | bgcolor(theme::Bg());
    }
    return hbox({
        text("QEVORY") | bold | color(theme::Primary()),
        text("X") | bold | color(theme::BrandRed()), text("  "),
        text("● ") | color(state_color),
        text(state_text) | bold | color(state_color), filler(),
        text("Workers ") | color(theme::Muted()),
        text(std::to_string(snapshot.config.worker_count)) | color(theme::Secondary()),
    }) | bgcolor(theme::Bg());
}

inline Element compact_status(const ApplicationSnapshot& snapshot) {
    return hbox({
        text("Generated ") | color(theme::Muted()),
        text(std::to_string(snapshot.generated)) | color(theme::Secondary()),
        text("  Errors ") | color(theme::Muted()),
        text(std::to_string(snapshot.errors)) | color(theme::Secondary()),
        filler(),
        text("Target " + snapshot.config.target_ip + ":" +
             std::to_string(snapshot.config.target_port)) | color(theme::Muted()),
    }) | bgcolor(theme::Bg());
}

inline Element compact_event(const ApplicationSnapshot& snapshot, const TuiState& state) {
    if (snapshot.events.empty())
        return text(" No events recorded") | color(theme::Muted()) | bgcolor(theme::Bg());

    std::size_t index = snapshot.events.size() - 1;
    if (state.event_log_offset < index) index -= state.event_log_offset;
    const auto& event = snapshot.events[index];
    return hbox({
        text(" "), text(event.timestamp) | color(theme::Muted()), text("  "),
        text(severity_text(event.severity)) | bold | color(severity_color(event.severity)),
        text(" "), text(event.message) | color(theme::Secondary()),
    }) | bgcolor(theme::Bg());
}

// ══════════════════════════════════════════════════════════════
//  C2 SERVER PANELS
// ══════════════════════════════════════════════════════════════

inline std::vector<std::string> c2_server_action_labels(const ApplicationSnapshot& snapshot) {
    return {
        snapshot.c2.server_running ? "Stop Server" : "Start Server",
        "Restart Server",
        "Install Script",
        "Back to Local",
    };
}

inline Element c2_server_panel(const ApplicationSnapshot& snapshot, const TuiState& state, int width) {
    const auto actions = c2_server_action_labels(snapshot);
    Elements rows;

    auto section = [](const std::string& label) {
        return hbox({text("-- ") | color(theme::Border()),
                     text(label) | bold | color(theme::Accent()),
                     text(" --") | color(theme::Border())});
    };

    auto stat = [](const std::string& label, const std::string& value, Color c = theme::Secondary()) {
        return hbox({text("  " + label) | color(theme::Muted()) | size(WIDTH, EQUAL, 14),
                     text(value) | color(c)});
    };

    rows.push_back(section("SERVER"));
    if (snapshot.c2.remote) {
        rows.push_back(hbox({text("  ") | color(theme::Success()),
                             text("REMOTE LINK") | bold | color(theme::Success())}));
        rows.push_back(stat("Server", snapshot.c2.remote_host + ":" +
                             std::to_string(snapshot.c2.server_port)));
        rows.push_back(stat("Mode", "Operator (F4 disconnects)"));
    } else {
        rows.push_back(hbox({text("  ") | color(snapshot.c2.server_running ? theme::Success() : theme::Error()),
                             text(snapshot.c2.server_running ? "ONLINE" : "OFFLINE") |
                                 bold | color(snapshot.c2.server_running ? theme::Success() : theme::Error())}));
    }
    // Editable Port / PSK fields. Selected via c2_server_cursor 0-1.
    // Hidden in remote mode (local server controls don't apply).
    if (!snapshot.c2.remote) {
        const bool focused = state.c2_focus == C2FocusPanel::ServerControl;
        const bool editing = focused && state.input_mode == InputMode::Editing &&
                             (state.edit_row == 100 || state.edit_row == 101);
        for (int fi = 0; fi < 2; ++fi) {
            const bool sel = focused && state.c2_server_cursor == fi;
            std::string value;
            if (editing && state.edit_row == 100 + fi) {
                value = state.edit_buffer;
            } else if (fi == 0) {
                value = snapshot.c2.server_running
                    ? std::to_string(snapshot.c2.server_port)
                    : state.c2_port_buffer;
            } else {
                const std::string& src = snapshot.c2.server_running && !snapshot.c2.psk.empty()
                    ? snapshot.c2.psk
                    : state.c2_psk_buffer;
                value = src.empty() ? std::string("(none)") :
                    std::string(std::min(src.size(), std::size_t(8)), '*') + "...";
            }
            const std::string label = fi == 0 ? "Port" : "PSK";
            auto marker = text(sel ? "▶ " : "  ") |
                          color(sel ? theme::Accent() : theme::Muted());
            auto label_el = text(label) | color(theme::Muted()) | size(WIDTH, EQUAL, 6);
            auto field_el = text((sel && editing ? "│ " : "[ ") + value +
                                (sel && editing ? " │" : " ]")) |
                            color(sel ? theme::Accent() : theme::Primary());
            auto row = hbox({marker, label_el, field_el, filler()});
            if (sel) row = row | bgcolor(theme::SelectionBg());
            rows.push_back(std::move(row));
        }
    }
    rows.push_back(stat("Bind", snapshot.c2.bind_address));

    if (snapshot.c2.server_running) {
        rows.push_back(text(" "));
        rows.push_back(section("STATUS"));
        auto uptime_s = snapshot.c2.uptime_sec;
        auto uptime_h = uptime_s / 3600;
        auto uptime_m = (uptime_s % 3600) / 60;
        rows.push_back(stat("Uptime", std::to_string(uptime_h) + "h " +
                            std::to_string(uptime_m) + "m"));
        rows.push_back(stat("Total conns", std::to_string(snapshot.c2.total_connections_accepted)));

        rows.push_back(text(" "));
        rows.push_back(section("POOL"));
        rows.push_back(stat("Active", std::to_string(snapshot.c2.pool_active) +
                            " / " + std::to_string(snapshot.c2.pool_max)));
        rows.push_back(stat("Queue", std::to_string(snapshot.c2.pool_queue)));
        rows.push_back(stat("Threads", std::to_string(snapshot.c2.pool_idle_threads) + " idle"));
    }

    rows.push_back(text(" "));
    rows.push_back(separator() | color(theme::Border()));

    for (int index = 0; index < c2_server_action_count; ++index) {
        const bool selected = state.c2_focus == C2FocusPanel::ServerControl &&
                              (state.c2_server_cursor - 2 == index);
        auto marker = text(selected ? "> " : "  ") |
                      color(selected ? theme::Accent() : theme::Muted());
        auto row = hbox({marker, text(actions[index]) |
                                    color(selected ? theme::Primary() : theme::Secondary()),
                         filler()});
        if (selected) row = row | bgcolor(theme::SelectionBg());
        rows.push_back(std::move(row));
    }

    return panel("C2 SERVER",
                  yframe(vbox(std::move(rows)) |
                         focusPosition(0, state.c2_server_cursor)) |
                      vscroll_indicator | flex,
                  state.c2_focus == C2FocusPanel::ServerControl) |
           size(WIDTH, EQUAL, 28);
}

inline Element c2_agent_list_panel(const ApplicationSnapshot& snapshot, const TuiState& state) {
    Elements rows;
    if (snapshot.c2.agents.empty()) {
        rows.push_back(text("No agents connected") | color(theme::Muted()));
    } else {
        const std::size_t count = snapshot.c2.agents.size();
        const std::size_t max_visible = visible_agent_rows;
        const int selected = state.c2_selected_agent;
        std::size_t first = 0;
        if (count > max_visible) {
            first = static_cast<std::size_t>(selected) >= max_visible
                        ? static_cast<std::size_t>(selected) - max_visible + 1
                        : 0;
        }
        std::size_t last = std::min(first + max_visible, count);

        for (std::size_t i = first; i < last; ++i) {
            const auto& agent = snapshot.c2.agents[i];
            const bool is_selected = static_cast<int>(i) == selected;

            Color status_color = agent.idle ? theme::Success()
                                : agent.status == "Busy" ? theme::Warning()
                                                         : theme::Accent();

            auto row = hbox({
                text(is_selected ? "▶ " : "  ") |
                    color(is_selected ? theme::Accent() : theme::Muted()),
                text(agent.hostname.substr(0, 12)) | bold |
                    color(is_selected ? theme::Primary() : theme::Secondary()) |
                    size(WIDTH, EQUAL, 14),
                text("● ") | color(status_color),
                text(agent.status) | color(status_color) |
                    size(WIDTH, EQUAL, 10),
                text(agent.os) | color(theme::Muted()) |
                    size(WIDTH, EQUAL, 8),
                text(std::to_string(agent.packets_sent) + " pkts") |
                    color(theme::Muted()),
            });
            if (is_selected) row = row | bgcolor(theme::SelectionBg());
            rows.push_back(std::move(row));
        }
    }

    // Detail for the selected agent, so CPU/mem/uptime/task are visible.
    if (!snapshot.c2.agents.empty() &&
        state.c2_selected_agent >= 0 &&
        state.c2_selected_agent < static_cast<int>(snapshot.c2.agents.size())) {
        const auto& sel = snapshot.c2.agents[static_cast<std::size_t>(state.c2_selected_agent)];
        rows.push_back(separator() | color(theme::Border()));
        rows.push_back(hbox({
            text("CPU ") | color(theme::Muted()),
            text(std::to_string(static_cast<int>(sel.cpu_usage)) + "%") | color(theme::Secondary()),
            text("  MEM ") | color(theme::Muted()),
            text(std::to_string(static_cast<int>(sel.memory_usage)) + "%") | color(theme::Secondary()),
            text("  Up ") | color(theme::Muted()),
            text(std::to_string(sel.uptime_sec) + "s") | color(theme::Secondary()),
        }));
        std::string short_id = sel.id.substr(0, 8);
        rows.push_back(hbox({
            text("ID " + short_id) | color(theme::Muted()),
            filler(),
            text(state.agent_assign_mode ? "[TARGETED]" : "") | bold | color(theme::Warning()),
        }));
    }

    auto footer_row = hbox({
        text("● ") | color(theme::Success()),
        text(std::to_string(snapshot.c2.agent_connected) + " conn") |
            color(theme::Success()),
        text("  ● ") | color(theme::Warning()),
        text(std::to_string(snapshot.c2.agent_busy) + " busy") |
            color(theme::Warning()),
        text("  ● ") | color(theme::Secondary()),
        text(std::to_string(snapshot.c2.agent_idle) + " idle") |
            color(theme::Secondary()),
        text("  " + std::to_string(snapshot.c2.agents.size()) + " agent(s)") |
            color(theme::Muted()),
        filler(),
        text("G target  A send") | color(theme::Muted()),
    });

    return panel("AGENTS",
                  vbox({yframe(vbox(std::move(rows)) |
                               focusPosition(0, state.c2_selected_agent)) |
                            vscroll_indicator | flex,
                        separator() | color(theme::Border()), footer_row}),
                  state.c2_focus == C2FocusPanel::AgentList);
}

inline Element c2_task_panel(const ApplicationSnapshot& snapshot, const TuiState& state, int width) {
    Elements rows;

    const bool focused = state.c2_focus == C2FocusPanel::TaskDispatch;
    const bool editing = focused && state.input_mode == InputMode::Editing &&
                         state.edit_row >= 200 && state.edit_row <= 205;

    auto field = [&](int index, const std::string& label, const std::string& value,
                     const std::string& desc) {
        const bool sel = focused && state.c2_task_cursor == index;
        const bool is_editing = sel && editing && state.edit_row == 200 + index;
        const std::string shown = is_editing ? state.edit_buffer : value;
        auto marker = text(sel ? "▶ " : "  ") |
                      color(sel ? theme::Accent() : theme::Muted());
        auto label_el = text(label) | color(theme::Secondary()) | size(WIDTH, EQUAL, 10);
        auto value_el = text((is_editing ? "│ " : "[ ") + shown +
                            (is_editing ? " │" : " ]")) |
                        color(sel ? theme::Accent() : theme::Primary());
        if (sel && state.c2_editing_field) value_el = value_el | bold;
        Element row = hbox({marker, label_el, text(" "), value_el});
        if (width >= 90) {
            row = hbox({marker, label_el, text(" "), value_el, text("  "),
                        text(desc) | color(theme::Muted()) | flex});
        }
        if (sel) row = row | bgcolor(theme::SelectionBg());
        return row;
    };

    rows.push_back(field(0, "Target", state.c2_target_buffer, "IPv4, Enter to edit"));
    rows.push_back(field(1, "Port", std::to_string(state.c2_target_port), "1-65535, ←→ adjust"));
    rows.push_back(field(2, "Mode", std::string(c2_mode_label(state.c2_mode_index)), "←→ / Space cycle"));
    rows.push_back(field(3, "Workers", std::to_string(state.c2_workers), "1-64, ←→ adjust"));
    rows.push_back(field(4, "Rate", std::to_string(state.c2_rate), "pps, ←→ adjust"));
    rows.push_back(field(5, "Source", std::string(state.c2_spoof ? "Spoofed" : "Real"), "←→ / Space toggle"));

    rows.push_back(separator() | color(theme::Border()));

    std::vector<std::string> task_actions = {"Broadcast Task (A)", "Stop All Tasks (T)", "Back to Local"};
    for (int index = 0; index < c2_task_action_count; ++index) {
        const bool selected = focused && state.c2_task_cursor == 6 + index;
        auto marker = text(selected ? "▶ " : "  ") |
                      color(selected ? theme::Accent() : theme::Muted());
        auto row = hbox({marker, text(task_actions[index]) | bold |
                                    color(selected ? theme::Primary() : theme::Secondary()),
                         filler()});
        if (selected) row = row | bgcolor(theme::SelectionBg());
        rows.push_back(std::move(row));
    }

    return panel("TASK DISPATCH",
                  yframe(vbox(std::move(rows)) |
                         focusPosition(0, state.c2_task_cursor)) |
                      vscroll_indicator | flex,
                  state.c2_focus == C2FocusPanel::TaskDispatch) | flex;
}

inline Element c2_campaign_panel(const ApplicationSnapshot& snapshot, const TuiState& state) {
    Elements rows;
    rows.push_back(text("Active: " + std::to_string(snapshot.c2.campaign_active)) |
                   color(theme::Secondary()));
    rows.push_back(separator() | color(theme::Border()));

    if (state.creating_campaign) {
        auto field = [&](int idx, const std::string& label, const std::string& val) {
            bool sel = state.campaign_selected_field == idx && state.c2_focus == C2FocusPanel::Campaigns;
            auto lbl = text(label) | color(theme::Muted()) | size(WIDTH, EQUAL, 10);
            auto v = text("[ " + val + " ]") | color(sel ? theme::Accent() : theme::Primary());
            if (sel && state.campaign_editing) v = v | bold;
            return hbox({lbl, v});
        };
        rows.push_back(text("--- NEW CAMPAIGN ---") | bold | color(theme::Accent()));
        rows.push_back(field(0, "Name:", state.campaign_name_buffer));
        rows.push_back(field(1, "Target:", state.campaign_target_buffer));
        rows.push_back(field(2, "Port:", std::to_string(state.campaign_target_port)));
        rows.push_back(field(3, "Workers:", std::to_string(state.campaign_workers)));
        rows.push_back(field(4, "Rate:", std::to_string(state.campaign_rate)));
        rows.push_back(field(5, "Duration:", std::to_string(state.campaign_duration)));
        rows.push_back(separator() | color(theme::Border()));
        rows.push_back(text("[Enter] Launch  [Esc] Cancel") | color(theme::Muted()));
    } else {
        if (snapshot.c2.campaign_names.empty()) {
            rows.push_back(snapshot.c2.campaign_active == 0
                ? text("No campaigns yet") | color(theme::Muted())
                : text(std::to_string(snapshot.c2.campaign_active) +
                       " campaign(s) running") | color(theme::Success()));
        } else {
            const std::size_t shown = std::min(snapshot.c2.campaign_names.size(),
                                               std::size_t{4});
            for (std::size_t i = 0; i < shown; ++i) {
                rows.push_back(text("• " + snapshot.c2.campaign_names[i]) |
                               color(theme::Secondary()));
            }
            if (snapshot.c2.campaign_names.size() > shown) {
                rows.push_back(text("+" + std::to_string(
                    snapshot.c2.campaign_names.size() - shown) + " more") |
                    color(theme::Muted()));
            }
        }
        rows.push_back(separator() | color(theme::Border()));
        bool sel = state.c2_focus == C2FocusPanel::Campaigns && state.c2_campaign_selected == 0;
        auto marker = text(sel ? "▶ " : "  ") | color(sel ? theme::Accent() : theme::Muted());
        rows.push_back(hbox({marker, text("Create Campaign") | bold |
            color(sel ? theme::Primary() : theme::Secondary())}));
    }
    const int camp_focus =
        state.creating_campaign ? state.campaign_selected_field + 2 : state.c2_campaign_selected;
    return panel("CAMPAIGNS",
                  yframe(vbox(std::move(rows)) | focusPosition(0, camp_focus)) |
                      vscroll_indicator | flex,
                  state.c2_focus == C2FocusPanel::Campaigns) | flex;
}

inline Element c2_malleable_panel(const ApplicationSnapshot& snapshot, const TuiState& state) {
    Elements rows;
    if (state.creating_malleable) {
        auto field = [&](int idx, const std::string& label, const std::string& val) {
            bool sel = state.malleable_selected_field == idx && state.c2_focus == C2FocusPanel::Malleable;
            auto lbl = text(label) | color(theme::Muted()) | size(WIDTH, EQUAL, 10);
            auto v = text("[ " + val + " ]") | color(sel ? theme::Accent() : theme::Primary());
            if (sel && state.malleable_editing) v = v | bold;
            return hbox({lbl, v});
        };
        rows.push_back(text("--- NEW PROFILE ---") | bold | color(theme::Accent()));
        rows.push_back(field(0, "Name:", state.malleable_name_buffer));
        rows.push_back(field(1, "User-Agent:", state.malleable_ua_buffer.substr(0, 30)));
        rows.push_back(field(2, "URI:", state.malleable_uri_buffer));
        rows.push_back(field(3, "Content-Type:", state.malleable_ct_buffer));
        rows.push_back(separator() | color(theme::Border()));
        rows.push_back(text("[Enter] Save  [Esc] Cancel") | color(theme::Muted()));
    } else {
        if (snapshot.c2.malleable_profiles.empty()) {
            rows.push_back(text("No profiles loaded") | color(theme::Muted()));
        } else {
            int idx = 0;
            for (const auto& name : snapshot.c2.malleable_profiles) {
                bool sel = state.c2_focus == C2FocusPanel::Malleable &&
                           state.c2_malleable_selected == idx;
                auto marker = text(sel ? "▶ " : "  ") |
                              color(sel ? theme::Accent() : theme::Muted());
                auto row = hbox({marker, text(name) | bold |
                    color(sel ? theme::Primary() : theme::Secondary()), filler()});
                if (sel) row = row | bgcolor(theme::SelectionBg());
                rows.push_back(std::move(row));
                ++idx;
            }
        }
        rows.push_back(separator() | color(theme::Border()));
        bool sel = state.c2_focus == C2FocusPanel::Malleable &&
                   state.c2_malleable_selected == static_cast<int>(snapshot.c2.malleable_profiles.size());
        auto marker = text(sel ? "▶ " : "  ") | color(sel ? theme::Accent() : theme::Muted());
        rows.push_back(hbox({marker, text("Create Profile") | bold |
            color(sel ? theme::Primary() : theme::Secondary())}));
    }
    const int mal_focus =
        state.creating_malleable ? state.malleable_selected_field + 1 : state.c2_malleable_selected;
    return panel("MALLEABLE PROFILES",
                  yframe(vbox(std::move(rows)) | focusPosition(0, mal_focus)) |
                      vscroll_indicator | flex,
                  state.c2_focus == C2FocusPanel::Malleable) | flex;
}

inline Element c2_footer(const ApplicationSnapshot& snapshot, const TuiState& state) {
    auto segment = [](const std::string& key, const std::string& description) {
        return hbox({text(key) | bold | color(theme::Accent()), text(" "),
                     text(description) | color(theme::Muted()), text("   ")});
    };
    auto mode_badge = hbox({
        text("F2 ") | bold | color(theme::Accent()),
        text("LOCAL  ") | color(theme::Muted()),
        text("F3 ") | bold | color(theme::Accent()),
        text("C2 SERVER  ") | color(theme::Primary()),
        separator() | color(theme::Border()), text(" "),
    });

    if (state.input_mode == InputMode::Editing) {
        return hbox({text(" "), mode_badge, segment("Enter", "Save"), segment("Esc", "Cancel"),
                     segment("Chars", "Edit value")}) | color(theme::Border());
    }

    if (state.c2_focus == C2FocusPanel::ServerControl) {
        return hbox({text(" "), mode_badge,
                     segment("↑↓", "Field/Action"), segment("Enter", "Edit/Run"),
                     segment("S/X", "Start/Stop"), segment("F4", "Remote"),
                     segment("Tab", "Panel"), segment("?", "Help"),
                     segment("Q", "Quit")}) | color(theme::Border());
    }
    if (state.c2_focus == C2FocusPanel::AgentList) {
        return hbox({text(" "), mode_badge,
                     segment("↑↓", "Agent"), segment("G", "Target"),
                     segment("A", "Send task"), segment("Tab", "Panel"),
                     segment("?", "Help"), segment("Q", "Quit")}) | color(theme::Border());
    }
    if (state.c2_focus == C2FocusPanel::Campaigns) {
        return hbox({text(" "), mode_badge,
                     segment("↑↓", "Select"), segment("Enter", "New/Save"),
                     segment("Tab", "Panel"), segment("?", "Help"),
                     segment("Q", "Quit")}) | color(theme::Border());
    }
    if (state.c2_focus == C2FocusPanel::Malleable) {
        return hbox({text(" "), mode_badge,
                     segment("↑↓", "Profile"), segment("Enter", "New/Save"),
                     segment("Tab", "Panel"), segment("?", "Help"),
                     segment("Q", "Quit")}) | color(theme::Border());
    }
    return hbox({text(" "), mode_badge,
                 segment("↑↓", "Field/Action"), segment("Enter", "Edit/Run"),
                 segment("←→", "Adjust"), segment("A", "Broadcast"),
                 segment("T", "Stop all"), segment("Tab", "Panel"),
                 segment("?", "Help"), segment("Q", "Quit")}) | color(theme::Border());
}

inline Element c2_header(const ApplicationSnapshot& snapshot, int width, int height) {
    const auto& art = height >= 38 && width >= 105 ? logo::Emblem20
                      : height >= 30 && width >= 88  ? logo::Emblem16
                      : height >= 26 && width >= 76  ? logo::Emblem12
                                                     : logo::Emblem8;
    const Color state_color = snapshot.c2.server_running ? theme::Success() : theme::Error();
    const std::string state_text = snapshot.c2.server_running ? "SERVER ACTIVE" : "SERVER IDLE";
    auto wordmark = vbox({
        hbox({text("QEVORY") | bold | color(theme::Primary()),
              text("X") | bold | color(theme::BrandRed())}),
        text("C2 Command & Control") | color(theme::Secondary()),
    });
    auto status = vbox({
        hbox({text("● ") | color(state_color), text(state_text) | bold | color(state_color)}),
        text("Port " + std::to_string(snapshot.c2.server_port)) | color(theme::Muted()),
        text(std::to_string(snapshot.c2.agents.size()) + " agent(s)") | color(theme::Muted()),
        hbox({
            text("Templates ") | color(theme::Muted()),
            text(std::to_string(snapshot.c2.template_count)) | color(theme::Secondary()),
            text("  Campaigns ") | color(theme::Muted()),
            text(std::to_string(snapshot.c2.campaign_active)) |
                color(snapshot.c2.campaign_active > 0 ? theme::Success() : theme::Secondary()),
            text("  Profiles ") | color(theme::Muted()),
            text(std::to_string(snapshot.c2.malleable_profiles.size())) |
                color(theme::Secondary()),
        }),
    });
    auto row = hbox({
        render_emblem(art), text("  "), wordmark, filler(),
        separator() | color(theme::Border()), text(" "), status,
    });
    return window(text(""), row) | color(theme::Border()) |
           size(HEIGHT, EQUAL, static_cast<int>(art.size()) + 2);
}

inline Element c2_main_screen(const ApplicationSnapshot& snapshot, const TuiState& state,
                              int width, int height) {
    auto server_pane = c2_server_panel(snapshot, state, width);
    auto task_pane = c2_task_panel(snapshot, state, width);
    auto agents_pane = c2_agent_list_panel(snapshot, state);
    auto campaigns_pane = c2_campaign_panel(snapshot, state);
    auto malleable_pane = c2_malleable_panel(snapshot, state);

    // All five panels stay visible: top = server + agents, bottom = task + campaigns + malleable.
    // Narrow terminals stack vertically instead of clipping.
    Element body;
    if (width < 110) {
        body = vbox({server_pane, text(" "), agents_pane, text(" "),
                     task_pane, text(" "), campaigns_pane, text(" "),
                     malleable_pane}) |
               flex;
    } else {
        auto top = hbox({server_pane, text(" "), agents_pane | flex}) | flex;
        auto bottom = hbox({task_pane | flex, text(" "), campaigns_pane | flex,
                            text(" "), malleable_pane | flex}) |
                      flex;
        body = vbox({top, text(" "), bottom}) | flex;
    }

    if (height < 16) {
        auto c2_compact = hbox({
            text("QEVORY") | bold | color(theme::Primary()),
            text("X") | bold | color(theme::BrandRed()), text("  "),
            text("● ") | color(snapshot.c2.server_running ? theme::Success() : theme::Error()),
            text(snapshot.c2.server_running ? "C2 ACTIVE" : "C2 IDLE") | bold |
                color(snapshot.c2.server_running ? theme::Success() : theme::Error()),
            filler(),
            text(std::to_string(snapshot.c2.agents.size()) + " agent(s)") | color(theme::Muted()),
            text("  Tmpl " + std::to_string(snapshot.c2.template_count)) | color(theme::Muted()),
            text("  Camp " + std::to_string(snapshot.c2.campaign_active)) | color(theme::Muted()),
            text("  Prof " + std::to_string(snapshot.c2.malleable_profiles.size())) |
                color(theme::Muted()),
        }) | bgcolor(theme::Bg());
        return vbox({c2_compact, body, c2_footer(snapshot, state)}) |
               bgcolor(theme::Bg());
    }

    return vbox({c2_header(snapshot, width, height), body,
                 event_log_panel(snapshot, state), c2_footer(snapshot, state)}) |
           bgcolor(theme::Bg());
}

inline Element main_screen(const ApplicationSnapshot& snapshot, const TuiState& state,
                           int width, int height) {
    Element side_panel = actions_panel(snapshot, state);
    auto body = hbox({config_panel(snapshot, state, width), text(" "),
                      std::move(side_panel)}) | flex;
    if (height < 12)
        return vbox({compact_header(snapshot), body}) | bgcolor(theme::Bg());
    if (height < 16)
        return vbox({compact_header(snapshot), body, footer(snapshot, state)}) |
               bgcolor(theme::Bg());
    if (height < 24)
        return vbox({compact_header(snapshot), body, compact_status(snapshot),
                     compact_event(snapshot, state), footer(snapshot, state)}) |
               bgcolor(theme::Bg());

    return vbox({header(snapshot, width, height), body, status_panel(snapshot),
                 event_log_panel(snapshot, state), footer(snapshot, state)}) |
           bgcolor(theme::Bg());
}

Element render(const ApplicationSnapshot& snapshot, const TuiState& state,
               int width, int height) {
    if (state.show_help) return help_screen() | bgcolor(theme::Bg());

    Element screen;
    if (state.tui_mode == TuiMode::C2Server) {
        screen = c2_main_screen(snapshot, state, width, height);
    } else {
        screen = main_screen(snapshot, state, width, height);
    }

    if (state.show_launch_confirmation) {
        screen = dbox({std::move(screen), confirmation_dialog(
            "LAUNCH CONFIRMATION", "Type YES to start live packet generation.",
            state.confirm_buffer, state.error_message)});
    } else if (state.show_reset_confirmation) {
        screen = dbox({std::move(screen), confirmation_dialog(
            "RESET CONFIRMATION", "Type RESET to restore safe defaults.",
            state.confirm_buffer, state.error_message)});
    } else if (state.input_mode == InputMode::Editing && state.edit_row >= 500) {
        screen = dbox({std::move(screen), remote_connect_dialog(state)});
    }
    return screen;
}

} // namespace qevoryx::frontend
