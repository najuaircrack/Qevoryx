#include "tui/tui.hpp"
#include "common/constants.hpp"
#include "common/platform.hpp"
#include "config/settings_store.hpp"
#include "tui/logo.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>

#if QEVORYX_PLATFORM_WINDOWS
#include <conio.h>
#else
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace tui {
namespace {

enum class Key {
    Up,
    Down,
    Left,
    Right,
    Enter,
    Escape,
    Tab,
    Backspace,
    ControlC,
    Printable,
    Unknown
};

struct KeyEvent {
    Key key{Key::Unknown};
    char character{'\0'};
};

enum class Panel {
    Configuration,
    Actions
};

enum class FieldKind {
    Text,
    Number,
    Choice
};

struct Field {
    const char* label;
    FieldKind kind;
};

struct TerminalSize {
    std::size_t columns{80};
    std::size_t rows{24};
};

struct Layout {
    std::size_t width{80};
    std::size_t left_width{50};
    std::size_t right_width{25};
    bool compact{false};
};

constexpr int field_count = 10;
constexpr int action_count = 4;

#if QEVORYX_PLATFORM_WINDOWS
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

const std::array<Field, field_count> fields = {{
    {"Target IP", FieldKind::Text},
    {"Target port", FieldKind::Number},
    {"Traffic profile", FieldKind::Choice},
    {"Workers", FieldKind::Number},
    {"Source mode", FieldKind::Choice},
    {"Interface", FieldKind::Text},
    {"Payload minimum", FieldKind::Number},
    {"Payload maximum", FieldKind::Number},
    {"Rate limit", FieldKind::Number},
    {"Monitor", FieldKind::Choice},
}};

class TerminalSession {
public:
    TerminalSession() {
#if QEVORYX_PLATFORM_WINDOWS
        console_ = GetStdHandle(STD_OUTPUT_HANDLE);
        if (console_ == INVALID_HANDLE_VALUE || !GetConsoleMode(console_, &original_mode_)) {
            throw std::runtime_error("This terminal does not support the control panel.");
        }

        const DWORD enabled = original_mode_ | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (!SetConsoleMode(console_, enabled)) {
            throw std::runtime_error("This terminal does not support the control panel.");
        }
        mode_changed_ = true;
#else
        if (tcgetattr(STDIN_FILENO, &original_termios_) != 0) {
            throw std::runtime_error("This terminal does not support interactive input.");
        }

        active_termios_ = original_termios_;
        active_termios_.c_lflag &= ~(static_cast<unsigned>(ECHO) | static_cast<unsigned>(ICANON) |
                                     static_cast<unsigned>(ISIG));
        active_termios_.c_iflag &= ~(static_cast<unsigned>(IXON) | static_cast<unsigned>(ICRNL));
        active_termios_.c_cc[VMIN] = 1;
        active_termios_.c_cc[VTIME] = 0;
        raw_mode_ = tcsetattr(STDIN_FILENO, TCSANOW, &active_termios_) == 0;
        if (!raw_mode_) {
            throw std::runtime_error("This terminal does not support keyboard control.");
        }
#endif

        std::cout << "\033[?1049h\033[?25l" << std::flush;
    }

    ~TerminalSession() {
#if QEVORYX_PLATFORM_WINDOWS
        if (mode_changed_) {
            SetConsoleMode(console_, original_mode_);
        }
#else
        if (raw_mode_) {
            tcsetattr(STDIN_FILENO, TCSANOW, &original_termios_);
        }
#endif
        std::cout << "\033[?25h\033[?1049l" << std::flush;
    }

    TerminalSession(const TerminalSession&) = delete;
    TerminalSession& operator=(const TerminalSession&) = delete;

    TerminalSize size() const {
#if QEVORYX_PLATFORM_WINDOWS
        CONSOLE_SCREEN_BUFFER_INFO info {};
        if (GetConsoleScreenBufferInfo(console_, &info)) {
            return {
                static_cast<std::size_t>(std::max<SHORT>(1, info.dwSize.X)),
                static_cast<std::size_t>(std::max<SHORT>(1, info.dwSize.Y))
            };
        }
#else
        winsize window {};
        if (ioctl(STDIN_FILENO, TIOCGWINSZ, &window) == 0 && window.ws_col > 0 && window.ws_row > 0) {
            return {static_cast<std::size_t>(window.ws_col), static_cast<std::size_t>(window.ws_row)};
        }
#endif
        return {};
    }

    KeyEvent read_key() const {
#if QEVORYX_PLATFORM_WINDOWS
        const int first = _getwch();
        if (first == 0 || first == 224) {
            const int code = _getwch();
            switch (code) {
                case 72: return {Key::Up, '\0'};
                case 80: return {Key::Down, '\0'};
                case 75: return {Key::Left, '\0'};
                case 77: return {Key::Right, '\0'};
                default: return {Key::Unknown, '\0'};
            }
        }

        if (first == '\r' || first == '\n') return {Key::Enter, '\0'};
        if (first == 27) return {Key::Escape, '\0'};
        if (first == '\t') return {Key::Tab, '\0'};
        if (first == 8) return {Key::Backspace, '\0'};
        if (first == 3) return {Key::ControlC, '\0'};
        if (first >= 32 && first <= 126) return {Key::Printable, static_cast<char>(first)};
        return {Key::Unknown, '\0'};
#else
        char first = '\0';
        if (read(STDIN_FILENO, &first, 1) != 1) {
            return {Key::ControlC, '\0'};
        }

        if (first != 27) {
            if (first == '\r' || first == '\n') return {Key::Enter, '\0'};
            if (first == '\t') return {Key::Tab, '\0'};
            if (first == 8 || first == 127) return {Key::Backspace, '\0'};
            if (first == 3) return {Key::ControlC, '\0'};
            if (first >= 32 && first <= 126) return {Key::Printable, first};
            return {Key::Unknown, '\0'};
        }

        char next = '\0';
        if (!read_pending(next)) {
            return {Key::Escape, '\0'};
        }
        if (next != '[' && next != 'O') {
            return {Key::Unknown, '\0'};
        }

        char code = '\0';
        if (!read_pending(code)) {
            return {Key::Escape, '\0'};
        }
        switch (code) {
            case 'A': return {Key::Up, '\0'};
            case 'B': return {Key::Down, '\0'};
            case 'C': return {Key::Right, '\0'};
            case 'D': return {Key::Left, '\0'};
            default: return {Key::Unknown, '\0'};
        }
#endif
    }

private:
#if QEVORYX_PLATFORM_WINDOWS
    HANDLE console_{INVALID_HANDLE_VALUE};
    DWORD original_mode_{0};
    bool mode_changed_{false};
#else
    termios original_termios_{};
    termios active_termios_{};
    bool raw_mode_{false};

    bool read_pending(char& value) const {
        termios pending = active_termios_;
        pending.c_lflag &= ~(static_cast<unsigned>(ECHO) | static_cast<unsigned>(ICANON) |
                             static_cast<unsigned>(ISIG));
        pending.c_iflag &= ~(static_cast<unsigned>(IXON) | static_cast<unsigned>(ICRNL));
        pending.c_cc[VMIN] = 0;
        pending.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &pending) != 0) {
            return false;
        }

        const bool received = read(STDIN_FILENO, &value, 1) == 1;
        tcsetattr(STDIN_FILENO, TCSANOW, &active_termios_);
        return received;
    }
#endif
};

