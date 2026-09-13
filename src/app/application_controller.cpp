#include "app/application_controller.hpp"

#include "app/application.hpp"
#include "config/settings_store.hpp"

#include <atomic>
#include <chrono>
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
        log(ui::Severity::Info, "Qevoryx started");
    }

    ~BackendApplicationController() override {
        stop();
    }

    ui::ApplicationSnapshot snapshot() const override {
        std::lock_guard<std::mutex> lock(events_mutex_);

        ui::ApplicationSnapshot result;
        result.config = config_;
        result.running = running_.load();
        result.paused = paused_.load();
        result.ready = !running_.load();
        result.generated = app::Application::generated_packets();
        result.errors = app::Application::error_count();
        result.settings_path = config::SettingsStore::settings_path();
        result.events.assign(events_.begin(), events_.end());
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
        {
            std::lock_guard<std::mutex> lock(events_mutex_);
            config_ = config::SettingsStore::defaults();
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

private:
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

    config::Config config_;
    std::unique_ptr<app::Application> application_;
    std::thread runtime_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};
    mutable std::mutex events_mutex_;
    std::deque<ui::UiEventLogEntry> events_;
};

} // namespace

std::unique_ptr<ApplicationController> create_application_controller(config::Config config) {
    return std::make_unique<BackendApplicationController>(std::move(config));
}

} // namespace app
