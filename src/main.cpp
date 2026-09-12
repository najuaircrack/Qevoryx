#include "config/cli_parser.hpp"
#include "app/application.hpp"
#include "tui/tui.hpp"
#include <iostream>
#include <exception>

int main(int argc, char** argv) {
    try {
        config::Config config;

        if (config::CliParser::has_tui_flag(argc, argv)) {
            config = tui::Tui::run();
            if (!tui::Tui::confirm_launch(config)) {
                std::cout << "\n  Aborted.\n" << std::endl;
                return 0;
            }
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
