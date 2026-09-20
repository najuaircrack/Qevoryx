#include "view.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

#include <algorithm>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <future>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "app/application_controller.hpp"
#include "common/constants.hpp"
#include "common/network_interfaces.hpp"
#include "common/platform.hpp"
#include "config/config.hpp"
#include "config/settings_store.hpp"
#include "ftxui/state.hpp"

using namespace ftxui;

namespace {
namespace view = qevoryx::frontend;

std::optional<std::uint32_t> parse_u32(const std::string& value) {
    if (value.empty()) return std::nullopt;
    std::uint32_t result = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()) return std::nullopt;
    return result;
}

bool valid_config(const ui::ApplicationSnapshot& snapshot) {
    const config::Config& config = snapshot.config;
    struct in_addr address {};
    if (inet_pton(AF_INET, config.target_ip.c_str(), &address) != 1) return false;
    if (config.target_port == 0 || config.worker_count == 0 ||
        config.worker_count > static_cast<std::uint32_t>(common::MAX_THREADS)) return false;
    if (config.payload_min > config.payload_max || config.payload_max > 1472) return false;
    if (config.use_spoof_ips) return true;
    if (config.real_ip_interface.empty() || snapshot.interfaces.empty()) return false;
    return std::any_of(snapshot.interfaces.begin(), snapshot.interfaces.end(),
                       [&config](const common::NetworkInterface& interface) {
                           return interface.name == config.real_ip_interface;
                       });
}

void set_profile(config::Config& config, int index) {
    switch (index % 7) {
        case 0: config.packet_mode = config::PacketMode::Mixed; break;
        case 1: config.packet_mode = config::PacketMode::Tcp; break;
        case 2: config.packet_mode = config::PacketMode::Udp; break;
        case 3: config.packet_mode = config::PacketMode::Icmp; break;
        case 4: config.packet_mode = config::PacketMode::Ack; break;
        case 5: config.packet_mode = config::PacketMode::Rst; break;
        default: config.packet_mode = config::PacketMode::SynAck; break;
    }
}

int profile_index(config::PacketMode mode) {
    switch (mode) {
        case config::PacketMode::Mixed: return 0;
        case config::PacketMode::Tcp: return 1;
        case config::PacketMode::Udp: return 2;
        case config::PacketMode::Icmp: return 3;
        case config::PacketMode::Ack: return 4;
        case config::PacketMode::Rst: return 5;
        case config::PacketMode::SynAck: return 6;
    }
    return 0;
}

std::string initial_edit_value(const config::Config& config, int row) {
    return view::config_values(config)[static_cast<std::size_t>(row)];
}

