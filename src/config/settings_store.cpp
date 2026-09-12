#include "config/settings_store.hpp"
#include "common/constants.hpp"
#include "common/platform.hpp"

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace config {
namespace {

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::optional<std::string> environment_value(const char* name) {
#if QEVORYX_PLATFORM_WINDOWS
    char* value = nullptr;
    std::size_t size = 0;
    if (_dupenv_s(&value, &size, name) != 0 || value == nullptr) {
        return std::nullopt;
    }

    std::string result(value);
    std::free(value);
    return result;
#else
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return std::nullopt;
    }
    return std::string(value);
#endif
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

std::optional<std::uint32_t> parse_u32(std::string_view value) {
    const auto parsed = parse_unsigned(value);
    if (!parsed || *parsed > std::numeric_limits<std::uint32_t>::max()) {
        return std::nullopt;
    }
    return static_cast<std::uint32_t>(*parsed);
}

std::optional<std::uint16_t> parse_u16(std::string_view value) {
    const auto parsed = parse_unsigned(value);
    if (!parsed || *parsed > std::numeric_limits<std::uint16_t>::max()) {
        return std::nullopt;
    }
    return static_cast<std::uint16_t>(*parsed);
}

std::string mode_to_string(PacketMode mode) {
    switch (mode) {
        case PacketMode::Mixed: return "mixed";
        case PacketMode::Tcp: return "tcp_syn";
        case PacketMode::Udp: return "udp";
        case PacketMode::Icmp: return "icmp_echo";
        case PacketMode::Ack: return "tcp_ack";
        case PacketMode::Rst: return "tcp_rst";
        case PacketMode::SynAck: return "tcp_syn_ack";
    }
    return "mixed";
}

std::optional<PacketMode> mode_from_string(std::string_view value) {
    if (value == "mixed") return PacketMode::Mixed;
    if (value == "tcp_syn") return PacketMode::Tcp;
    if (value == "udp") return PacketMode::Udp;
    if (value == "icmp_echo") return PacketMode::Icmp;
    if (value == "tcp_ack") return PacketMode::Ack;
    if (value == "tcp_rst") return PacketMode::Rst;
    if (value == "tcp_syn_ack") return PacketMode::SynAck;
    return std::nullopt;
}

std::string monitor_to_string(MonitorMode mode) {
    return mode == MonitorMode::Console ? "console" : "silent";
}

std::optional<MonitorMode> monitor_from_string(std::string_view value) {
    if (value == "console") return MonitorMode::Console;
    if (value == "silent") return MonitorMode::Silent;
    return std::nullopt;
}

bool valid(const Config& config) {
    struct in_addr address {};
    if (inet_pton(AF_INET, config.target_ip.c_str(), &address) != 1) {
        return false;
    }
    if (config.target_port == 0 || config.worker_count == 0 ||
        config.worker_count > static_cast<std::uint32_t>(common::MAX_THREADS)) {
        return false;
    }
    if (config.payload_min > config.payload_max || config.payload_max > 1472) {
        return false;
    }
    return !config.real_ip_interface.empty();
}

std::optional<std::filesystem::path> config_directory() {
#if QEVORYX_PLATFORM_WINDOWS
    const auto app_data = environment_value("APPDATA");
    if (!app_data || app_data->empty()) {
        return std::nullopt;
    }
    return std::filesystem::path(*app_data) / "Qevoryx";
#else
    const auto xdg_config_home = environment_value("XDG_CONFIG_HOME");
    if (xdg_config_home && !xdg_config_home->empty()) {
        return std::filesystem::path(*xdg_config_home) / "qevoryx";
    }

    const auto home = environment_value("HOME");
    if (home && !home->empty()) {
        return std::filesystem::path(*home) / ".config" / "qevoryx";
    }
    return std::nullopt;
#endif
}

std::vector<std::pair<std::string, std::string>> settings_lines(const Config& config) {
    return {
        {"target_ip", config.target_ip},
        {"target_port", std::to_string(config.target_port)},
        {"worker_count", std::to_string(config.worker_count)},
        {"packet_mode", mode_to_string(config.packet_mode)},
        {"monitor_mode", monitor_to_string(config.monitor_mode)},
        {"payload_min", std::to_string(config.payload_min)},
        {"payload_max", std::to_string(config.payload_max)},
        {"rate_limit", std::to_string(config.rate_limit)},
        {"source_mode", config.use_spoof_ips ? "spoofed" : "real"},
        {"real_ip_interface", config.real_ip_interface},
    };
}

} // namespace

Config SettingsStore::defaults() {
    Config config;
    config.target_ip = "127.0.0.1";
    config.target_port = 25565;
    config.worker_count = 1;
    config.packet_mode = PacketMode::Mixed;
    config.monitor_mode = MonitorMode::Console;
    config.payload_min = 0;
    config.payload_max = 512;
    config.rate_limit = 1000;
    config.use_spoof_ips = false;
    config.real_ip_interface = "eth0";
    return config;
}

std::string SettingsStore::settings_path() {
    const auto directory = config_directory();
    if (!directory) {
        return {};
    }
    return (*directory / "settings.ini").string();
}

std::optional<Config> SettingsStore::load() {
    const std::string path = settings_path();
    if (path.empty()) {
        return std::nullopt;
    }

    std::ifstream input(path);
    if (!input) {
        return std::nullopt;
    }

    Config config = defaults();
    std::string line;
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            return std::nullopt;
        }

        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));

        if (key == "target_ip") {
            config.target_ip = value;
        } else if (key == "target_port") {
            const auto parsed = parse_u16(value);
            if (!parsed || *parsed == 0) return std::nullopt;
            config.target_port = *parsed;
        } else if (key == "worker_count") {
            const auto parsed = parse_u32(value);
            if (!parsed || *parsed == 0 || *parsed > static_cast<std::uint32_t>(common::MAX_THREADS)) {
                return std::nullopt;
            }
            config.worker_count = *parsed;
        } else if (key == "packet_mode") {
            const auto parsed = mode_from_string(value);
            if (!parsed) return std::nullopt;
            config.packet_mode = *parsed;
        } else if (key == "monitor_mode") {
            const auto parsed = monitor_from_string(value);
            if (!parsed) return std::nullopt;
            config.monitor_mode = *parsed;
        } else if (key == "payload_min") {
            const auto parsed = parse_u32(value);
            if (!parsed || *parsed > 1472) return std::nullopt;
            config.payload_min = *parsed;
        } else if (key == "payload_max") {
            const auto parsed = parse_u32(value);
            if (!parsed || *parsed > 1472) return std::nullopt;
            config.payload_max = *parsed;
        } else if (key == "rate_limit") {
            const auto parsed = parse_u32(value);
            if (!parsed) return std::nullopt;
            config.rate_limit = *parsed;
        } else if (key == "source_mode") {
            if (value != "real" && value != "spoofed") return std::nullopt;
            config.use_spoof_ips = value == "spoofed";
        } else if (key == "real_ip_interface") {
            if (value.empty()) return std::nullopt;
            config.real_ip_interface = value;
        } else {
            return std::nullopt;
        }
    }

    if (!valid(config)) {
        return std::nullopt;
    }
    return config;
}

bool SettingsStore::save(const Config& config) {
    if (!valid(config)) {
        return false;
    }

    const auto directory = config_directory();
    if (!directory) {
        return false;
    }

    std::error_code error;
    std::filesystem::create_directories(*directory, error);
    if (error) {
        return false;
    }

    std::ofstream output((*directory / "settings.ini").string(), std::ios::trunc);
    if (!output) {
        return false;
    }

    output << "# Qevoryx settings\n";
    for (const auto& setting : settings_lines(config)) {
        output << setting.first << '=' << setting.second << '\n';
    }
    return output.good();
}

} // namespace config
