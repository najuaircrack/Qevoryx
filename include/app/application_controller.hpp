#pragma once

#include "config/config.hpp"
#include "ui/application_controller.hpp"

#include <memory>

namespace app {

std::unique_ptr<ui::ApplicationController> create_application_controller(config::Config config);

} // namespace app
