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

class BackendApplicationController final : public ui::ApplicationController {
public:
    explicit BackendApplicationController(config::Config config)
        : config_(std::move(config)) {}

    ~BackendApplicationController() override {
        stop();
    }

    ui::ApplicationSnapshot snapshot() const override {
        std::lock_guard<std::mutex> lock(events_mutex_);

        ui::ApplicationSnapshot result;
        result.config = config_;
        result.running = running_.load();
        result.ready = !running_.load();
        result.generated = app::Application::generated_packets();
        result.errors = app::Application::error_count();
        result.events.assign(events_.begin(), events_.end());
        return result;
    }

    void launch(const config::Config& config) override {
        if (running_.exchange(true)) {
            return;
        }

        config_ = config;
        log(Severity::Info, "Runtime started");

        auto application = std::make_unique<app::Application>(config_);
        auto* raw_application = application.get();

        runtime_thread_ = std::thread([this, raw_application]() {
            std::ostringstream output;
            auto* old_cout = std::cout.rdbuf(output.rdbuf());
            auto* old_cerr = std::cerr.rdbuf(output.rdbuf());

            const int result = raw_application->run();

            std::cout.rdbuf(old_cout);
            std::cerr.rdbuf(old_cerr);

            running_ = false;
            if (result == 0) {
                log(Severity::Success, "Runtime stopped");
            } else {
                log(Severity::Error, "Runtime failed");
            }
        });
    }

    void stop() override {
        if (runtime_thread_.joinable()) {
            app::Application::request_stop();
            runtime_thread_.join();
        }
        running_ = false;
    }

    void save(const config::Config& config) override {
        config_ = config;
        if (config::SettingsStore::save(config_)) {
            log(Severity::Success, "Settings saved");
        } else {
            log(Severity::Error, "Failed to save settings");
        }
    }

    void reset() override {
        config_ = config::SettingsStore::defaults();
        log(Severity::Info, "Settings reset");
    }

    void pause() override {
        stop();
        log(Severity::Warning, "Runtime paused");
    }

    void resume() override {
        launch(config_);
    }

private:
    static std::string timestamp() {
        const auto now = std::chrono::system_clock::now();
        const std::time_t time = std::chrono::system_clock::to_time_t(now);
        std::tm local{};
        localtime_r(&time, &local);

        std::ostringstream stream;
        stream << std::put_time(&local, "%H:%M:%S");
        return stream.str();
    }

    void log(Severity severity, const std::string& message) {
        std::lock_guard<std::mutex> lock(events_mutex_);
        events_.push_back({timestamp(), severity, message});
        if (events_.size() > 100) {
            events_.pop_front();
        }
    }

    config::Config config_;
    std::thread runtime_thread_;
    std::atomic<bool> running_{false};
    mutable std::mutex events_mutex_;
    std::deque<ui::UiEventLogEntry> events_;
};

} // namespace

std::unique_ptr<ui::ApplicationController> create_application_controller(config::Config config) {
    return std::make_unique<BackendApplicationController>(std::move(config));
}

} // namespace app
