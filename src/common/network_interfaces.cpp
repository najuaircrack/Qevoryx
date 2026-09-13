#include "common/network_interfaces.hpp"

#include "common/platform.hpp"

#include <algorithm>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>

namespace common {
namespace {

#if QEVORYX_PLATFORM_WINDOWS
std::string wide_to_utf8(const wchar_t* value) {
    if (!value) return {};
    const int length = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0,
                                            nullptr, nullptr);
    if (length <= 0) return {};
    std::string result(static_cast<std::size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), length,
                        nullptr, nullptr);
    result.pop_back();
    return result;
}
#endif

} // namespace

std::vector<NetworkInterface> list_network_interfaces() {
    std::vector<NetworkInterface> interfaces;
    std::set<std::string> seen;

#if QEVORYX_PLATFORM_WINDOWS
    ULONG buffer_size = 15000;
    IP_ADAPTER_ADDRESSES* adapters = nullptr;
    DWORD result = ERROR_BUFFER_OVERFLOW;

    for (int attempt = 0; attempt < 4 && result == ERROR_BUFFER_OVERFLOW; ++attempt) {
        adapters = static_cast<IP_ADAPTER_ADDRESSES*>(std::malloc(buffer_size));
        if (!adapters) return {};

        result = GetAdaptersAddresses(
            AF_INET,
            GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                GAA_FLAG_SKIP_DNS_SERVER,
            nullptr, adapters, &buffer_size);

        if (result == ERROR_BUFFER_OVERFLOW) {
            std::free(adapters);
            adapters = nullptr;
        }
    }

    if (result != NO_ERROR) {
        std::free(adapters);
        return {};
    }

    for (auto* adapter = adapters; adapter; adapter = adapter->Next) {
        const std::string name = wide_to_utf8(adapter->FriendlyName);
        if (name.empty() || !seen.insert(name).second) continue;

        for (auto* address = adapter->FirstUnicastAddress; address;
             address = address->Next) {
            if (address->Address.lpSockaddr->sa_family != AF_INET) continue;

            auto* socket_address =
                reinterpret_cast<struct sockaddr_in*>(address->Address.lpSockaddr);
            char text[INET_ADDRSTRLEN] = {};
            if (inet_ntop(AF_INET, &socket_address->sin_addr, text,
                          sizeof(text))) {
                interfaces.push_back({name, text});
            }
            break;
        }
    }

    std::free(adapters);
#else
    struct ifaddrs* addresses = nullptr;
    if (getifaddrs(&addresses) != 0) return {};

    for (auto* address = addresses; address; address = address->ifa_next) {
        if (!address->ifa_addr || address->ifa_addr->sa_family != AF_INET) continue;

        const std::string name = address->ifa_name ? address->ifa_name : "";
        if (name.empty() || !seen.insert(name).second) continue;

        auto* socket_address =
            reinterpret_cast<struct sockaddr_in*>(address->ifa_addr);
        char text[INET_ADDRSTRLEN] = {};
        if (inet_ntop(AF_INET, &socket_address->sin_addr, text, sizeof(text))) {
            interfaces.push_back({name, text});
        }
    }

    freeifaddrs(addresses);
#endif

    std::sort(interfaces.begin(), interfaces.end(),
              [](const NetworkInterface& left, const NetworkInterface& right) {
                  return left.name < right.name;
              });
    return interfaces;
}

} // namespace common
