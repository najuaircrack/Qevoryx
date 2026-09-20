#include "remote/operator_client.hpp"
#include "common/platform.hpp"

#include <chrono>
#include <cctype>
#include <cstring>
#include <sstream>

#if QEVORYX_PLATFORM_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
namespace {
bool ensure_wsa() {
    static bool init = [] {
        WSADATA wsa{};
        return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
    }();
    return init;
}
}  // namespace
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace remote {
namespace {

std::optional<std::string> jstr(const std::string& b, const char* key) {
    const std::string needle = std::string("\"") + key + "\"";
    auto p = b.find(needle);
    if (p == std::string::npos) return std::nullopt;
    p = b.find(':', p + needle.size());
    if (p == std::string::npos) return std::nullopt;
    ++p;
    while (p < b.size() && std::isspace(static_cast<unsigned char>(b[p]))) ++p;
    if (p >= b.size() || b[p] != '"') return std::nullopt;
    ++p;
    std::string o;
    while (p < b.size() && b[p] != '"') {
        if (b[p] == '\\' && p + 1 < b.size()) {
            ++p;
            o.push_back(b[p++]);
        } else {
            o.push_back(b[p++]);
        }
        if (o.size() > 512) return std::nullopt;
    }
    return o;
}

std::optional<std::uint64_t> jnum(const std::string& b, const char* key) {
    const std::string needle = std::string("\"") + key + "\"";
    auto p = b.find(needle);
    if (p == std::string::npos) return std::nullopt;
    p = b.find(':', p + needle.size());
    if (p == std::string::npos) return std::nullopt;
    ++p;
    while (p < b.size() && std::isspace(static_cast<unsigned char>(b[p]))) ++p;
    if (p < b.size() && b[p] == '"') {  // tolerate quoted numbers
        ++p;
        std::uint64_t v = 0;
        bool any = false;
        while (p < b.size() && b[p] >= '0' && b[p] <= '9') {
            any = true;
            v = v * 10 + static_cast<std::uint64_t>(b[p++] - '0');
        }
        return any ? std::optional<std::uint64_t>(v) : std::nullopt;
    }
    std::uint64_t v = 0;
    bool any = false;
    while (p < b.size() && b[p] >= '0' && b[p] <= '9') {
        any = true;
        v = v * 10 + static_cast<std::uint64_t>(b[p++] - '0');
        if (v > 0xFFFFFFFFFFFFull) return std::nullopt;
    }
    return any ? std::optional<std::uint64_t>(v) : std::nullopt;
}

bool jbool(const std::string& b, const char* key) {
    const std::string needle = std::string("\"") + key + "\"";
    auto p = b.find(needle);
    if (p == std::string::npos) return false;
    return b.find("true", p) != std::string::npos;
}

// Split a top-level JSON array body into its {...} object substrings.
std::vector<std::string> split_objects(const std::string& body, const char* array_key) {
    std::vector<std::string> out;
    const std::string needle = std::string("\"") + array_key + "\"";
    auto p = body.find(needle);
    if (p == std::string::npos) return out;
    p = body.find('[', p);
    if (p == std::string::npos) return out;
    int depth = 0;
    std::size_t start = std::string::npos;
    for (std::size_t i = p; i < body.size(); ++i) {
        if (body[i] == '{') {
            if (depth == 0) start = i;
            ++depth;
        } else if (body[i] == '}') {
            if (--depth == 0 && start != std::string::npos) {
                out.push_back(body.substr(start, i - start + 1));
                start = std::string::npos;
            }
        } else if (body[i] == ']' && depth == 0) {
            break;
        }
    }
    return out;
}

std::vector<std::uint32_t> parse_id_array(const std::string& body) {
    std::vector<std::uint32_t> out;
    auto p = body.find('[');
    if (p == std::string::npos) return out;
    std::uint64_t v = 0;
    bool any = false;
    for (std::size_t i = p; i < body.size(); ++i) {
        if (body[i] >= '0' && body[i] <= '9') {
            any = true;
            v = v * 10 + static_cast<std::uint64_t>(body[i] - '0');
        } else {
            if (any && v <= 0xFFFFFFFFull) out.push_back(static_cast<std::uint32_t>(v));
            any = false;
            v = 0;
            if (body[i] == ']') break;
        }
    }
    return out;
}

}  // namespace

void OperatorClient::configure(std::string host, std::uint16_t port, std::string token,
                               int timeout_ms) {
    host_ = std::move(host);
    port_ = port;
    token_ = std::move(token);
    timeout_ms_ = timeout_ms > 0 ? timeout_ms : 3000;
}

Result OperatorClient::request(const std::string& method, const std::string& path,
                               const std::string& body) const {
    Result r;
#if QEVORYX_PLATFORM_WINDOWS
    if (!ensure_wsa()) {
        r.error = "WSAStartup failed";
        return r;
    }
#endif
    auto sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#if QEVORYX_PLATFORM_WINDOWS
    if (sock == INVALID_SOCKET) {
        r.error = "socket failed";
        return r;
    }
#else
    if (sock < 0) {
        r.error = "socket failed";
        return r;
    }
#endif

#if !QEVORYX_PLATFORM_WINDOWS
    const int fl = ::fcntl(sock, F_GETFL, 0);
    ::fcntl(sock, F_SETFL, fl | O_NONBLOCK);
#endif
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    if (::inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) != 1) {
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }
    ::connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    {
#if QEVORYX_PLATFORM_WINDOWS
        DWORD tv = static_cast<DWORD>(timeout_ms_);
        ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv),
                     sizeof(tv));
        ::setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&tv),
                     sizeof(tv));
