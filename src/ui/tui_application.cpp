#include "ui/tui_application.hpp"

#include "common/platform.hpp"

#include <ncursesw/curses.h>

#include <atomic>
#include <charconv>
#include <chrono>
#include <algorithm>
#include <cstddef>
#include <clocale>
#include <cstdint>
#include <csignal>
#include <cctype>
#include <optional>
#include <string>
#include <system_error>
#include <thread>

namespace ui {
namespace {

std::atomic<bool> g_quit{false};

constexpr int config_row_count = 8;
constexpr int action_count = 6;

void signal_handler(int) {
    g_quit = true;
}

enum class FieldKind {
    String,
    Integer,
    Boolean,
    Enum
};

FieldKind field_kind(int row) {
    switch (row) {
        case 0: return FieldKind::String;
        case 1: return FieldKind::Integer;
        case 2: return FieldKind::Enum;
        case 3: return FieldKind::Integer;
        case 4: return FieldKind::Boolean;
        case 5: return FieldKind::String;
        case 6:
        case 7: return FieldKind::Integer;
        default: return FieldKind::String;
    }
}

std::string config_value(const config::Config& config, int row) {
    switch (row) {
        case 0: return config.target_ip;
        case 1: return std::to_string(config.target_port);
        case 2:
            switch (config.packet_mode) {
                case config::PacketMode::Mixed: return "Mixed";
                case config::PacketMode::Tcp: return "TCP SYN";
                case config::PacketMode::Udp: return "UDP";
                case config::PacketMode::Icmp: return "ICMP";
                case config::PacketMode::Ack: return "TCP ACK";
                case config::PacketMode::Rst: return "TCP RST";
                case config::PacketMode::SynAck: return "TCP SYN ACK";
            }
            return "Mixed";
        case 3: return std::to_string(config.worker_count);
        case 4: return config.use_spoof_ips ? "Spoofed" : "Real interface";
        case 5: return config.real_ip_interface;
        case 6: return std::to_string(config.payload_min);
        case 7: return std::to_string(config.payload_max);
        default: return {};
    }
}

void adjust_enum(config::Config& config, int row, int direction) {
    if (row == 2) {
        const int current = static_cast<int>(config.packet_mode);
        const int next = (current + direction + 7) % 7;
        config.packet_mode = static_cast<config::PacketMode>(next);
    } else if (row == 4) {
        config.use_spoof_ips = direction > 0;
    }
}

void adjust_integer(config::Config& config, int row, int direction) {
    const auto adjust = [direction](std::uint32_t value, std::uint32_t maximum) {
        if (direction > 0) {
            return value == maximum ? 1u : value + 1;
        }
        return value == 0 ? maximum : value - 1;
    };

    switch (row) {
        case 1:
            config.target_port = static_cast<std::uint16_t>(adjust(config.target_port, 65535));
            break;
        case 3:
            config.worker_count = adjust(config.worker_count, 10000);
            break;
        case 6:
            config.payload_min = adjust(config.payload_min, 1472);
            break;
        case 7:
            config.payload_max = adjust(config.payload_max, 1472);
            break;
        default:
            break;
    }
}

bool commit_edit(config::Config& config, int row, const std::string& value) {
    switch (field_kind(row)) {
        case FieldKind::String:
            if (row == 0) config.target_ip = value;
            if (row == 5) config.real_ip_interface = value;
            return true;
        case FieldKind::Integer: {
            std::uint32_t parsed = 0;
            const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
            if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) {
                return false;
            }

            switch (row) {
                case 1:
                    if (parsed == 0 || parsed > 65535) return false;
                    config.target_port = static_cast<std::uint16_t>(parsed);
                    break;
                case 3:
                    if (parsed == 0 || parsed > 10000) return false;
                    config.worker_count = parsed;
                    break;
                case 6:
                case 7:
                    if (parsed > 1472) return false;
                    if (row == 6) config.payload_min = parsed;
                    else config.payload_max = parsed;
                    break;
                default:
                    return false;
            }
            return true;
        }
        case FieldKind::Boolean:
            config.use_spoof_ips = value == "Spoofed";
            return true;
        case FieldKind::Enum:
            return true;
    }
    return false;
}

bool valid_config(const config::Config& config) {
    struct in_addr address {};
    if (inet_pton(AF_INET, config.target_ip.c_str(), &address) != 1) return false;
    if (config.target_port == 0) return false;
    if (config.worker_count == 0 || config.worker_count > 10000) return false;
    if (config.payload_min > config.payload_max || config.payload_max > 1472) return false;
    if (!config.use_spoof_ips && config.real_ip_interface.empty()) return false;
    return true;
}

FocusPanel next_panel(FocusPanel current) {
    switch (current) {
        case FocusPanel::Configuration: return FocusPanel::Actions;
        case FocusPanel::Actions: return FocusPanel::Status;
        case FocusPanel::Status: return FocusPanel::EventLog;
        case FocusPanel::EventLog: return FocusPanel::Configuration;
    }
    return FocusPanel::Configuration;
}

FocusPanel previous_panel(FocusPanel current) {
    switch (current) {
        case FocusPanel::Configuration: return FocusPanel::EventLog;
        case FocusPanel::Actions: return FocusPanel::Configuration;
        case FocusPanel::Status: return FocusPanel::Actions;
        case FocusPanel::EventLog: return FocusPanel::Status;
    }
    return FocusPanel::Configuration;
}

std::optional<TuiEvent> translate_key(int key) {
    switch (key) {
        case KEY_UP: return TuiEvent{TuiEventType::NavigateUp};
        case KEY_DOWN: return TuiEvent{TuiEventType::NavigateDown};
        case KEY_LEFT: return TuiEvent{TuiEventType::NavigateLeft};
        case KEY_RIGHT: return TuiEvent{TuiEventType::NavigateRight};
        case KEY_ENTER:
        case '\n':
        case '\r': return TuiEvent{TuiEventType::Activate};
        case 27: return TuiEvent{TuiEventType::Cancel};
        case ' ': return TuiEvent{TuiEventType::Toggle};
        case '\t': return TuiEvent{TuiEventType::NextPanel};
        case KEY_BTAB: return TuiEvent{TuiEventType::PreviousPanel};
        case 'l':
        case 'L': return TuiEvent{TuiEventType::Launch};
        case 's':
        case 'S': return TuiEvent{TuiEventType::Save};
        case 'd':
        case 'D': return TuiEvent{TuiEventType::Reset};
        case '?': return TuiEvent{TuiEventType::Help};
        case 'q':
        case 'Q': return TuiEvent{TuiEventType::Quit};
        case 'p':
        case 'P': return TuiEvent{TuiEventType::Pause};
        case 'r':
        case 'R': return TuiEvent{TuiEventType::Return};
        case KEY_RESIZE: return TuiEvent{TuiEventType::Resize};
        default: return std::nullopt;
    }
}

std::optional<TuiEvent> translate_editing_key(int key) {
    switch (key) {
        case KEY_ENTER:
        case '\n':
        case '\r': return TuiEvent{TuiEventType::Activate};
        case 27: return TuiEvent{TuiEventType::Cancel};
        case KEY_LEFT: return TuiEvent{TuiEventType::NavigateLeft};
        case KEY_RIGHT: return TuiEvent{TuiEventType::NavigateRight};
        case KEY_BACKSPACE:
        case 8:
        case 127: return TuiEvent{TuiEventType::Backspace};
        case KEY_DC: return TuiEvent{TuiEventType::Delete};
        default:
            if (key >= 32 && key <= 126) {
                return TuiEvent{TuiEventType::Insert, key};
            }
            return std::nullopt;
    }
}

} // namespace

