#pragma once
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>
#include "theme.hpp"
#include "logo.hpp"

namespace ui {
using namespace ftxui;

struct Config {
  std::string target_ip = "51.79.149.112";
  int target_port = 7777;
  std::string profile = "UDP";
  int workers = 10000;
  std::string source = "Real interface";
  std::string iface = "eth0";
  int payload_min = 512;
  int payload_max = 1400;
};

struct AppState {
  Config config;
  int focus_panel = 0;      // 0=config 1=actions
  int selected_config = 1;
  int selected_action = 0;
  std::string state_text = "READY";
  std::string settings_path = "~/.config/qevoryx/settings.ini";
  bool running = false;
};

// A titled, focus-aware bordered panel.
inline Element panel(const std::string& title, Element body, bool focused) {
  auto b = window(text(" " + title + " ") | bold |
                    color(focused ? theme::Accent() : theme::Secondary()),
                  std::move(body));
  return b | color(focused ? theme::BorderFocus() : theme::Border());
}

inline Element header(const AppState& s, int width) {
  // Emblem sized to available width.
  const auto& art = width >= 110 ? logo::Emblem16
                  : width >= 90  ? logo::Emblem12 : logo::Emblem8;
  auto wordmark = vbox({
    hbox({ text("QEVORY") | bold | color(theme::Primary()),
           text("X")      | bold | color(theme::BrandRed()) }),
    text("Terminal Control Panel") | color(theme::Secondary()),
    text("v4.0.7") | color(theme::Muted()),
  });
  auto status = vbox({
    hbox({ text("● ") | color(theme::Success()),
           text(s.state_text) | bold | color(theme::Success()) }),
    text(s.settings_path) | color(theme::Muted()),
  });
  auto row = hbox({
    render_emblem(art),
    text("  "),
    wordmark,
    filler(),
    separator() | color(theme::Border()),
    text(" "),
    status,
  });
  return window(text(""), row) | color(theme::Border()) | size(HEIGHT, EQUAL, art.size() + 2);
}

inline Element config_panel(const AppState& s, int width) {
  struct Row { std::string label, value, desc; };
  std::vector<Row> rows = {
    {"Target IP", s.config.target_ip, "Destination address"},
    {"Target Port", std::to_string(s.config.target_port), "Destination port"},
    {"Traffic Profile", s.config.profile, "Packet profile"},
    {"Workers", std::to_string(s.config.workers), "Worker threads"},
    {"Source Mode", s.config.source, "Source selection"},
    {"Interface", s.config.iface, "Network interface"},
    {"Payload Minimum", std::to_string(s.config.payload_min), "UDP payload lower bound"},
    {"Payload Maximum", std::to_string(s.config.payload_max), "UDP payload upper bound"},
  };
  Elements lines;
  for (size_t i = 0; i < rows.size(); ++i) {
    bool sel = s.focus_panel == 0 && (int)i == s.selected_config;
    auto marker = text(sel ? "▶ " : "  ") | color(sel ? theme::Accent() : theme::Muted());
    auto label = text(rows[i].label) | color(sel ? theme::Primary() : theme::Secondary())
                   | size(WIDTH, EQUAL, 16);
    auto value = text("[ " + rows[i].value + " ]") | color(sel ? theme::Accent() : theme::Primary())
                   | size(WIDTH, EQUAL, 20);
    // Description only when the panel is wide enough to show it in full.
    Element line;
    if (width >= 96) {
      auto desc = text(rows[i].desc) | color(theme::Muted()) | flex;
      line = hbox({ marker, label, text(" "), value, text("  "), desc });
    } else {
      line = hbox({ marker, label, text(" "), value, filler() });
    }
    if (sel) line = line | bgcolor(theme::SelectionBg());
    lines.push_back(line);
  }
  return panel("CONFIGURATION", vbox(std::move(lines)), s.focus_panel == 0) | flex;
}

inline Element actions_panel(const AppState& s) {
  std::vector<std::string> acts = {"Launch","Save Settings","Reset Defaults","View Logs","Help","Quit"};
  Elements lines;
  for (size_t i = 0; i < acts.size(); ++i) {
    bool sel = s.focus_panel == 1 && (int)i == s.selected_action;
    auto marker = text(sel ? "▶ " : "  ") | color(sel ? theme::Accent() : theme::Muted());
    auto label = text(acts[i]) | color(sel ? theme::Primary() : theme::Secondary()) | bold;
    auto line = hbox({ marker, label, filler() });
    if (sel) line = line | bgcolor(theme::SelectionBg());
    lines.push_back(line);
  }
  return panel("ACTIONS", vbox(std::move(lines)), s.focus_panel == 1)
           | size(WIDTH, EQUAL, 26);
}

inline Element status_panel(const AppState& s) {
  auto dot = [](Color c){ return text("● ") | color(c); };
  auto row1 = hbox({
    dot(theme::Success()), text("Configuration loaded") | color(theme::Secondary()),
    text("    "),
    dot(theme::Success()), text("Settings ready") | color(theme::Secondary()),
    text("    "),
    dot(s.running ? theme::Success() : theme::Accent()),
    text(s.running ? "Running" : "Ready") | color(theme::Secondary()),
  });
  auto summary = text("Target " + s.config.target_ip + ":" + std::to_string(s.config.target_port) +
                      "   Profile " + s.config.profile +
                      "   Workers " + std::to_string(s.config.workers)) | color(theme::Muted());
  auto warn = hbox({ text("⚠ ") | color(theme::Warning()),
                     text("Payload range spans " + std::to_string(s.config.payload_min) +
                          "-" + std::to_string(s.config.payload_max)) | color(theme::Secondary()) });
  return panel("STATUS", vbox({row1, summary, warn}), false);
}

inline Element event_log() {
  struct E { std::string time, level, msg; Color c; };
  std::vector<E> ev = {
    {"04:15:30","INFO ","Qevoryx started", theme::Accent()},
    {"04:15:31","INFO ","Configuration loaded from settings.ini", theme::Accent()},
    {"04:15:32","WARN ","Payload range is fixed", theme::Warning()},
    {"04:15:33","ERROR","Target unreachable (simulated)", theme::Error()},
  };
  Elements lines;
  for (auto& e : ev) {
    lines.push_back(hbox({
      text(e.time) | color(theme::Muted()), text("  "),
      text(e.level) | bold | color(e.c) | size(WIDTH, EQUAL, 6),
      text(" "), text(e.msg) | color(theme::Secondary()),
    }));
  }
  return panel("EVENT LOG", vbox(std::move(lines)), false);
}

inline Element footer() {
  auto seg = [](std::string k, std::string d){
    return hbox({ text(k) | bold | color(theme::Accent()), text(" "),
                  text(d) | color(theme::Muted()), text("   ") });
  };
  return hbox({
    text(" "),
    seg("↑↓","Move"), seg("←→","Change"), seg("Enter","Edit"),
    seg("Tab","Panel"), seg("L","Launch"), seg("S","Save"), seg("?","Help"), seg("Q","Quit"),
  }) | color(theme::Border());
}

inline Element render(const AppState& s, int width) {
  auto body = hbox({ config_panel(s, width), text(" "), actions_panel(s) }) | flex;
  return vbox({
    header(s, width),
    body,
    status_panel(s),
    event_log(),
    footer(),
  }) | bgcolor(theme::Bg());
}

} // namespace ui
