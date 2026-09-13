#pragma once
#include <ftxui/dom/elements.hpp>
#include "theme.hpp"
#include "logo_data.hpp"

// Render a half-block emblem as a colored FTXUI element.
inline ftxui::Element render_emblem(const std::vector<std::vector<logo::EmblemCell>>& art) {
  using namespace ftxui;
  auto tone = [](logo::Tone t) {
    switch (t) { case logo::L: return theme::BrandWhite();
                 case logo::R: return theme::BrandRed();
                 default: return theme::Bg(); }
  };
  Elements rows;
  for (const auto& row : art) {
    Elements cells;
    for (const auto& c : row) {
      // Upper-half glyph: fg=top tone, bg=bottom tone. Full block: fg only.
      auto e = text(c.glyph);
      if (c.fg != logo::N) e = e | color(tone(c.fg));
      if (c.bg != logo::N) e = e | bgcolor(tone(c.bg));
      cells.push_back(e);
    }
    rows.push_back(hbox(std::move(cells)));
  }
  return vbox(std::move(rows));
}
