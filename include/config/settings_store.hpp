#pragma once

#include "config/config.hpp"

#include <optional>
#include <string>

namespace config {

class SettingsStore {
public:
    static std::optional<Config> load();
    static bool save(const Config& config);
    static std::string settings_path();
    static Config defaults();
};

} // namespace config