int interface_index(const ui::ApplicationSnapshot& snapshot) {
    for (std::size_t index = 0; index < snapshot.interfaces.size(); ++index) {
        if (snapshot.interfaces[index].name == snapshot.config.real_ip_interface) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

void adjust_interface(ui::ApplicationSnapshot& snapshot, int direction) {
    if (snapshot.interfaces.empty()) return;
    const int count = static_cast<int>(snapshot.interfaces.size());
    const int current = interface_index(snapshot);
    const int index = current < 0
                          ? (direction < 0 ? count - 1 : 0)
                          : (current + direction + count) % count;
    snapshot.config.real_ip_interface =
        snapshot.interfaces[static_cast<std::size_t>(index)].name;
}

void adjust_config(ui::ApplicationSnapshot& snapshot, int row, int direction) {
    config::Config& config = snapshot.config;
    const auto change = [direction](std::uint32_t value, std::uint32_t step,
                                    std::uint32_t maximum) {
        if (direction > 0) return value >= maximum ? maximum : value + step;
        return value < step ? 0U : value - step;
    };
    switch (row) {
        case 1:
            config.target_port = static_cast<std::uint16_t>(
                change(config.target_port, 1U, std::numeric_limits<std::uint16_t>::max()));
            break;
        case 2:
            set_profile(config, profile_index(config.packet_mode) + direction);
            break;
        case 3:
            config.worker_count =
                change(config.worker_count, 1U, static_cast<std::uint32_t>(common::MAX_THREADS));
            if (config.worker_count == 0) config.worker_count = 1;
            break;
        case 4:
            config.rate_limit = change(config.rate_limit, 100U,
                                       std::numeric_limits<std::uint32_t>::max());
            break;
        case 5:
            config.use_spoof_ips = !config.use_spoof_ips;
            break;
        case 6:
            adjust_interface(snapshot, direction);
            break;
        case 7:
            config.payload_min = change(config.payload_min, 16U, 1472U);
            if (config.payload_min > config.payload_max) config.payload_max = config.payload_min;
            break;
        case 8:
            config.payload_max = change(config.payload_max, 16U, 1472U);
            if (config.payload_max < config.payload_min) config.payload_min = config.payload_max;
            break;
        default:
            break;
    }
}

bool commit_edit(ui::ApplicationSnapshot& snapshot, ui::TuiState& state) {
    config::Config& config = snapshot.config;
    const std::string& value = state.edit_buffer;
    switch (state.edit_row) {
        case 0: config.target_ip = value; break;
        case 1: {
            const auto parsed = parse_u32(value);
            if (!parsed || *parsed == 0 || *parsed > std::numeric_limits<std::uint16_t>::max()) {
                state.error_message = "Port must be from 1 to 65535.";
                return false;
            }
            config.target_port = static_cast<std::uint16_t>(*parsed);
            break;
        }
        case 2:
            if (value == "mixed") config.packet_mode = config::PacketMode::Mixed;
            else if (value == "tcp_syn") config.packet_mode = config::PacketMode::Tcp;
            else if (value == "udp") config.packet_mode = config::PacketMode::Udp;
            else if (value == "icmp_echo") config.packet_mode = config::PacketMode::Icmp;
            else if (value == "tcp_ack") config.packet_mode = config::PacketMode::Ack;
            else if (value == "tcp_rst") config.packet_mode = config::PacketMode::Rst;
            else if (value == "tcp_syn_ack") config.packet_mode = config::PacketMode::SynAck;
            else {
                state.error_message = "Use mixed, tcp_syn, udp, icmp_echo, tcp_ack, tcp_rst, or tcp_syn_ack.";
                return false;
            }
            break;
        case 3: {
            const auto parsed = parse_u32(value);
            if (!parsed || *parsed == 0 ||
                *parsed > static_cast<std::uint32_t>(common::MAX_THREADS)) {
                state.error_message = "Workers must be from 1 to 10000.";
                return false;
            }
            config.worker_count = *parsed;
            break;
        }
        case 4: {
            const auto parsed = parse_u32(value);
            if (!parsed) {
                state.error_message = "Rate limit must be an unsigned integer.";
                return false;
            }
            config.rate_limit = *parsed;
            break;
        }
        case 5:
            if (value == "real") config.use_spoof_ips = false;
            else if (value == "spoofed") config.use_spoof_ips = true;
            else {
                state.error_message = "Use real or spoofed.";
                return false;
            }
            break;
        case 6: config.real_ip_interface = value; break;
        case 7: {
            const auto parsed = parse_u32(value);
            if (!parsed || *parsed > 1472) {
                state.error_message = "Payload minimum must be from 0 to 1472.";
                return false;
            }
            config.payload_min = *parsed;
            break;
        }
        case 8: {
            const auto parsed = parse_u32(value);
            if (!parsed || *parsed > 1472) {
                state.error_message = "Payload maximum must be from 0 to 1472.";
                return false;
            }
            config.payload_max = *parsed;
            break;
        }
        default: return false;
    }
    if (!valid_config(snapshot)) {
        state.error_message = "Configuration is invalid. Check IP, workers, and payload range.";
        return false;
    }
    state.error_message.clear();
    state.input_mode = ui::InputMode::Navigation;
    return true;
}

void refresh_runtime(ui::ApplicationSnapshot& snapshot,
                     const ui::ApplicationSnapshot& latest) {
    snapshot.running = latest.running;
    snapshot.paused = latest.paused;
    snapshot.ready = latest.ready;
    snapshot.generated = latest.generated;
    snapshot.errors = latest.errors;
    snapshot.settings_path = latest.settings_path;
    snapshot.interfaces = latest.interfaces;
    snapshot.events = latest.events;
    snapshot.c2 = latest.c2;
}

void next_panel(ui::TuiState& state, bool reverse) {
    int panel = 0;
    switch (state.focus_panel) {
        case ui::FocusPanel::Configuration: panel = 0; break;
        case ui::FocusPanel::Actions: panel = 1; break;
        case ui::FocusPanel::EventLog: panel = 2; break;
        default: panel = 0; break;
    }
    panel = reverse ? (panel + 2) % 3 : (panel + 1) % 3;
    state.focus_panel = panel == 0 ? ui::FocusPanel::Configuration
                        : panel == 1 ? ui::FocusPanel::Actions
                                     : ui::FocusPanel::EventLog;
}

void next_c2_panel(ui::TuiState& state, bool reverse) {
    int panel = 0;
    switch (state.c2_focus) {
        case ui::C2FocusPanel::ServerControl: panel = 0; break;
        case ui::C2FocusPanel::AgentList: panel = 1; break;
        case ui::C2FocusPanel::TaskDispatch: panel = 2; break;
        case ui::C2FocusPanel::Campaigns: panel = 3; break;
        case ui::C2FocusPanel::Malleable: panel = 4; break;
    }
    panel = reverse ? (panel + 4) % 5 : (panel + 1) % 5;
    state.c2_focus = panel == 0 ? ui::C2FocusPanel::ServerControl
                    : panel == 1 ? ui::C2FocusPanel::AgentList
                    : panel == 2 ? ui::C2FocusPanel::TaskDispatch
                    : panel == 3 ? ui::C2FocusPanel::Campaigns
                                 : ui::C2FocusPanel::Malleable;
}

void start_edit(const ui::ApplicationSnapshot& snapshot, ui::TuiState& state) {
    if (state.focus_panel != ui::FocusPanel::Configuration) return;
    if (state.selected_config_row == 6) {
        state.error_message.clear();
        return;
    }
    state.edit_row = state.selected_config_row;
    state.edit_buffer = initial_edit_value(snapshot.config, state.edit_row);
    state.input_mode = ui::InputMode::Editing;
    state.error_message.clear();
}

void open_launch_confirmation(ui::TuiState& state) {
    state.show_launch_confirmation = true;
    state.show_reset_confirmation = false;
    state.show_help = false;
    state.confirm_buffer.clear();
    state.error_message.clear();
    state.input_mode = ui::InputMode::Modal;
}

void open_reset_confirmation(ui::TuiState& state) {
    state.show_reset_confirmation = true;
    state.show_launch_confirmation = false;
    state.show_help = false;
    state.confirm_buffer.clear();
    state.error_message.clear();
    state.input_mode = ui::InputMode::Modal;
}

void close_modal(ui::TuiState& state) {
    state.show_launch_confirmation = false;
    state.show_reset_confirmation = false;
    state.confirm_buffer.clear();
    state.error_message.clear();
    state.input_mode = ui::InputMode::Navigation;
}

inline void sync_task_cursor(ui::TuiState& state);
inline void sync_server_cursor(ui::TuiState& state);
void adjust_c2_config(ui::TuiState& state, int direction);
void start_c2_edit(ui::TuiState& state);
bool commit_c2_edit(ui::TuiState& state);

bool activate_action(ui::ApplicationSnapshot& snapshot, ui::TuiState& state,
                     app::ApplicationController& controller) {
    switch (state.selected_action) {
        case 0:
            if (!snapshot.running && valid_config(snapshot)) open_launch_confirmation(state);
            else if (!valid_config(snapshot))
                state.error_message = "Fix the configuration before launching.";
            return true;
        case 1: controller.stop(); return true;
        case 2:
            if (snapshot.running && snapshot.paused) controller.resume();
            else if (snapshot.running) controller.pause();
            else state.error_message = "Runtime is not active.";
            return true;
        case 3:
            if (valid_config(snapshot)) controller.save(snapshot.config);
            else state.error_message = "Cannot save an invalid configuration.";
            return true;
        case 4: open_reset_confirmation(state); return true;
        case 5:
            state.focus_panel = ui::FocusPanel::EventLog;
            state.event_log_offset = 0;
            return true;
        case 6: state.show_help = !state.show_help; return true;
        case 7: return false;
        default: return true;
    }
}

inline bool broadcast_current_task(ui::ApplicationSnapshot& snapshot, ui::TuiState& state,
                                     app::ApplicationController& controller,
                                     const std::string& agent_id = "") {
    if (!snapshot.c2.server_running) {
        state.error_message = "Start the server first.";
        return true;
    }
    if (state.c2_target_buffer.empty()) {
        state.error_message = "Enter a target IP.";
        return true;
    }
    {
        struct in_addr check_addr{};
        if (inet_pton(AF_INET, state.c2_target_buffer.c_str(), &check_addr) != 1) {
            state.error_message = "Invalid target IP address.";
            return true;
        }
    }
    if (state.c2_target_port == 0) {
        state.error_message = "Port must be from 1 to 65535.";
        return true;
    }
    if (state.c2_workers == 0 || state.c2_workers > 64) {
        state.error_message = "Workers must be from 1 to 64.";
        return true;
    }
    if (agent_id.empty()) {
        controller.c2_broadcast_task(
            state.c2_target_buffer, state.c2_target_port,
            state.c2_mode_index, state.c2_workers, state.c2_rate,
            state.c2_spoof);
    } else {
        controller.c2_send_task_to_agent(
            agent_id, state.c2_target_buffer, state.c2_target_port,
            state.c2_mode_index, state.c2_workers, state.c2_rate,
            state.c2_spoof);
    }
    refresh_runtime(snapshot, controller.snapshot());
    return true;
}

bool activate_c2_action(ui::ApplicationSnapshot& snapshot, ui::TuiState& state,
                        app::ApplicationController& controller) {
    if (state.c2_focus == ui::C2FocusPanel::ServerControl) {
        sync_server_cursor(state);
        const int action = state.c2_server_cursor - 2;
        if (state.c2_server_cursor <= 1) {
            start_c2_edit(state);
            return true;
        }
        switch (action) {
            case 0: {
                if (snapshot.c2.server_running) {
                    controller.c2_stop_server();
                } else {
                    std::uint16_t port = 7777;
                    auto parsed = parse_u32(state.c2_port_buffer);
                    if (parsed && *parsed > 0 && *parsed <= 65535)
                        port = static_cast<std::uint16_t>(*parsed);
                    else {
                        state.error_message = "Invalid port. Enter a value from 1 to 65535.";
                        return true;
                    }
                    controller.c2_start_server(port, state.c2_psk_buffer);
                }
                refresh_runtime(snapshot, controller.snapshot());
                return true;
            }
            case 1: {
                if (snapshot.c2.server_running) {
                    auto port = snapshot.c2.server_port;
                    auto psk = snapshot.c2.psk;
                    controller.c2_stop_server();
                    controller.c2_start_server(port, psk);
                }
                refresh_runtime(snapshot, controller.snapshot());
                return true;
            }
            case 2: {
                std::string script = controller.c2_install_script();
                state.error_message = script.empty() ? "No script available" : "Script copied";
                return true;
            }
            case 3:
                state.tui_mode = ui::TuiMode::Local;
                return true;
            default: return true;
        }
    }

    if (state.c2_focus == ui::C2FocusPanel::TaskDispatch) {
        sync_task_cursor(state);
        if (state.c2_task_cursor <= 5) {
            // Text fields open the editor; toggles adjust directly.
            if (state.c2_task_cursor == 0 || state.c2_task_cursor == 1 ||
                state.c2_task_cursor == 3 || state.c2_task_cursor == 4) {
                start_c2_edit(state);
                return true;
            }
            adjust_c2_config(state, 1);
            return true;
        }
        switch (state.c2_task_cursor - 6) {
            case 0: {
                return broadcast_current_task(snapshot, state, controller);
            }
            case 1: {
                controller.c2_stop_task("");
                refresh_runtime(snapshot, controller.snapshot());
                return true;
            }
            case 2: {
                state.tui_mode = ui::TuiMode::Local;
                return true;
            }
            default: return true;
        }
    }

    if (state.c2_focus == ui::C2FocusPanel::Campaigns) {
        if (state.creating_campaign) {
            if (state.campaign_selected_field >= 5) {
                // Save: validate then create + start via backend.
                if (state.campaign_target_buffer.empty()) {
                    start_c2_edit(state);
                    return true;
                }
                controller.c2_create_campaign(
                    state.campaign_name_buffer, state.campaign_target_buffer,
                    state.campaign_target_port, state.campaign_mode_index,
                    state.campaign_workers, state.campaign_rate,
                    state.campaign_duration);
                state.creating_campaign = false;
                state.campaign_editing = false;
                refresh_runtime(snapshot, controller.snapshot());
                return true;
            }
            start_c2_edit(state);
            return true;
        } else {
            if (state.c2_campaign_selected == 0) {
                state.creating_campaign = true;
                state.campaign_name_buffer.clear();
                state.campaign_target_buffer.clear();
                state.campaign_selected_field = 0;
                state.campaign_editing = false;
                return true;
            }
        }
        return true;
    }

    if (state.c2_focus == ui::C2FocusPanel::Malleable) {
        if (state.creating_malleable) {
            if (state.malleable_selected_field >= 3) {
                if (state.malleable_name_buffer.empty()) {
                    start_c2_edit(state);
                    return true;
                }
                controller.c2_add_malleable(
                    state.malleable_name_buffer, state.malleable_ua_buffer,
                    state.malleable_uri_buffer, state.malleable_ct_buffer);
                state.creating_malleable = false;
                state.malleable_editing = false;
                refresh_runtime(snapshot, controller.snapshot());
                return true;
            }
            start_c2_edit(state);
            return true;
        } else {
            if (state.c2_malleable_selected == static_cast<int>(snapshot.c2.malleable_profiles.size())) {
                state.creating_malleable = true;
                state.malleable_name_buffer.clear();
                state.malleable_ua_buffer.clear();
                state.malleable_uri_buffer.clear();
                state.malleable_ct_buffer = "application/octet-stream";
                state.malleable_selected_field = 0;
                state.malleable_editing = false;
                return true;
            }
        }
        return true;
    }

    return true;
}

inline void sync_task_cursor(ui::TuiState& state) {
    if (state.c2_task_cursor < 0) state.c2_task_cursor = 0;
    if (state.c2_task_cursor > 8) state.c2_task_cursor = 8;
    if (state.c2_task_cursor <= 5) {
        state.c2_selected_field = state.c2_task_cursor;
    } else {
        state.c2_selected_task_action = state.c2_task_cursor - 6;
    }
}

inline void sync_server_cursor(ui::TuiState& state) {
    if (state.c2_server_cursor < 0) state.c2_server_cursor = 0;
    if (state.c2_server_cursor > 5) state.c2_server_cursor = 5;
    if (state.c2_server_cursor <= 1) {
        state.c2_server_selected_field = state.c2_server_cursor;
    } else {
        state.c2_selected_task_action = state.c2_server_cursor - 2;
    }
}

void adjust_c2_config(ui::TuiState& state, int direction) {
    if (state.c2_focus != ui::C2FocusPanel::TaskDispatch) return;

    const int field = state.c2_task_cursor <= 5 ? state.c2_task_cursor
                                                : state.c2_selected_field;
    const auto step = [direction](std::uint32_t value, std::uint32_t step_size,
                                   std::uint32_t maximum) {
        if (direction > 0) return value >= maximum ? maximum : value + step_size;
        return value < step_size ? 0U : value - step_size;
    };

    switch (field) {
        case 1:
            state.c2_target_port = static_cast<std::uint16_t>(
                step(state.c2_target_port, 1U, 65535));
            if (state.c2_target_port == 0) state.c2_target_port = 1;
            break;
        case 2:
            state.c2_mode_index = (state.c2_mode_index + direction + 7) % 7;
            break;
        case 3:
            state.c2_workers = step(state.c2_workers, 1U, 64);
            if (state.c2_workers == 0) state.c2_workers = 1;
            break;
        case 4:
            state.c2_rate = step(state.c2_rate, 1000U, 10000000U);
            break;
        case 5:
            state.c2_spoof = !state.c2_spoof;
            break;
        default:
            break;
    }
}

void start_c2_edit(ui::TuiState& state) {
    // Map current cursor to an edit row. Text fields only.
    if (state.c2_focus == ui::C2FocusPanel::ServerControl) {
        if (state.c2_server_cursor == 0) {
            state.edit_row = 100;
            state.edit_buffer = state.c2_port_buffer;
        } else if (state.c2_server_cursor == 1) {
            state.edit_row = 101;
            state.edit_buffer = state.c2_psk_buffer;
        } else {
            return;
        }
    } else if (state.c2_focus == ui::C2FocusPanel::TaskDispatch) {
        if (state.c2_task_cursor == 0) {
            state.edit_row = 200;
            state.edit_buffer = state.c2_target_buffer;
        } else if (state.c2_task_cursor == 1) {
            state.edit_row = 201;
            state.edit_buffer = std::to_string(state.c2_target_port);
        } else if (state.c2_task_cursor == 3) {
            state.edit_row = 203;
            state.edit_buffer = std::to_string(state.c2_workers);
        } else if (state.c2_task_cursor == 4) {
            state.edit_row = 204;
            state.edit_buffer = std::to_string(state.c2_rate);
        } else {
            return;
        }
    } else if (state.c2_focus == ui::C2FocusPanel::Campaigns) {
        if (!state.creating_campaign) return;
        static const std::string* fields[6] = {nullptr};
        (void)fields;
        state.edit_row = 300 + state.campaign_selected_field;
        switch (state.campaign_selected_field) {
            case 0: state.edit_buffer = state.campaign_name_buffer; break;
            case 1: state.edit_buffer = state.campaign_target_buffer; break;
            case 2: state.edit_buffer = std::to_string(state.campaign_target_port); break;
            case 3: state.edit_buffer = std::to_string(state.campaign_workers); break;
            case 4: state.edit_buffer = std::to_string(state.campaign_rate); break;
            case 5: state.edit_buffer = std::to_string(state.campaign_duration); break;
            default: return;
        }
    } else if (state.c2_focus == ui::C2FocusPanel::Malleable) {
        if (!state.creating_malleable) return;
        state.edit_row = 400 + state.malleable_selected_field;
        switch (state.malleable_selected_field) {
            case 0: state.edit_buffer = state.malleable_name_buffer; break;
            case 1: state.edit_buffer = state.malleable_ua_buffer; break;
            case 2: state.edit_buffer = state.malleable_uri_buffer; break;
            case 3: state.edit_buffer = state.malleable_ct_buffer; break;
            default: return;
        }
    } else {
        return;
    }
    state.input_mode = ui::InputMode::Editing;
    state.error_message.clear();
}

bool commit_c2_edit(ui::TuiState& state) {
    const std::string& value = state.edit_buffer;
    switch (state.edit_row) {
        case 100: {
            const auto parsed = parse_u32(value);
            if (!parsed || *parsed == 0 || *parsed > 65535) {
                state.error_message = "Port must be from 1 to 65535.";
                return false;
            }
            state.c2_port_buffer = value;
            break;
        }
        case 101: {
            if (value.size() > 64) {
                state.error_message = "PSK too long (max 64).";
                return false;
            }
            state.c2_psk_buffer = value;
            break;
        }
        case 200: {
            struct in_addr addr{};
            if (inet_pton(AF_INET, value.c_str(), &addr) != 1) {
                state.error_message = "Invalid target IPv4 address.";
                return false;
            }
            state.c2_target_buffer = value;
            break;
        }
        case 201: {
            const auto parsed = parse_u32(value);
            if (!parsed || *parsed == 0 || *parsed > 65535) {
                state.error_message = "Port must be from 1 to 65535.";
                return false;
            }
            state.c2_target_port = static_cast<std::uint16_t>(*parsed);
            break;
        }
        case 203: {
            const auto parsed = parse_u32(value);
            if (!parsed || *parsed == 0 || *parsed > 64) {
                state.error_message = "Workers must be from 1 to 64.";
                return false;
            }
            state.c2_workers = *parsed;
            break;
        }
        case 204: {
            const auto parsed = parse_u32(value);
            if (!parsed) {
                state.error_message = "Rate must be an unsigned integer.";
                return false;
            }
            state.c2_rate = *parsed;
            break;
        }
        case 300: state.campaign_name_buffer = value.substr(0, 64); break;
        case 301: {
            if (!value.empty()) {
                struct in_addr addr{};
                if (inet_pton(AF_INET, value.c_str(), &addr) != 1) {
                    state.error_message = "Invalid campaign target IP.";
                    return false;
                }
            }
            state.campaign_target_buffer = value;
            break;
        }
        case 302: {
            const auto parsed = parse_u32(value);
            if (!parsed || *parsed == 0 || *parsed > 65535) {
                state.error_message = "Port must be from 1 to 65535.";
                return false;
            }
            state.campaign_target_port = static_cast<std::uint16_t>(*parsed);
            break;
        }
        case 303: {
            const auto parsed = parse_u32(value);
            if (!parsed || *parsed == 0 || *parsed > 64) {
                state.error_message = "Workers must be from 1 to 64.";
                return false;
            }
            state.campaign_workers = *parsed;
            break;
        }
        case 304: {
            const auto parsed = parse_u32(value);
            if (!parsed) {
                state.error_message = "Rate must be an unsigned integer.";
                return false;
            }
            state.campaign_rate = *parsed;
            break;
        }
        case 305: {
            const auto parsed = parse_u32(value);
            if (!parsed || *parsed > 86400) {
                state.error_message = "Duration must be 0-86400.";
                return false;
            }
            state.campaign_duration = *parsed;
            break;
        }
        case 400: state.malleable_name_buffer = value.substr(0, 64); break;
        case 401: state.malleable_ua_buffer = value.substr(0, 200); break;
        case 402: state.malleable_uri_buffer = value.substr(0, 200); break;
        case 403: state.malleable_ct_buffer = value.substr(0, 100); break;
        default: return false;
    }
    state.error_message.clear();
    state.input_mode = ui::InputMode::Navigation;
    return true;
}

bool handle_edit_event(ui::ApplicationSnapshot& snapshot, ui::TuiState& state, const Event& event) {
    if (event == Event::Escape) {
        state.input_mode = ui::InputMode::Navigation;
        state.error_message.clear();
        return true;
    }
    if (event == Event::Return) {
        if (state.tui_mode == ui::TuiMode::C2Server && state.edit_row == -1) {
            state.input_mode = ui::InputMode::Navigation;
            state.error_message.clear();
            return true;
        }
        if (state.tui_mode == ui::TuiMode::C2Server && state.edit_row >= 100) {
            commit_c2_edit(state);
            return true;
        }
        commit_edit(snapshot, state);
        return true;
    }
    if (event == Event::Backspace) {
        if (!state.edit_buffer.empty()) state.edit_buffer.pop_back();
        return true;
    }
    if (event.is_character() && event.input().size() == 1) {
        const unsigned char character = static_cast<unsigned char>(event.input()[0]);
        if (character >= 0x20 && character < 0x7f && state.edit_buffer.size() < 64)
            state.edit_buffer.push_back(static_cast<char>(character));
        return true;
    }
    return true;
}

bool handle_modal_event(ui::ApplicationSnapshot& snapshot, ui::TuiState& state,
                        app::ApplicationController& controller, const Event& event) {
    if (event == Event::Escape) {
        close_modal(state);
        return true;
    }
    if (event == Event::Backspace) {
        if (!state.confirm_buffer.empty()) state.confirm_buffer.pop_back();
        return true;
    }
    if (event.is_character() && event.input().size() == 1) {
        const unsigned char character = static_cast<unsigned char>(event.input()[0]);
        if (character >= 0x20 && character < 0x7f && state.confirm_buffer.size() < 16)
            state.confirm_buffer.push_back(static_cast<char>(character));
        return true;
    }
    if (event == Event::Return) {
        if (state.show_launch_confirmation) {
            if (state.confirm_buffer == "YES") {
                controller.launch(snapshot.config);
                refresh_runtime(snapshot, controller.snapshot());
                close_modal(state);
            } else state.error_message = "Type YES exactly to confirm launch.";
        } else if (state.show_reset_confirmation) {
            if (state.confirm_buffer == "RESET") {
                controller.reset();
                snapshot = controller.snapshot();
                close_modal(state);
            } else state.error_message = "Type RESET exactly to confirm.";
        }
        return true;
    }
    return true;
}

int snapshot_main(int width, int height) {
    ui::ApplicationSnapshot snapshot;
    snapshot.config = config::SettingsStore::defaults();
    snapshot.interfaces = common::list_network_interfaces();
    if (!snapshot.interfaces.empty()) {
        snapshot.config.real_ip_interface = snapshot.interfaces.front().name;
    }
    snapshot.settings_path = config::SettingsStore::settings_path();
    snapshot.events.push_back({"00:00:00", ui::Severity::Info, "Snapshot mode"});
    ui::TuiState state;
    state.focus_panel = ui::FocusPanel::Configuration;
    auto document = qevoryx::frontend::render(snapshot, state, width, height);
    auto screen = Screen::Create(Dimension::Fixed(width), Dimension::Fixed(height));
    Render(screen, document);
    std::cout << screen.ToString();
    return 0;
}

void show_loading_screen() {
    auto screen = ScreenInteractive::Fullscreen();
    int frame = 0;
    auto component = Renderer([&] {
        const auto terminal_size = Terminal::Size();
        return view::loading_screen(frame, terminal_size.dimx, terminal_size.dimy);
    });
    component |= CatchEvent([&](Event event) {
        if (event == Event::Custom) {
            if (++frame >= 36) screen.Exit();
            return true;
        }
        if (event == Event::CtrlC || event.is_character()) screen.Exit();
        return true;
    });

    std::atomic<bool> refresh_running{true};
    std::thread refresh_thread([&screen, &refresh_running]() {
        while (refresh_running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(45));
            if (refresh_running.load()) screen.PostEvent(Event::Custom);
        }
    });
    screen.Loop(component);
    refresh_running.store(false);
    refresh_thread.join();
}

} // namespace

