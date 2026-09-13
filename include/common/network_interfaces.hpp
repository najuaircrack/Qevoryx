#pragma once

#include <string>
#include <vector>

namespace common {

struct NetworkInterface {
    std::string name;
    std::string address;
};

std::vector<NetworkInterface> list_network_interfaces();

} // namespace common
