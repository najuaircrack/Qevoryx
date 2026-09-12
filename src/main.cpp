#include "config/cli_parser.hpp"
#include "app/application.hpp"

int main(int argc, char** argv) {
    auto config = config::CliParser::parse(argc, argv);
    app::Application application{std::move(config)};
    return application.run();
}