TuiApplication::TuiApplication(ApplicationController& controller)
    : controller_(controller) {}

TuiApplication::~TuiApplication() {
    shutdown();
}

void TuiApplication::initialize() {
    setlocale(LC_ALL, "");
    initscr();
    terminal_initialized_ = true;
    theme_.initialize();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(50);

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    renderer_.initialize();
    layout_.update(state_.screen);
}

void TuiApplication::shutdown() {
    if (!terminal_initialized_) {
        return;
    }

    renderer_.shutdown();
    endwin();
    terminal_initialized_ = false;
}

int TuiApplication::run() {
    config_ = controller_.snapshot().config;
    initialize();

    std::uint64_t last_generated = 0;
    std::uint64_t last_errors = 0;
    bool last_running = false;
    bool last_paused = false;
    std::size_t last_event_count = 0;
    std::string last_event_signature;

    while (state_.running && !g_quit) {
        const auto event = poll_event();
        if (event) {
            process_event(*event);
        }

        const ApplicationSnapshot snapshot = controller_.snapshot();
        std::string event_signature;
        if (!snapshot.events.empty()) {
            event_signature = snapshot.events.back().timestamp + " " + snapshot.events.back().message;
        }

        if (snapshot.generated != last_generated ||
            snapshot.errors != last_errors ||
            snapshot.running != last_running ||
            snapshot.paused != last_paused ||
            snapshot.events.size() != last_event_count ||
            event_signature != last_event_signature) {
            last_generated = snapshot.generated;
            last_errors = snapshot.errors;
            last_running = snapshot.running;
            last_paused = snapshot.paused;
            last_event_count = snapshot.events.size();
            last_event_signature = event_signature;
            state_.runtime_paused = snapshot.paused;
            state_.dirty = true;
        }

        if (state_.dirty) {
            render();
            state_.dirty = false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (controller_.snapshot().running) {
        controller_.stop();
    }

    shutdown();
    return 0;
}

std::optional<TuiEvent> TuiApplication::poll_event() {
    const int key = getch();
    if (key == ERR) {
        return std::nullopt;
    }
    if (state_.input_mode == InputMode::Editing ||
        (state_.input_mode == InputMode::Modal && state_.show_launch_confirmation)) {
        return translate_editing_key(key);
    }
    return translate_key(key);
}

void TuiApplication::process_event(const TuiEvent& event) {
    state_.dirty = true;

    if (event.type == TuiEventType::Resize) {
        layout_.update(state_.screen);
        return;
    }

    if (state_.input_mode == InputMode::Modal) {
        if (state_.show_launch_confirmation) {
            if (event.type == TuiEventType::Insert && state_.confirm_buffer.size() < 3) {
                state_.confirm_buffer.push_back(static_cast<char>(event.value));
            } else if (event.type == TuiEventType::Backspace && !state_.confirm_buffer.empty()) {
                state_.confirm_buffer.pop_back();
            } else if (event.type == TuiEventType::Activate) {
                if (state_.confirm_buffer == "YES") {
                    state_.show_launch_confirmation = false;
                    state_.input_mode = InputMode::Navigation;
                    state_.confirm_buffer.clear();
                    if (valid_config(config_)) {
                        controller_.launch(config_);
                        state_.screen = TuiScreen::Runtime;
                        state_.error_message.clear();
                    } else {
                        state_.error_message = "Configuration is invalid";
                    }
                } else {
                    state_.error_message = "Type YES to confirm launch";
                }
            } else if (event.type == TuiEventType::Cancel) {
                state_.show_launch_confirmation = false;
                state_.input_mode = InputMode::Navigation;
                state_.confirm_buffer.clear();
                state_.error_message.clear();
            }
            return;
        }

        if (event.type == TuiEventType::NavigateLeft ||
            event.type == TuiEventType::NavigateRight ||
            event.type == TuiEventType::Toggle) {
            state_.modal_confirm_selected = !state_.modal_confirm_selected;
        } else if (event.type == TuiEventType::Activate) {
            if (state_.modal_confirm_selected) {
                controller_.reset();
                config_ = controller_.snapshot().config;
                state_.error_message.clear();
            }
            state_.show_reset_confirmation = false;
            state_.input_mode = InputMode::Navigation;
            state_.modal_confirm_selected = false;
        } else if (event.type == TuiEventType::Cancel) {
            state_.show_reset_confirmation = false;
            state_.input_mode = InputMode::Navigation;
            state_.modal_confirm_selected = false;
        }
        return;
    }

    if (state_.input_mode == InputMode::Editing) {
        switch (event.type) {
            case TuiEventType::Activate:
                if (commit_edit(config_, state_.edit_row, state_.edit_buffer)) {
                    state_.input_mode = InputMode::Navigation;
                    state_.error_message.clear();
                } else {
                    state_.error_message = "Invalid value";
                }
                return;
            case TuiEventType::Cancel:
                state_.input_mode = InputMode::Navigation;
                state_.error_message.clear();
                return;
            case TuiEventType::NavigateLeft:
                if (state_.edit_cursor > 0) --state_.edit_cursor;
                return;
            case TuiEventType::NavigateRight:
                if (state_.edit_cursor < static_cast<int>(state_.edit_buffer.size())) ++state_.edit_cursor;
                return;
            case TuiEventType::Insert:
                if (state_.edit_buffer.size() < 128) {
                    state_.edit_buffer.insert(static_cast<std::size_t>(state_.edit_cursor),
                                               1,
                                               static_cast<char>(event.value));
                    ++state_.edit_cursor;
                }
                return;
            case TuiEventType::Backspace:
                if (state_.edit_cursor > 0) {
                    state_.edit_buffer.erase(static_cast<std::size_t>(state_.edit_cursor) - 1, 1);
                    --state_.edit_cursor;
                }
                return;
            case TuiEventType::Delete:
                if (state_.edit_cursor < static_cast<int>(state_.edit_buffer.size())) {
                    state_.edit_buffer.erase(static_cast<std::size_t>(state_.edit_cursor), 1);
                }
                return;
            default:
                return;
        }
    }

    switch (event.type) {
        case TuiEventType::NavigateUp:
            if (state_.focus_panel == FocusPanel::Configuration) {
                state_.selected_config_row = (state_.selected_config_row + config_row_count - 1) % config_row_count;
            } else if (state_.focus_panel == FocusPanel::Actions) {
                state_.selected_action = (state_.selected_action + action_count - 1) % action_count;
            } else if (state_.focus_panel == FocusPanel::EventLog) {
                const auto& events = controller_.snapshot().events;
                const int visible_rows = std::max(layout_.event_log().height - 4, 0);
                const int maximum_offset = std::max(static_cast<int>(events.size()) - visible_rows, 0);
                state_.event_log_offset = std::min(state_.event_log_offset + 1, maximum_offset);
            }
            break;
        case TuiEventType::NavigateDown:
            if (state_.focus_panel == FocusPanel::Configuration) {
                state_.selected_config_row = (state_.selected_config_row + 1) % config_row_count;
            } else if (state_.focus_panel == FocusPanel::Actions) {
                state_.selected_action = (state_.selected_action + 1) % action_count;
            } else if (state_.focus_panel == FocusPanel::EventLog) {
                state_.event_log_offset = std::max(state_.event_log_offset - 1, 0);
            }
            break;
        case TuiEventType::NavigateLeft:
        case TuiEventType::NavigateRight: {
            const int direction = event.type == TuiEventType::NavigateRight ? 1 : -1;
            if (state_.focus_panel == FocusPanel::Configuration) {
                const int row = state_.selected_config_row;
                if (field_kind(row) == FieldKind::Enum || field_kind(row) == FieldKind::Boolean) {
                    adjust_enum(config_, row, direction);
                } else if (field_kind(row) == FieldKind::Integer) {
                    adjust_integer(config_, row, direction);
                }
            }
            break;
        }
        case TuiEventType::Activate:
            if (state_.focus_panel == FocusPanel::Configuration) {
                const int row = state_.selected_config_row;
                if (field_kind(row) == FieldKind::String || field_kind(row) == FieldKind::Integer) {
                    state_.input_mode = InputMode::Editing;
                    state_.edit_row = row;
                    state_.edit_buffer = config_value(config_, row);
                    state_.edit_cursor = static_cast<int>(state_.edit_buffer.size());
                } else {
                    adjust_enum(config_, row, 1);
                }
            } else if (state_.focus_panel == FocusPanel::Actions) {
                switch (state_.selected_action) {
                    case 0:
                        state_.show_launch_confirmation = true;
                        state_.confirm_buffer.clear();
                        state_.input_mode = InputMode::Modal;
                        break;
                    case 1:
                        controller_.save(config_);
                        break;
                    case 2:
                        state_.show_reset_confirmation = true;
                        state_.input_mode = InputMode::Modal;
                        break;
                    case 3:
                        if (layout_.event_log_visible()) {
                            state_.focus_panel = FocusPanel::EventLog;
                            state_.event_log_offset = 0;
                        } else {
                            state_.error_message = "Event log is hidden at this terminal size";
                        }
                        break;
                    case 4:
                        state_.screen = TuiScreen::Help;
                        break;
                    case 5:
                        state_.running = false;
                        break;
                }
            }
            break;
        case TuiEventType::Toggle:
            if (state_.focus_panel == FocusPanel::Configuration) {
                adjust_enum(config_, state_.selected_config_row, 1);
            }
            break;
        case TuiEventType::NextPanel:
            state_.focus_panel = next_panel(state_.focus_panel);
            if (state_.focus_panel == FocusPanel::EventLog && !layout_.event_log_visible()) {
                state_.focus_panel = FocusPanel::Configuration;
            }
            break;
        case TuiEventType::PreviousPanel:
            state_.focus_panel = previous_panel(state_.focus_panel);
            if (state_.focus_panel == FocusPanel::EventLog && !layout_.event_log_visible()) {
                state_.focus_panel = FocusPanel::Actions;
            }
            break;
        case TuiEventType::Launch:
            state_.show_launch_confirmation = true;
            state_.confirm_buffer.clear();
            state_.input_mode = InputMode::Modal;
            break;
        case TuiEventType::Save:
            controller_.save(config_);
            break;
        case TuiEventType::Reset:
            state_.show_reset_confirmation = true;
            state_.input_mode = InputMode::Modal;
            break;
        case TuiEventType::Help:
            state_.screen = TuiScreen::Help;
            break;
        case TuiEventType::Quit:
            state_.running = false;
            break;
        case TuiEventType::Stop:
            controller_.stop();
            state_.screen = TuiScreen::Main;
            state_.runtime_paused = false;
            break;
        case TuiEventType::Pause:
            if (state_.runtime_paused) {
                controller_.resume();
                state_.runtime_paused = false;
            } else {
                controller_.pause();
                state_.runtime_paused = true;
            }
            break;
        case TuiEventType::Resume:
            controller_.resume();
            state_.runtime_paused = false;
            break;
        case TuiEventType::Return:
            state_.screen = TuiScreen::Main;
            break;
        case TuiEventType::Cancel:
            if (state_.screen == TuiScreen::Help || state_.screen == TuiScreen::Runtime) {
                state_.screen = TuiScreen::Main;
            } else {
                state_.error_message.clear();
            }
            break;
        default:
            break;
    }
}

void TuiApplication::render() {
    layout_.update(state_.screen);
    ApplicationSnapshot snapshot = controller_.snapshot();
    snapshot.config = config_;
    renderer_.render(state_, layout_, theme_, snapshot);
}

} // namespace ui
