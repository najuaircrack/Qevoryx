#pragma once

#include "common/network_interfaces.hpp"
#include "config/config.hpp"

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace ui {

enum class TuiScreen {
    Main,
    Runtime,
    Help
};

enum class TuiMode {
    Local,
    C2Server
};

enum class FocusPanel {
    Configuration,
    Actions,
    Status,
    EventLog
};

enum class C2FocusPanel {
    ServerControl,
    AgentList,
    TaskDispatch,
    Campaigns,
    Malleable
};

enum class InputMode {
    Navigation,
    Editing,
    Modal
};

enum class Severity {
    Info,
    Warning,
    Error,
    Success
};

struct UiEventLogEntry {
    std::string timestamp;
    Severity severity{Severity::Info};
    std::string message;
};

struct AgentSnapshot {
    std::string id;
    std::string hostname;
    std::string os;
    std::string status;
    std::uint64_t packets_sent{0};
    float cpu_usage{0.0f};
    float memory_usage{0.0f};
    std::uint32_t uptime_sec{0};
    bool idle{true};
};

struct C2Snapshot {
    bool server_running{false};
    bool remote{false};  // true when driving a remote Enterprise server
    std::string remote_host;
    std::uint16_t server_port{7777};
    std::string psk;
    std::string bind_address{"0.0.0.0"};
    std::vector<AgentSnapshot> agents;
    std::string install_script;
    std::size_t pool_active{0};
    std::size_t pool_max{0};
    std::size_t pool_queue{0};
    std::size_t pool_idle_threads{0};
    std::size_t campaign_active{0};
    std::vector<std::string> campaign_names;
    std::vector<std::string> malleable_profiles;
    std::uint32_t agent_connected{0};
    std::uint32_t agent_busy{0};
    std::uint32_t agent_idle{0};
    std::uint32_t agent_dead{0};
    std::uint64_t total_connections_accepted{0};
    std::uint32_t uptime_sec{0};
    std::uint32_t template_count{0};
    std::vector<std::uint32_t> template_ids;
    bool safety_kill_date_enabled{false};
    std::uint32_t safety_kill_date_epoch{0};
    std::uint32_t safety_max_workers{0};
    std::uint32_t safety_max_rate{0};
};

struct ApplicationSnapshot {
    config::Config config;
    bool running{false};
    bool paused{false};
    bool ready{true};
    std::uint64_t generated{0};
    std::uint64_t errors{0};
    std::string settings_path;
    std::vector<common::NetworkInterface> interfaces;
    std::vector<UiEventLogEntry> events;
    C2Snapshot c2;
};

struct TuiState {
    TuiScreen screen{TuiScreen::Main};
    TuiMode tui_mode{TuiMode::Local};
    FocusPanel focus_panel{FocusPanel::Configuration};
    C2FocusPanel c2_focus{C2FocusPanel::ServerControl};
    InputMode input_mode{InputMode::Navigation};

    int selected_config_row{0};
    int selected_action{0};
    int c2_selected_agent{0};
    int c2_selected_task_action{0};
    // Combined cursor for TaskDispatch: 0-5 fields, 6-8 actions.
    // Kept in sync with c2_selected_field / c2_selected_task_action.
    int c2_task_cursor{0};
    // Combined cursor for ServerControl: 0-1 fields (port/psk), 2-5 actions.
    int c2_server_cursor{0};
    int c2_server_selected_field{0};

    bool show_help{false};
    bool show_reset_confirmation{false};
    bool show_launch_confirmation{false};

    bool running{true};
    bool dirty{true};
    bool runtime_paused{false};

    std::string edit_buffer;
    int edit_cursor{0};
    int edit_row{0};
    int event_log_offset{0};
    bool modal_confirm_selected{false};
    std::string confirm_buffer;

    std::string error_message;

    std::string c2_port_buffer{"7777"};
    std::string c2_psk_buffer;
    // Remote operator connect dialog (F4). Edit rows 500/501/502.
    std::string remote_host_buffer{"127.0.0.1"};
    std::string remote_port_buffer{"8080"};
    std::string remote_token_buffer;
    std::string c2_target_buffer{"192.168.1.100"};
    std::uint16_t c2_target_port{25565};
    int c2_mode_index{0};
    std::uint32_t c2_workers{64};
    std::uint32_t c2_rate{100000};
    bool c2_spoof{true};
    int c2_selected_field{0};
    bool c2_editing_field{false};

    int c2_campaign_selected{0};
    int c2_malleable_selected{0};

    std::string campaign_name_buffer;
    std::string campaign_target_buffer;
    std::uint16_t campaign_target_port{80};
    std::string campaign_malleable_profile{"default"};
    int campaign_mode_index{2};
    std::uint32_t campaign_workers{64};
    std::uint32_t campaign_rate{100000};
    std::uint32_t campaign_duration{60};
    int campaign_selected_field{0};
    bool campaign_editing{false};
    bool creating_campaign{false};

    std::string malleable_name_buffer;
    std::string malleable_ua_buffer;
    std::string malleable_uri_buffer;
    std::string malleable_ct_buffer;
    int malleable_selected_field{0};
    bool malleable_editing{false};
    bool creating_malleable{false};

    std::string agent_group_buffer;
    std::string agent_tag_buffer;
    int agent_assign_selected{0};
    bool agent_assign_mode{false};
};

} // namespace ui
