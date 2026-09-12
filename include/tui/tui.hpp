#pragma once

#include "config/config.hpp"

#include <optional>

namespace tui {

class Tui {
public:
    static std::optional<config::Config> run();
};

} // namespace tui
