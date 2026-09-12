#pragma once

#include "config/config.hpp"

namespace tui {

class Tui {
public:
    static config::Config run();
    static bool confirm_launch(const config::Config& cfg);

private:
    static void clear_screen();
    static void print_banner();
    static void print_divider();
    static std::string input_string(const std::string& prompt, const std::string& default_val);
    static int input_int(const std::string& prompt, int default_val);
    static bool input_bool(const std::string& prompt, bool default_val);
    static int select_from_list(const std::string& prompt, const std::string* options, int count);
    static std::string mode_to_string(config::PacketMode mode);
};

} // namespace tui