Layout make_layout(const TerminalSize& size) {
    const std::size_t width = std::clamp(size.columns, static_cast<std::size_t>(56), static_cast<std::size_t>(120));
    const std::size_t right_width = std::clamp(width / 4, static_cast<std::size_t>(18), static_cast<std::size_t>(30));
    const std::size_t left_width = width - right_width - 5;
    const bool compact = width < 84 || size.rows < 30;
    return {width, left_width, right_width, compact};
}

std::string pad(std::string value, std::size_t width) {
    if (value.size() > width) {
        value.resize(width);
    }
    value.resize(width, ' ');
    return value;
}

std::string repeated(char value, std::size_t count) {
    return std::string(count, value);
}

std::string top_border(const Layout& layout, const std::string& title) {
    const std::size_t decoration_width = layout.width - title.size();
    return "+" + title + " " + repeated('-', decoration_width) + "+";
}

std::string border(const Layout& layout) {
    return "+" + repeated('-', layout.width + 1) + "+";
}

std::string content_line(const Layout& layout, const std::string& left, const std::string& right) {
    return "| " + pad(left, layout.left_width) + "| " + pad(right, layout.right_width) + "|";
}

std::string content_line(const Layout& layout, const std::string& text) {
    return "| " + pad(text, layout.width) + "|";
}

