#pragma once

// Public, dependency-free HTTP client for the Enterprise operator API.
// Speaks to a remote C2 server (IP/port/bearer token). Contains no C2
// engine code — only HTTP + the documented JSON shapes. Compiles in both
// the open and the enterprise trees.
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace remote {

struct AgentInfo {
    std::string id;
    std::string hostname;
    int status{0};
    std::uint64_t packets_sent{0};
};

struct TaskInfo {
    std::uint32_t id{0};
    int state{0};
    std::string target_ip;
    std::uint16_t target_port{0};
    std::uint64_t total_packets{0};
    std::uint64_t total_errors{0};
};

struct StatusInfo {
    bool running{false};
    std::uint16_t port{0};
    std::size_t agents_connected{0};
    std::size_t agents_busy{0};
    std::uint64_t total_connections{0};
    std::uint32_t uptime_sec{0};
};

struct Result {
    bool ok{false};
    int http_code{0};
    std::string body;
    std::string error;
};

class OperatorClient {
public:
    OperatorClient() = default;
    void configure(std::string host, std::uint16_t port, std::string token,
                   int timeout_ms = 3000);

    Result get_status(StatusInfo& out) const;
    Result get_agents(std::vector<AgentInfo>& out) const;
    Result get_tasks(std::vector<TaskInfo>& out) const;
    // Returns created task ids (empty when no idle agents).
    Result dispatch(const std::string& target_ip, std::uint16_t target_port, int mode,
                    std::uint32_t workers, std::uint32_t rate, std::uint32_t duration,
                    std::vector<std::uint32_t>& task_ids) const;
    Result stop_task(std::uint32_t task_id) const;

private:
    Result request(const std::string& method, const std::string& path,
                   const std::string& body) const;

    std::string host_{"127.0.0.1"};
    std::uint16_t port_{0};
    std::string token_;
    int timeout_ms_{3000};
};

}  // namespace remote
