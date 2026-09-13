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

bool handle_edit_event(ui::ApplicationSnapshot& snapshot, ui::TuiState& state, const Event& event) {
    if (event == Event::Escape) {
        state.input_mode = ui::InputMode::Navigation;
        state.error_message.clear();
        return true;
    }
    if (event == Event::Return) {
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
            if (event == Event::CtrlC || event == Event::Character('q') ||
                event == Event::Character('Q')) {
                screen.Exit();
                return true;
            }

            if (state.input_mode == ui::InputMode::Editing)
                return handle_edit_event(snapshot, state, event);
            if (state.input_mode == ui::InputMode::Modal)
                return handle_modal_event(snapshot, state, *controller, event);

            if (event == Event::Escape) {
                state.show_help = false;
                state.error_message.clear();
                return true;
            }
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