namespace qevoryx::frontend {

int run(int argc, char** argv) {
    for (int index = 1; index < argc; ++index) {
        if (std::strcmp(argv[index], "--help") == 0) {
            std::cout << "Usage: qevoryx_tui [--snapshot WIDTH HEIGHT]\n";
            return 0;
        }
        if (std::strcmp(argv[index], "--version") == 0) {
            std::cout << "Qevoryx FTXUI " << common::VERSION << "\n";
            return 0;
        }
        if (std::strcmp(argv[index], "--snapshot") == 0) {
            if (index + 2 >= argc) {
                std::cerr << "--snapshot requires WIDTH and HEIGHT\n";
                return 2;
            }
            const auto width = parse_u32(argv[index + 1]);
            const auto height = parse_u32(argv[index + 2]);
            if (!width || !height || *width < 20 || *height < 10) {
                std::cerr << "Snapshot dimensions must be at least 20x10\n";
                return 2;
            }
            if (*width > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
                *height > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
                std::cerr << "Snapshot dimensions too large\n";
                return 2;
            }
            return snapshot_main(static_cast<int>(*width), static_cast<int>(*height));
        }
    }

    show_loading_screen();

    try {
        auto initialization = std::async(std::launch::async, [] {
            return config::SettingsStore::load().value_or(config::SettingsStore::defaults());
        });
        config::Config config = initialization.get();
        auto controller = app::create_application_controller(std::move(config));
        ui::ApplicationSnapshot snapshot = controller->snapshot();
        ui::TuiState state;
        // Pre-fill server fields from persisted server config (never empty).
        state.c2_port_buffer = std::to_string(snapshot.c2.server_port);
        state.c2_psk_buffer = snapshot.c2.psk;

        auto screen = ScreenInteractive::Fullscreen();
        auto component = Renderer([&] {
            const auto terminal_size = Terminal::Size();
            return qevoryx::frontend::render(
                snapshot, state, terminal_size.dimx, terminal_size.dimy);
        });

        component |= CatchEvent([&](Event event) {
            if (event == Event::Custom) {
                refresh_runtime(snapshot, controller->snapshot());
                return true;
            }

            // Global mode switch: F2 = Local, F3 = C2Server
            if (event == Event::F2) {
                state.tui_mode = ui::TuiMode::Local;
                state.error_message.clear();
                return true;
            }
            if (event == Event::F3) {
#ifdef QEVORYX_ENABLE_C2
                state.tui_mode = ui::TuiMode::C2Server;
                state.error_message.clear();
#else
                state.error_message = "C2 server not included in this build.";
#endif
                return true;
            }

            if (state.input_mode == ui::InputMode::Editing)
                return handle_edit_event(snapshot, state, event);
            if (state.input_mode == ui::InputMode::Modal)
                return handle_modal_event(snapshot, state, *controller, event);

            if (event == Event::CtrlC || event == Event::Character('q') ||
                event == Event::Character('Q')) {
                screen.Exit();
                return true;
            }

            if (event == Event::Escape) {
                if (state.creating_campaign) {
                    state.creating_campaign = false;
                    return true;
                }
                if (state.creating_malleable) {
                    state.creating_malleable = false;
                    return true;
                }
                state.show_help = false;
                state.error_message.clear();
                return true;
            }

            // ── C2 Server Mode keyboard ──
            if (state.tui_mode == ui::TuiMode::C2Server) {
                if (event == Event::Tab || event == Event::TabReverse) {
                    next_c2_panel(state, event == Event::TabReverse);
                    return true;
                }
                if (event == Event::ArrowDown) {
                    if (state.c2_focus == ui::C2FocusPanel::ServerControl) {
                        state.c2_server_cursor =
                            (state.c2_server_cursor + 1) % view::c2_server_cursor_count;
                        sync_server_cursor(state);
                    } else if (state.c2_focus == ui::C2FocusPanel::TaskDispatch) {
                        state.c2_task_cursor =
                            (state.c2_task_cursor + 1) % view::c2_task_cursor_count;
                        sync_task_cursor(state);
                    } else if (state.c2_focus == ui::C2FocusPanel::AgentList) {
                        if (!snapshot.c2.agents.empty())
                            state.c2_selected_agent = (state.c2_selected_agent + 1) %
                                static_cast<int>(snapshot.c2.agents.size());
                    } else if (state.c2_focus == ui::C2FocusPanel::Campaigns) {
                        if (state.creating_campaign) {
                            state.campaign_selected_field = (state.campaign_selected_field + 1) % 6;
                        } else {
                            state.c2_campaign_selected = (state.c2_campaign_selected + 1) % 2;
                        }
                    } else if (state.c2_focus == ui::C2FocusPanel::Malleable) {
                        if (state.creating_malleable) {
                            state.malleable_selected_field = (state.malleable_selected_field + 1) % 4;
                        } else {
                            int total = static_cast<int>(snapshot.c2.malleable_profiles.size()) + 1;
                            if (total > 0)
                                state.c2_malleable_selected = (state.c2_malleable_selected + 1) % total;
                        }
                    }
                    return true;
                }
                if (event == Event::ArrowUp) {
                    if (state.c2_focus == ui::C2FocusPanel::ServerControl) {
                        state.c2_server_cursor =
                            (state.c2_server_cursor + view::c2_server_cursor_count - 1) %
                            view::c2_server_cursor_count;
                        sync_server_cursor(state);
                    } else if (state.c2_focus == ui::C2FocusPanel::TaskDispatch) {
                        state.c2_task_cursor =
                            (state.c2_task_cursor + view::c2_task_cursor_count - 1) %
                            view::c2_task_cursor_count;
                        sync_task_cursor(state);
                    } else if (state.c2_focus == ui::C2FocusPanel::AgentList) {
                        if (!snapshot.c2.agents.empty())
                            state.c2_selected_agent = (state.c2_selected_agent +
                                static_cast<int>(snapshot.c2.agents.size()) - 1) %
                                static_cast<int>(snapshot.c2.agents.size());
                    } else if (state.c2_focus == ui::C2FocusPanel::Campaigns) {
                        if (state.creating_campaign) {
                            state.campaign_selected_field = (state.campaign_selected_field + 5) % 6;
                        } else {
                            state.c2_campaign_selected = (state.c2_campaign_selected + 1) % 2;
                        }
                    } else if (state.c2_focus == ui::C2FocusPanel::Malleable) {
                        if (state.creating_malleable) {
                            state.malleable_selected_field = (state.malleable_selected_field + 3) % 4;
                        } else {
                            int total = static_cast<int>(snapshot.c2.malleable_profiles.size()) + 1;
                            if (total > 0)
                                state.c2_malleable_selected = (state.c2_malleable_selected + total - 1) % total;
                        }
                    }
                    return true;
                }
                if (event == Event::ArrowLeft || event == Event::ArrowRight) {
                    const int dir = event == Event::ArrowRight ? 1 : -1;
                    if (state.c2_focus == ui::C2FocusPanel::TaskDispatch) {
                        if (state.c2_task_cursor <= 5) adjust_c2_config(state, dir);
                    } else if (state.c2_focus == ui::C2FocusPanel::Campaigns && state.creating_campaign) {
                        if (state.campaign_selected_field == 2) {
                            int v = static_cast<int>(state.campaign_target_port) + dir;
                            if (v < 1) v = 1;
                            if (v > 65535) v = 65535;
                            state.campaign_target_port = static_cast<std::uint16_t>(v);
                        }
                    }
                    return true;
                }
                if (event == Event::Character(' ')) {
                    if (state.c2_focus == ui::C2FocusPanel::TaskDispatch) {
                        if (state.c2_task_cursor == 2 || state.c2_task_cursor == 5)
                            adjust_c2_config(state, 1);
                    }
                    return true;
                }
                if (event == Event::Return) {
                    if (!activate_c2_action(snapshot, state, *controller)) {
                        screen.Exit();
                    }
                    return true;
                }
                if (event == Event::Character('?')) {
                    state.show_help = !state.show_help;
                    return true;
                }
                if (event == Event::Character('i') || event == Event::Character('I')) {
                    std::string script = controller->c2_install_script();
                    if (!script.empty()) {
                        state.error_message = "Install script available (check logs)";
                    }
                    return true;
                }
                if (event == Event::Character('g') || event == Event::Character('G')) {
                    if (state.c2_focus == ui::C2FocusPanel::AgentList &&
                        !snapshot.c2.agents.empty()) {
                        state.agent_assign_mode = !state.agent_assign_mode;
                        state.error_message = state.agent_assign_mode
                            ? "Targeted mode: A sends to selected agent."
                            : "Broadcast mode: A sends to idle agents.";
                    }
                    return true;
                }
                if (event == Event::Character('a') || event == Event::Character('A')) {
                    if (state.agent_assign_mode &&
                        state.c2_focus == ui::C2FocusPanel::AgentList &&
                        !snapshot.c2.agents.empty() &&
                        state.c2_selected_agent >= 0 &&
                        state.c2_selected_agent < static_cast<int>(snapshot.c2.agents.size())) {
                        const auto& ag = snapshot.c2.agents[static_cast<std::size_t>(
                            state.c2_selected_agent)];
                        broadcast_current_task(snapshot, state, *controller, ag.id);
                    } else {
                        broadcast_current_task(snapshot, state, *controller);
                    }
                    return true;
                }
                if (event == Event::Character('t') || event == Event::Character('T')) {
                    if (state.agent_assign_mode &&
                        state.c2_focus == ui::C2FocusPanel::AgentList &&
                        !snapshot.c2.agents.empty() &&
                        state.c2_selected_agent >= 0 &&
                        state.c2_selected_agent < static_cast<int>(snapshot.c2.agents.size())) {
                        const auto& ag = snapshot.c2.agents[static_cast<std::size_t>(
                            state.c2_selected_agent)];
                        controller->c2_stop_task(ag.id);
                    } else {
                        controller->c2_stop_task("");
                    }
                    refresh_runtime(snapshot, controller->snapshot());
                    return true;
                }
                if (event == Event::PageUp) {
                    state.event_log_offset += 5;
                    return true;
                }
                if (event == Event::PageDown) {
                    if (state.event_log_offset >= 5) state.event_log_offset -= 5;
                    else state.event_log_offset = 0;
                    return true;
                }
                if (event == Event::Character('x') || event == Event::Character('X')) {
                    if (state.c2_focus == ui::C2FocusPanel::ServerControl) {
                        controller->c2_stop_server();
                        refresh_runtime(snapshot, controller->snapshot());
                    }
                    return true;
                }
                if (event == Event::Character('s') || event == Event::Character('S')) {
                    if (state.c2_focus == ui::C2FocusPanel::ServerControl) {
                        std::uint16_t port = 7777;
                        auto parsed = parse_u32(state.c2_port_buffer);
                        if (parsed && *parsed > 0 && *parsed <= 65535)
                            port = static_cast<std::uint16_t>(*parsed);
                        else {
                            state.error_message = "Invalid port. Enter a value from 1 to 65535.";
                            return true;
                        }
                        controller->c2_start_server(port, state.c2_psk_buffer);
                        refresh_runtime(snapshot, controller->snapshot());
                    }
                    return true;
                }
                return true;
            }

            // ── Local Mode keyboard ──
            if (event == Event::Tab || event == Event::TabReverse) {
                next_panel(state, event == Event::TabReverse);
                return true;
            }
            if (event == Event::ArrowDown) {
                if (state.focus_panel == ui::FocusPanel::Configuration)
                    state.selected_config_row = (state.selected_config_row + 1) % view::config_row_count;
                else if (state.focus_panel == ui::FocusPanel::Actions)
                    state.selected_action = (state.selected_action + 1) % view::action_count;
                else if (state.event_log_offset + view::visible_log_rows < snapshot.events.size())
                    ++state.event_log_offset;
                return true;
            }
            if (event == Event::ArrowUp) {
                if (state.focus_panel == ui::FocusPanel::Configuration)
                    state.selected_config_row =
                        (state.selected_config_row + view::config_row_count - 1) %
                        view::config_row_count;
                else if (state.focus_panel == ui::FocusPanel::Actions)
                    state.selected_action = (state.selected_action + view::action_count - 1) %
                                            view::action_count;
                else if (state.event_log_offset > 0) --state.event_log_offset;
                return true;
            }
            if (event == Event::ArrowLeft || event == Event::ArrowRight) {
                if (state.focus_panel == ui::FocusPanel::Configuration)
                    adjust_config(snapshot, state.selected_config_row,
                                  event == Event::ArrowRight ? 1 : -1);
                return true;
            }
            if (event == Event::Character(' ')) {
                if (state.focus_panel == ui::FocusPanel::Configuration)
                    adjust_config(snapshot, state.selected_config_row, 1);
                return true;
            }
            if (event == Event::Return) {
                if (state.focus_panel == ui::FocusPanel::Configuration) start_edit(snapshot, state);
                else if (state.focus_panel == ui::FocusPanel::Actions) {
                    if (!activate_action(snapshot, state, *controller)) {
                        screen.Exit();
                        return true;
                    }
                    refresh_runtime(snapshot, controller->snapshot());
                }
                return true;
            }
            if (event == Event::Character('?')) {
                state.show_help = !state.show_help;
                return true;
            }
            if (event == Event::Character('l') || event == Event::Character('L')) {
                open_launch_confirmation(state);
                return true;
            }
            if (event == Event::Character('p') || event == Event::Character('P')) {
                if (snapshot.running && snapshot.paused) controller->resume();
                else if (snapshot.running) controller->pause();
                refresh_runtime(snapshot, controller->snapshot());
                return true;
            }
            if (event == Event::Character('r') || event == Event::Character('R')) {
                open_reset_confirmation(state);
                return true;
            }
            if (event == Event::Character('s') || event == Event::Character('S')) {
                if (valid_config(snapshot)) controller->save(snapshot.config);
                else state.error_message = "Cannot save an invalid configuration.";
                refresh_runtime(snapshot, controller->snapshot());
                return true;
            }
            if (event == Event::Character('x') || event == Event::Character('X')) {
                controller->stop();
                refresh_runtime(snapshot, controller->snapshot());
                return true;
            }
            return false;
        });

        screen.TrackMouse(false);
        screen.ForceHandleCtrlC(true);
        std::atomic<bool> refresh_running{true};
        std::thread refresh_thread([&screen, &refresh_running]() {
            while (refresh_running.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                if (refresh_running.load()) screen.PostEvent(Event::Custom);
            }
        });
        screen.Loop(component);
        refresh_running.store(false);
        refresh_thread.join();
        controller->stop();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Fatal unknown error\n";
        return 1;
    }
}

} // namespace qevoryx::frontend