std::optional<std::uint64_t> parse_unsigned(std::string_view value) {
    if (value.empty()) {
        return std::nullopt;
    }

    std::uint64_t result = 0;
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()) {
        return std::nullopt;
    }
    return result;
}

std::string mode_label(config::PacketMode mode) {
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

std::string field_value(const config::Config& config, int index) {
    switch (index) {
        case 0: return config.target_ip;
        case 1: return std::to_string(config.target_port);
        case 2: return mode_label(config.packet_mode);
        case 3: return std::to_string(config.worker_count);
        case 4: return config.use_spoof_ips ? "Spoofed source" : "Real interface";
        case 5: return config.real_ip_interface;
        case 6: return std::to_string(config.payload_min);
        case 7: return std::to_string(config.payload_max);
        case 8: return config.rate_limit == 0 ? "Unlimited" : std::to_string(config.rate_limit) + " per worker";
        case 9: return config.monitor_mode == config::MonitorMode::Console ? "Console" : "Silent";
        default: return {};
    }
}

std::string edit_value(const config::Config& config, int index) {
    if (index == 8 && config.rate_limit == 0) {
        return "0";
    }
    return field_value(config, index);
}

std::string validation_error(const config::Config& config) {
    struct in_addr address {};
    if (inet_pton(AF_INET, config.target_ip.c_str(), &address) != 1) {
        return "Enter a valid IPv4 target.";
    }
    if (config.target_port == 0) {
        return "Target port must be between 1 and 65535.";
    }
    if (config.worker_count == 0 || config.worker_count > static_cast<std::uint32_t>(common::MAX_THREADS)) {
        return "Workers must be between 1 and " + std::to_string(common::MAX_THREADS) + ".";
    }
    if (config.payload_min > config.payload_max || config.payload_max > 1472) {
        return "Payload values must be between 0 and 1472, with minimum no greater than maximum.";
    }
    if (config.real_ip_interface.empty() && !config.use_spoof_ips) {
        return "Enter the interface name for real source selection.";
    }
    return {};
}

std::uint64_t upper_limit(const config::Config& config, int field) {
    switch (field) {
        case 1: return 65535;
        case 3: return static_cast<std::uint64_t>(common::MAX_THREADS);
        case 6:
        case 7: return 1472;
        default:
            (void)config;
            return std::numeric_limits<std::uint32_t>::max();
    }
}

void cycle_choice(config::Config& config, int field, int direction) {
    if (field == 2) {
        const int mode = static_cast<int>(config.packet_mode);
        const int next = (mode + direction + 7) % 7;
        config.packet_mode = static_cast<config::PacketMode>(next);
    } else if (field == 4) {
        config.use_spoof_ips = !config.use_spoof_ips;
    } else if (field == 9) {
        config.monitor_mode = config.monitor_mode == config::MonitorMode::Console
                                  ? config::MonitorMode::Silent
                                  : config::MonitorMode::Console;
    }
}

void adjust_number(config::Config& config, int field, int direction) {
    if (fields[static_cast<std::size_t>(field)].kind != FieldKind::Number) {
        return;
    }

    const std::uint64_t current = [field, &config]() {
        switch (field) {
            case 1: return static_cast<std::uint64_t>(config.target_port);
            case 3: return static_cast<std::uint64_t>(config.worker_count);
            case 6: return static_cast<std::uint64_t>(config.payload_min);
            case 7: return static_cast<std::uint64_t>(config.payload_max);
            default: return static_cast<std::uint64_t>(config.rate_limit);
        }
    }();

    const std::uint64_t maximum = upper_limit(config, field);
    std::uint64_t next = current;
    if (direction > 0) {
        next = current == maximum ? 1 : current + 1;
    } else if (current > 0) {
        next = current - 1;
    }

    switch (field) {
        case 1: config.target_port = static_cast<std::uint16_t>(next); break;
        case 3: config.worker_count = static_cast<std::uint32_t>(next); break;
        case 6: config.payload_min = static_cast<std::uint32_t>(next); break;
        case 7: config.payload_max = static_cast<std::uint32_t>(next); break;
        default: config.rate_limit = static_cast<std::uint32_t>(next); break;
    }
}

bool commit_edit(config::Config& config, int field, const std::string& value, std::string& status) {
    if (fields[static_cast<std::size_t>(field)].kind == FieldKind::Text) {
        if (field == 5 && value.empty()) {
            status = "Interface name cannot be empty.";
            return false;
        }
        if (field == 0) {
            config.target_ip = value;
        } else {
            config.real_ip_interface = value;
        }
        status = "Value updated.";
        return true;
    }

    const auto parsed = parse_unsigned(value);
    if (!parsed || *parsed > upper_limit(config, field)) {
        status = "Enter a valid number for this field.";
        return false;
    }

    switch (field) {
        case 1: config.target_port = static_cast<std::uint16_t>(*parsed); break;
        case 3: config.worker_count = static_cast<std::uint32_t>(*parsed); break;
        case 6: config.payload_min = static_cast<std::uint32_t>(*parsed); break;
        case 7: config.payload_max = static_cast<std::uint32_t>(*parsed); break;
        default: config.rate_limit = static_cast<std::uint32_t>(*parsed); break;
    }
    status = "Value updated.";
    return true;
}

std::string action_label(int index) {
    switch (index) {
        case 0: return "Launch";
        case 1: return "Save settings";
        case 2: return "Reset defaults";
        default: return "Quit";
    }
}

std::string logo_line(std::size_t row, bool compact) {
    std::string result;
    const std::size_t width = compact ? logo::width / 2 : logo::width;
    result.reserve(width * 48);

    for (std::size_t column = 0; column < width; ++column) {
        const std::size_t source_column = compact ? column * 2 : column;
        const std::size_t top_row = compact ? row * 4 : row * 2;
        const std::size_t bottom_row = compact ? row * 4 + 2 : row * 2 + 1;
        const auto& top = logo::pixels[top_row][source_column];
        const auto& bottom = logo::pixels[bottom_row][source_column];
        result += "\033[38;2;" + std::to_string(top[0]) + ';' + std::to_string(top[1]) + ';' +
                  std::to_string(top[2]) + "m\033[48;2;" + std::to_string(bottom[0]) + ';' +
                  std::to_string(bottom[1]) + ';' + std::to_string(bottom[2]) + "m\u2580\033[0m";
    }

    return result;
}

void render(const config::Config& config,
            const Layout& layout,
            Panel panel,
            int selected_field,
            int selected_action,
            bool editing,
            const std::string& edit_buffer,
            const std::string& status) {
    std::ostringstream screen;
    screen << "\033[H";

    screen << top_border(layout, " QEVORYX CONTROL PANEL ") << '\n';
    const std::string settings_path = config::SettingsStore::settings_path();
    const std::size_t logo_rows = layout.compact ? logo::height / 4 : logo::height / 2;
    const std::size_t logo_width = layout.compact ? logo::width / 2 : logo::width;
    for (std::size_t row = 0; row < logo_rows; ++row) {
        std::string right;
        if (row == 0) right = "Qevoryx 4.0.7";
        if (row == 1) right = "Terminal control panel";
        if (row == 4) right = "Settings: " + settings_path;
        if (row == 7) right = "Use this only where you are authorized";
        if (row == 9) right = "Safe defaults: one worker, rate limited";

        screen << "| " << logo_line(row, layout.compact) << ' '
               << pad(right, layout.width - logo_width - 4) << "|\n";
    }

    screen << "+" << repeated('-', layout.left_width + 1)
           << "+" << repeated('-', layout.right_width + 1) << "+\n";

    for (int row = 0; row < field_count; ++row) {
        std::string left;
        if (row == 0) {
            left = " CONFIGURATION";
        } else if (row == 1) {
            left = " ------------------------";
        } else {
            const int field = row - 2;
            const bool selected = panel == Panel::Configuration && selected_field == field;
            const std::string marker = selected ? "> " : "  ";
            const std::string value = selected && editing
                                          ? edit_buffer + "_"
                                          : field_value(config, field);
            left = marker + pad(fields[static_cast<std::size_t>(field)].label, 17) + " " + value;
        }

        std::string right;
        if (row == 0) {
            right = "ACTIONS";
        } else if (row == 1) {
            right = "-------------------------";
        } else if (row - 2 < action_count) {
            const int action = row - 2;
            right = (panel == Panel::Actions && selected_action == action ? "> " : "  ") +
                    action_label(action);
        }

        const bool highlighted = (panel == Panel::Configuration && row >= 2 &&
                                  selected_field == row - 2) ||
                                 (panel == Panel::Actions && row >= 2 && selected_action == row - 2);
        const std::string line = content_line(layout, left, right);
        if (highlighted) {
            screen << "\033[97m" << line << "\033[0m\n";
        } else {
            screen << line << '\n';
        }
    }

    screen << border(layout) << '\n';
    const bool has_error = status.rfind("Enter ", 0) == 0 || status.rfind("Target ", 0) == 0 ||
                           status.rfind("Workers ", 0) == 0 || status.rfind("Payload ", 0) == 0 ||
                           status.rfind("Interface ", 0) == 0;
    if (has_error) {
        screen << "\033[31m" << content_line(layout, status) << "\033[0m\n";
    } else {
        screen << content_line(layout, status) << '\n';
    }

    screen << top_border(layout, " KEYBOARD ") << '\n';
    if (layout.compact) {
        screen << content_line(layout, "Arrows move. Tab switches. Enter edits or activates.") << '\n';
        screen << content_line(layout, "Space cycles. S saves. D resets. Q quits. Ctrl+C stops.") << '\n';
    } else {
        screen << content_line(layout, "Up and Down move. Tab switches panels. Enter edits or activates.") << '\n';
        screen << content_line(layout, "Left and Right change values. Space cycles choices. Esc cancels.") << '\n';
        screen << content_line(layout, "L launches. S saves. D resets. Q quits. Ctrl+C also quits.") << '\n';
    }
    screen << border(layout) << '\n';

    std::cout << screen.str() << std::flush;
}

bool confirm_launch(const config::Config& config, const TerminalSession& terminal, const Layout& layout) {
    std::string answer;
    while (true) {
        std::ostringstream screen;
        screen << "\033[H";
        screen << top_border(layout, " CONFIRM LIVE TRAFFIC ") << '\n';
        screen << content_line(layout, "You are about to generate live network traffic.") << '\n';
        screen << content_line(layout, "Target: " + config.target_ip + ":" + std::to_string(config.target_port)) << '\n';
        screen << content_line(layout, "Profile: " + mode_label(config.packet_mode) + "    Workers: " +
                               std::to_string(config.worker_count)) << '\n';
        screen << content_line(layout, "Source: " +
                               (config.use_spoof_ips ? std::string("spoofed") : config.real_ip_interface)) << '\n';
        screen << content_line(layout, "Confirm that you own this target or have written permission to test it.") << '\n';
        screen << content_line(layout, "Type YES and press Enter. Press Esc to cancel.") << '\n';
        screen << border(layout) << '\n';
        screen << content_line(layout, "Confirmation: " + answer + "_") << '\n';
        screen << border(layout) << '\n';
        std::cout << screen.str() << std::flush;

        const KeyEvent event = terminal.read_key();
        if (event.key == Key::Escape || event.key == Key::ControlC) {
            return false;
        }
        if (event.key == Key::Enter) {
            if (answer == "YES") {
                return true;
            }
            return false;
        }
        if (event.key == Key::Backspace && !answer.empty()) {
            answer.pop_back();
        } else if (event.key == Key::Printable && answer.size() < 16) {
            answer.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(event.character))));
        }
    }
}

} // namespace