#else
        fd_set wfds;
        FD_ZERO(&wfds);
        FD_SET(sock, &wfds);
        struct timeval tv{};
        tv.tv_sec = timeout_ms_ / 1000;
        tv.tv_usec = (timeout_ms_ % 1000) * 1000;
        if (::select(sock + 1, nullptr, &wfds, nullptr, &tv) <= 0) {
            QEVORYX_CLOSESOCK(sock);
            r.error = "connect timeout";
            return r;
        }
        int err = 0;
        socklen_t elen = sizeof(err);
        ::getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &elen);
        if (err != 0) {
            QEVORYX_CLOSESOCK(sock);
            r.error = "connect refused";
            return r;
        }
        ::fcntl(sock, F_SETFL, fl);
        struct timeval rtv{};
        rtv.tv_sec = timeout_ms_ / 1000;
        rtv.tv_usec = (timeout_ms_ % 1000) * 1000;
        ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &rtv, sizeof(rtv));
#endif
    }

    std::ostringstream req;
    req << method << " " << path << " HTTP/1.0\r\n"
        << "Host: " << host_ << "\r\n"
        << "Authorization: Bearer " << token_ << "\r\n"
        << "Connection: close\r\n";
    if (!body.empty()) req << "Content-Type: application/json\r\nContent-Length: " << body.size() << "\r\n";
    req << "\r\n" << body;
    const std::string out = req.str();
    if (::send(sock, out.c_str(), static_cast<int>(out.size()), 0) < 0) {
        QEVORYX_CLOSESOCK(sock);
        r.error = "send failed";
        return r;
    }
    std::string resp;
    char buf[4096];
    for (;;) {
        auto n = ::recv(sock, buf, sizeof(buf), 0);
        if (n <= 0) break;
        resp.append(buf, static_cast<std::size_t>(n));
        if (resp.size() > 65536) break;
    }
    QEVORYX_CLOSESOCK(sock);
    if (resp.size() < 12) {
        r.error = "empty reply";
        return r;
    }
    r.http_code = (resp[9] - '0') * 100 + (resp[10] - '0') * 10 + (resp[11] - '0');
    const auto hdr_end = resp.find("\r\n\r\n");
    r.body = (hdr_end == std::string::npos) ? "" : resp.substr(hdr_end + 4);
    r.ok = (r.http_code == 200);
    if (!r.ok && r.body.empty()) r.error = "http " + std::to_string(r.http_code);
    return r;
}

Result OperatorClient::get_status(StatusInfo& s) const {
    Result r = request("GET", "/api/status", "");
    if (!r.ok) return r;
    s.running = jbool(r.body, "running");
    s.port = static_cast<std::uint16_t>(jnum(r.body, "port").value_or(0));
    s.agents_connected = static_cast<std::size_t>(jnum(r.body, "agents_connected").value_or(0));
    s.agents_busy = static_cast<std::size_t>(jnum(r.body, "agents_busy").value_or(0));
    s.total_connections = jnum(r.body, "total_connections").value_or(0);
    s.uptime_sec = static_cast<std::uint32_t>(jnum(r.body, "uptime_sec").value_or(0));
    return r;
}

Result OperatorClient::get_agents(std::vector<AgentInfo>& out) const {
    out.clear();
    Result r = request("GET", "/api/agents", "");
    if (!r.ok) return r;
    for (const auto& o : split_objects(r.body, "agents")) {
        AgentInfo a;
        a.id = jstr(o, "id").value_or("");
        a.hostname = jstr(o, "hostname").value_or("");
        a.status = static_cast<int>(jnum(o, "status").value_or(0));
        a.packets_sent = jnum(o, "packets_sent").value_or(0);
        if (!a.id.empty()) out.push_back(std::move(a));
    }
    return r;
}

Result OperatorClient::get_tasks(std::vector<TaskInfo>& out) const {
    out.clear();
    Result r = request("GET", "/api/tasks", "");
    if (!r.ok) return r;
    for (const auto& o : split_objects(r.body, "tasks")) {
        TaskInfo t;
        t.id = static_cast<std::uint32_t>(jnum(o, "id").value_or(0));
        t.state = static_cast<int>(jnum(o, "state").value_or(0));
        t.target_ip = jstr(o, "target_ip").value_or("");
        t.target_port = static_cast<std::uint16_t>(jnum(o, "target_port").value_or(0));
        t.total_packets = jnum(o, "total_packets").value_or(0);
        t.total_errors = jnum(o, "total_errors").value_or(0);
        if (t.id != 0) out.push_back(std::move(t));
    }
    return r;
}

Result OperatorClient::dispatch(const std::string& target_ip, std::uint16_t target_port,
                                int mode, std::uint32_t workers, std::uint32_t rate,
                                std::uint32_t duration, std::vector<std::uint32_t>& task_ids) const {
    task_ids.clear();
    std::ostringstream b;
    b << "{\"target_ip\":\"" << target_ip << "\",\"target_port\":" << target_port
      << ",\"packet_mode\":" << mode << ",\"worker_count\":" << workers
      << ",\"rate_limit\":" << rate << ",\"duration_sec\":" << duration << "}";
    Result r = request("POST", "/api/tasks", b.str());
    if (!r.ok) return r;
    task_ids = parse_id_array(r.body);
    return r;
}

Result OperatorClient::stop_task(std::uint32_t task_id) const {
    std::ostringstream b;
    b << "{\"task_id\":" << task_id << "}";
    return request("POST", "/api/tasks/stop", b.str());
}

}  // namespace remote
