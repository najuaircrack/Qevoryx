#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>
#include <cstring>
#include <iostream>
#include <string>
#include "ui.hpp"

using namespace ftxui;

int main(int argc, char** argv) {
  ui::AppState state;

  // Snapshot mode: render one frame at a fixed size to stdout (for verification).
  int snap_w = 0, snap_h = 0;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--snapshot") == 0 && i + 2 < argc) {
      snap_w = std::atoi(argv[i+1]); snap_h = std::atoi(argv[i+2]);
    }
  }
  if (snap_w > 0) {
    auto doc = ui::render(state, snap_w);
    auto screen = Screen::Create(Dimension::Fixed(snap_w), Dimension::Fixed(snap_h));
    Render(screen, doc);
    std::cout << screen.ToString();
    return 0;
  }

  // Interactive mode.
  auto component = Renderer([&] {
    return ui::render(state, Terminal::Size().dimx);
  });

  component |= CatchEvent([&](Event e) {
    auto& s = state;
    if (e == Event::Character('q') || e == Event::Character('Q')) { return true; }
    if (e == Event::Tab) { s.focus_panel ^= 1; return true; }
    if (e == Event::ArrowDown) {
      if (s.focus_panel == 0) s.selected_config = (s.selected_config + 1) % 8;
      else s.selected_action = (s.selected_action + 1) % 6;
      return true;
    }
    if (e == Event::ArrowUp) {
      if (s.focus_panel == 0) s.selected_config = (s.selected_config + 7) % 8;
      else s.selected_action = (s.selected_action + 5) % 6;
      return true;
    }
    return false;
  });

  auto screen = ScreenInteractive::Fullscreen();
  screen.Loop(component);
  return 0;
}
