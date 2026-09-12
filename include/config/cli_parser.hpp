#pragma once

#include "config/config.hpp"

namespace config {

class CliParser {
public:
    static Config parse(int argc, char** argv);
    static bool has_tui_flag(int argc, char** argv);
    static void print_usage(const char* program_name);
};

} // namespace config