std::optional<config::Config> Tui::run() {
    TerminalSession terminal;

    const auto saved = config::SettingsStore::load();
    config::Config config = saved ? *saved : config::SettingsStore::defaults();
    std::string status = saved ? "Saved settings loaded." : "Starting with safe defaults.";

    Panel panel = Panel::Configuration;
    int selected_field = 0;
    int selected_action = 0;
    bool editing = false;
    std::string edit_buffer;

    while (true) {
        const Layout layout = make_layout(terminal.size());
        render(config, layout, panel, selected_field, selected_action, editing, edit_buffer, status);
        const KeyEvent event = terminal.read_key();

        if (event.key == Key::ControlC) {
            return std::nullopt;
        }

        if (editing) {
            if (event.key == Key::Escape) {
                editing = false;
                status = "Edit cancelled.";
            } else if (event.key == Key::Enter) {
                if (commit_edit(config, selected_field, edit_buffer, status)) {
                    editing = false;
                }
            } else if (event.key == Key::Backspace && !edit_buffer.empty()) {
                edit_buffer.pop_back();
            } else if (event.key == Key::Printable && edit_buffer.size() < 64) {
                const bool numeric = fields[static_cast<std::size_t>(selected_field)].kind == FieldKind::Number;
                if (!numeric || std::isdigit(static_cast<unsigned char>(event.character))) {
                    edit_buffer.push_back(event.character);
                }
            }
            continue;
        }

        if (event.key == Key::Tab) {
            panel = panel == Panel::Configuration ? Panel::Actions : Panel::Configuration;
            status = panel == Panel::Configuration ? "Configuration panel focused." : "Actions panel focused.";
        } else if (event.key == Key::Up) {
            if (panel == Panel::Configuration) {
                selected_field = (selected_field + field_count - 1) % field_count;
            } else {
                selected_action = (selected_action + action_count - 1) % action_count;
            }
        } else if (event.key == Key::Down) {
            if (panel == Panel::Configuration) {
                selected_field = (selected_field + 1) % field_count;
            } else {
                selected_action = (selected_action + 1) % action_count;
            }
        } else if (event.key == Key::Left || event.key == Key::Right) {
            if (panel == Panel::Configuration) {
                if (fields[static_cast<std::size_t>(selected_field)].kind == FieldKind::Choice) {
                    cycle_choice(config, selected_field, event.key == Key::Right ? 1 : -1);
                    status = "Choice updated.";
                } else if (fields[static_cast<std::size_t>(selected_field)].kind == FieldKind::Number) {
                    adjust_number(config, selected_field, event.key == Key::Right ? 1 : -1);
                    status = "Value updated.";
                }
            }
        } else if (event.key == Key::Enter || event.key == Key::Printable) {
            char hotkey = '\0';
            if (event.key == Key::Printable) {
                hotkey = static_cast<char>(std::tolower(static_cast<unsigned char>(event.character)));
            }

            if (hotkey == 'l' || hotkey == 's' || hotkey == 'd' || hotkey == 'q') {
                if (hotkey == 'q') {
                    return std::nullopt;
                }
                if (hotkey == 's') {
                    status = config::SettingsStore::save(config)
                                 ? "Settings saved."
                                 : "Could not save settings. Check your config directory permissions.";
                    continue;
                }
                if (hotkey == 'd') {
                    config = config::SettingsStore::defaults();
                    selected_field = 0;
                    status = "Safe defaults restored.";
                    continue;
                }
            } else if (event.key == Key::Printable) {
                if (event.character == ' ') {
                    if (panel == Panel::Configuration &&
                        fields[static_cast<std::size_t>(selected_field)].kind == FieldKind::Choice) {
                        cycle_choice(config, selected_field, 1);
                        status = "Choice updated.";
                    }
                    continue;
                }
                if (event.character == '+' || event.character == '-') {
                    if (panel == Panel::Configuration) {
                        adjust_number(config, selected_field, event.character == '+' ? 1 : -1);
                        status = "Value updated.";
                    }
                    continue;
                }
                if (event.character == 'j' || event.character == 'J') {
                    if (panel == Panel::Configuration) {
                        selected_field = (selected_field + 1) % field_count;
                    } else {
                        selected_action = (selected_action + 1) % action_count;
                    }
                    continue;
                }
                if (event.character == 'k' || event.character == 'K') {
                    if (panel == Panel::Configuration) {
                        selected_field = (selected_field + field_count - 1) % field_count;
                    } else {
                        selected_action = (selected_action + action_count - 1) % action_count;
                    }
                    continue;
                }
                continue;
            }

            if (panel == Panel::Actions) {
                if (selected_action == 0) {
                    const std::string error = validation_error(config);
                    if (!error.empty()) {
                        status = error;
                        continue;
                    }
                    if (!confirm_launch(config, terminal, layout)) {
                        status = "Launch cancelled.";
                        continue;
                    }
                    if (!config::SettingsStore::save(config)) {
                        status = "Settings could not be saved, but the launch was confirmed.";
                    }
                    return config;
                } else if (selected_action == 1) {
                    status = config::SettingsStore::save(config)
                                 ? "Settings saved."
                                 : "Could not save settings. Check your config directory permissions.";
                } else if (selected_action == 2) {
                    config = config::SettingsStore::defaults();
                    selected_field = 0;
                    status = "Safe defaults restored.";
                } else {
                    return std::nullopt;
                }
            } else if (fields[static_cast<std::size_t>(selected_field)].kind == FieldKind::Choice) {
                cycle_choice(config, selected_field, 1);
                status = "Choice updated.";
            } else {
                edit_buffer = edit_value(config, selected_field);
                editing = true;
                status = "Editing. Enter saves, Esc cancels.";
            }
        }
    }
}

} // namespace tui
