#pragma once

#include "ftxui/state.hpp"

#include <ftxui/dom/elements.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace qevoryx::frontend {

using Snapshot = ui::ApplicationSnapshot;
using State = ui::TuiState;

inline constexpr int config_row_count = 9;
inline constexpr int action_count = 8;
inline constexpr std::size_t visible_log_rows = 5;

std::vector<std::string> config_values(const config::Config& config);

ftxui::Element render(const Snapshot& snapshot, const State& state, int width);

} // namespace qevoryx::frontend
