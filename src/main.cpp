#include "app/application.hpp"
#include "app/application_controller.hpp"
#include "common/constants.hpp"
#include "config/cli_parser.hpp"
#include "config/settings_store.hpp"
#include "ftxui/application.hpp"
#ifdef QEVORYX_ENABLE_C2
#include "server/c2_config.hpp"
#include "server/serve.hpp"
#include "server/server.hpp"
#endif

#include <cstring>
#include <exception>
#include <iostream>
#include <utility>

namespace {

const char* flag_value(int argc, char** argv, const char* flag) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::strcmp(argv[i], flag) == 0) return argv[i + 1];
    }
    return nullptr;
}

bool has_flag(int argc, char** argv, const char* flag) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], flag) == 0) return true;
    }
    return false;
}

void print_server_help(const char* prog) {
    std::cout << "\n  C2 server (headless) usage:\n"
              << "    " << prog << " --serve [--config <server.ini>]\n"
              << "    " << prog << " --export-c2 <dest_dir> [--config <server.ini>]\n"
              << "    " << prog << " --server-status [--config <server.ini>]\n"
              << "    " << prog << " --print-systemd-unit [--bin <path>]\n\n"
              << "  Config: server.ini + c2.psk sidecar (see README). QEVORYX_C2_PSK env overrides.\n\n";
}

}  // namespace

int main(int argc, char** argv) {
    try {
        for (int index = 1; index < argc; ++index) {
            if (std::strcmp(argv[index], "--help") == 0) {
                config::CliParser::print_usage(argv[0]);
                print_server_help(argv[0]);
                return 0;
            }
            if (std::strcmp(argv[index], "--version") == 0) {
                std::cout << "Qevoryx " << common::VERSION << '\n';
                return 0;
            }
        }

        if (has_flag(argc, argv, "--serve") || has_flag(argc, argv, "--export-c2") ||
            has_flag(argc, argv, "--server-status") || has_flag(argc, argv, "--print-systemd-unit")) {
#ifdef QEVORYX_ENABLE_C2
            const char* cfg = flag_value(argc, argv, "--config");
            const std::string cfg_path = cfg ? cfg : "";
            if (has_flag(argc, argv, "--print-systemd-unit")) {
                const char* bin = flag_value(argc, argv, "--bin");
                server::ServerConfig sc = server::ServerConfig::load_or_defaults(cfg_path);
                server::C2Server tmp(sc);
                std::cout << tmp.generate_systemd_unit(bin ? bin : "");
                return 0;
            }
            if (const char* dest = flag_value(argc, argv, "--export-c2")) {
                return server::run_headless(cfg_path, true, dest);
            }
            if (has_flag(argc, argv, "--server-status")) {
                server::ServerConfig sc = server::ServerConfig::load_or_defaults(cfg_path);
                server::C2Server tmp(sc);
                std::cout << tmp.status_json() << "\n";
                return 0;
            }
            return server::run_headless(cfg_path, false, "");
#else
            std::cerr << "This build does not include the C2 server (open-source build).\n";
            return 2;
#endif
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
