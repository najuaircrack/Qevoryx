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
inline constexpr int c2_server_action_count = 4;
inline constexpr int c2_task_action_count = 3;
inline constexpr int c2_server_field_count = 2;
inline constexpr int c2_task_field_count = 6;
inline constexpr int c2_task_cursor_count = 9;
inline constexpr int c2_server_cursor_count = 6;
inline constexpr std::size_t visible_log_rows = 5;
inline constexpr std::size_t visible_agent_rows = 6;

std::vector<std::string> config_values(const config::Config& config);

ftxui::Element loading_screen(int frame, int width, int height);

ftxui::Element render(const Snapshot& snapshot, const State& state, int width, int height);

} // namespace qevoryx::frontend
