#pragma once

#include <array>
#include <cstdint>

namespace tui {
namespace logo {

constexpr std::size_t width = 24;
constexpr std::size_t height = 24;

using Pixel = std::array<std::uint8_t, 3>;
using Row = std::array<Pixel, width>;
using Image = std::array<Row, height>;

extern const Image pixels;

} // namespace logo
} // namespace tui
