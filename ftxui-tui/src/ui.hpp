#pragma once

#include <ftxui/dom/elements.hpp>

#include <cstddef>
#include <string>
#include <vector>

#include "config/config.hpp"
#include "logo.hpp"
#include "theme.hpp"
#include "ui/tui_state.hpp"

namespace ui {
using namespace ftxui;

namespace ftxui_view {

constexpr int config_row_count = 9;
constexpr int action_count = 8;
constexpr std::size_t visible_log_rows = 5;

inline const char* profile_value(config::PacketMode mode) {
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

inline const char* source_value(bool spoofed) {
    return spoofed ? "spoofed" : "real";
}

inline const char* source_label(bool spoofed) {
    return spoofed ? "Spoofed" : "Real interface";
}

inline std::vector<std::string> config_values(const config::Config& config) {
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

inline Element header(const ApplicationSnapshot& snapshot, int width) {
    const auto& art = width >= 110 ? logo::Emblem16
                      : width >= 90  ? logo::Emblem12
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
    const std::vector<std::string> labels = {
        "Target IP", "Target Port", "Traffic Profile", "Workers",
        "Rate Limit", "Source Mode", "Interface", "Payload Min", "Payload Max",
    };
    const std::vector<std::string> descriptions = {
        "Destination IPv4 address", "Destination port", "Packet profile token",
        "Worker threads", "Packets per second, 0 disables", "Source address selection",
        "Network interface", "UDP payload lower bound", "UDP payload upper bound",
    };
    Elements rows;
    for (int index = 0; index < config_row_count; ++index) {
        const bool selected = state.focus_panel == FocusPanel::Configuration &&
                              state.selected_config_row == index;
        const bool editing = selected && state.input_mode == InputMode::Editing;
        const std::string& value = editing ? state.edit_buffer : values[index];
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
    return panel("CONFIGURATION", vbox(std::move(rows)),
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
        const std::size_t first = count > visible_log_rows
                                      ? count - visible_log_rows - state.event_log_offset
                                      : 0;
        const std::size_t last = count - state.event_log_offset;
        for (std::size_t index = first; index < last; ++index) {
            const auto& event = snapshot.events[index];
            rows.push_back(hbox({
                text(event.timestamp) | color(theme::Muted()), text("  "),
                text(severity_text(event.severity)) | bold | color(severity_color(event.severity)),
                text(" "), text(event.message) | color(theme::Secondary()),
            }));
        }
    }
    const std::string position = state.event_log_offset == 0
                                     ? "latest"
                                     : "-" + std::to_string(state.event_log_offset);
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
    if (state.input_mode == InputMode::Editing) {
        return hbox({text(" "), segment("Enter", "Save"), segment("Esc", "Cancel"),
                     segment("Chars", "Edit value")}) | color(theme::Border());
    }
    if (snapshot.running) {
        return hbox({text(" "), segment("↑↓", "Move"), segment("P", "Pause/Resume"),
                     segment("X", "Stop"), segment("Tab", "Panel"), segment("?", "Help"),
                     segment("Q", "Quit")}) | color(theme::Border());
    }
    return hbox({text(" "), segment("↑↓", "Move"), segment("←→", "Change"),
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
        line("Tab", "Move focus between panels"),
        line("L", "Open the typed launch confirmation"),
        line("P", "Pause or resume a running test"),
        line("X", "Stop the running test"),
        line("S", "Save the current settings"),
        line("R", "Open the reset confirmation"),
        line("?", "Show or hide this help screen"),
        line("Q / Ctrl+C", "Stop the backend and exit"),
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

inline Element main_screen(const ApplicationSnapshot& snapshot, const TuiState& state, int width) {
    auto body = hbox({config_panel(snapshot, state, width), text(" "),
                      actions_panel(snapshot, state)}) | flex;
    return vbox({header(snapshot, width), body, status_panel(snapshot),
                 event_log_panel(snapshot, state), footer(snapshot, state)}) |
           bgcolor(theme::Bg());
}

inline Element render(const ApplicationSnapshot& snapshot, const TuiState& state, int width) {
    if (state.show_help) return help_screen() | bgcolor(theme::Bg());
    Element screen = main_screen(snapshot, state, width);
    if (state.show_launch_confirmation) {
        screen = dbox({std::move(screen), confirmation_dialog(
            "LAUNCH CONFIRMATION", "Type YES to start live packet generation.",
            state.confirm_buffer, state.error_message)});
    } else if (state.show_reset_confirmation) {
        screen = dbox({std::move(screen), confirmation_dialog(
            "RESET CONFIRMATION", "Type RESET to restore safe defaults.",
            state.confirm_buffer, state.error_message)});
    }
    return screen;
}

} // namespace ftxui_view
} // namespace ui
