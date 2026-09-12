#include "config/cli_parser.hpp"
#include "app/application.hpp"
#include "tui/tui.hpp"
#include <iostream>
#include <exception>
#include <cstring>

int main(int argc, char** argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "--help") == 0) {
                config::CliParser::print_usage(argv[0]);
                return 0;
            }
            if (std::strcmp(argv[i], "--version") == 0) {
                std::cout << "Qevoryx 4.0.7" << std::endl;
                return 0;
            }
        }

        bool explicit_tui = false;
        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "--tui") == 0) {
                explicit_tui = true;
            }
        }

        config::Config config;
        if (config::CliParser::has_cli_flag(argc, argv)) {
            config = config::CliParser::parse(argc, argv);
        } else if (explicit_tui || argc == 1) {
            const auto selected = tui::Tui::run();
            if (!selected) {
                std::cout << "\n  Cancelled.\n" << std::endl;
                return 0;
            }
            config = *selected;
        } else {
            config = config::CliParser::parse(argc, argv);
        }

        app::Application application{std::move(config)};
        return application.run();
    } catch (const std::exception& error) {
        std::cerr << "  Fatal error: " << error.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "  Fatal unknown error" << std::endl;
        return 1;
    }
}
