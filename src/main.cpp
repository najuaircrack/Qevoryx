#include "app/application.hpp"
#include "app/application_controller.hpp"
#include "common/constants.hpp"
#include "config/cli_parser.hpp"
#include "config/settings_store.hpp"
#include "ftxui/application.hpp"

#include <cstring>
#include <exception>
#include <iostream>
#include <utility>

int main(int argc, char** argv) {
    try {
        for (int index = 1; index < argc; ++index) {
            if (std::strcmp(argv[index], "--help") == 0) {
                config::CliParser::print_usage(argv[0]);
                return 0;
            }
            if (std::strcmp(argv[index], "--version") == 0) {
                std::cout << "Qevoryx " << common::VERSION << '\n';
                return 0;
            }
        }

        if (config::CliParser::has_cli_flag(argc, argv)) {
            config::Config config = config::CliParser::parse(argc, argv);
            app::Application application{std::move(config)};
            return application.run();
        }

        return qevoryx::frontend::run(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "  Fatal error: " << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "  Fatal unknown error" << '\n';
        return 1;
    }
}
