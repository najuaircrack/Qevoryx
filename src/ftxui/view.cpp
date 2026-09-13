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
using ui::FocusPanel;
using ui::InputMode;
using ui::Severity;
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
    return snapshot.interfaces[static_cast<std::size_t>(index)].name;
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
                          ? "Enter to choose a detected IPv4 interface"
                          : snapshot.interfaces[static_cast<std::size_t>(selected_interface)].address +
                                "  Enter to choose";
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

inline Element interface_panel(const ApplicationSnapshot& snapshot, const TuiState& state) {
    Elements rows;
    if (snapshot.interfaces.empty()) {
        rows.push_back(text("No IPv4 interfaces found") | color(theme::Muted()));
    } else {
        const int selected = interface_index(snapshot);
        for (int index = 0; index < static_cast<int>(snapshot.interfaces.size()); ++index) {
            const auto& interface = snapshot.interfaces[static_cast<std::size_t>(index)];
            const bool current = index == selected;
            auto row = hbox({
                text(current ? "▶ " : "  ") |
                    color(current ? theme::Accent() : theme::Muted()),
                text(interface.name) |
                    color(current ? theme::Primary() : theme::Secondary()) |
                    size(WIDTH, EQUAL, 16),
                text("  "),
                text(interface.address) |
                    color(current ? theme::Primary() : theme::Muted()),
                text(interface.default_route ? "  default" : "") |
                    color(theme::Success()),
                filler(),
            });
            if (current) row = row | bgcolor(theme::SelectionBg());
            rows.push_back(std::move(row));
        }
    }

    auto body = vbox(std::move(rows)) |
                focusPosition(0, std::max(0, interface_index(snapshot)));
    return panel("INTERFACES", yframe(std::move(body)) | vscroll_indicator,
                 state.focus_panel == FocusPanel::Interfaces);
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
    if (state.focus_panel == FocusPanel::Interfaces) {
        return hbox({text(" "), segment("↑↓", "Choose interface"),
                     segment("Enter/Esc", "Back"), segment("Tab", "Next panel"),
                     segment("Q", "Quit")}) | color(theme::Border());
    }
    if (snapshot.running) {
        return hbox({text(" "), segment("↑↓", "Move/Select"), segment("P", "Pause/Resume"),
                     segment("X", "Stop"), segment("Tab", "Panel"), segment("?", "Help"),
                     segment("Q", "Quit")}) | color(theme::Border());
    }
    return hbox({text(" "), segment("↑↓", "Move/Select"), segment("←→", "Change"),
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
        line("Interface", "Enter on the Interface row to choose a detected IPv4 adapter"),
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

inline Element compact_header(const ApplicationSnapshot& snapshot) {
    const Color state_color = snapshot.running
                                  ? (snapshot.paused ? theme::Warning() : theme::Success())
                                  : theme::Accent();
    const std::string state_text = snapshot.running
                                       ? (snapshot.paused ? "PAUSED" : "RUNNING")
                                       : "READY";
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

inline Element main_screen(const ApplicationSnapshot& snapshot, const TuiState& state,
                           int width, int height) {
    Element side_panel = state.focus_panel == FocusPanel::Interfaces
                             ? interface_panel(snapshot, state) |
                                   size(WIDTH, EQUAL, width >= 100 ? 38 : 30)
                             : actions_panel(snapshot, state);
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
    Element screen = main_screen(snapshot, state, width, height);
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

} // namespace qevoryx::frontend
