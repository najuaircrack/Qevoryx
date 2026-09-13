#pragma once

#include <string>
#include <vector>

namespace common {

struct NetworkInterface {
    std::string name;
    std::string address;
    bool default_route{false};
};

std::vector<NetworkInterface> list_network_interfaces();

} // namespace common
