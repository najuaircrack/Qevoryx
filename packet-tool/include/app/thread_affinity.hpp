#pragma once

#include <cstdint>

namespace app {

bool pin_current_thread(std::uint32_t cpu_index) noexcept;

} // namespace app
