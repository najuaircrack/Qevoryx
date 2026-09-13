#pragma once
#include <ftxui/screen/color.hpp>

// QEVORYX brand + semantic palette. Function-local statics avoid the MSVC
// static-init-order fiasco that crashes when Color globals are constructed
// before FTXUI's own translation-unit statics.
namespace theme {
using ftxui::Color;

inline Color BrandWhite() { return Color::RGB(243, 243, 246); }
inline Color BrandRed()   { return Color::RGB(226, 32, 42); }
inline Color BrandShadow(){ return Color::RGB(132, 142, 158); }
inline Color Accent()     { return Color::RGB(56, 196, 222); }
inline Color Primary()    { return Color::RGB(243, 243, 246); }
inline Color Secondary()  { return Color::RGB(178, 184, 196); }
inline Color Muted()      { return Color::RGB(118, 124, 138); }
inline Color Border()     { return Color::RGB(70, 78, 92); }
inline Color BorderFocus(){ return Accent(); }
inline Color Success()    { return Color::RGB(64, 200, 130); }
inline Color Warning()    { return Color::RGB(228, 190, 70); }
inline Color Error()      { return BrandRed(); }
inline Color SelectionBg(){ return Color::RGB(20, 90, 120); }
inline Color SelectionFg(){ return Color::RGB(240, 250, 255); }
inline Color Bg()         { return Color::RGB(12, 13, 16); }
} // namespace theme
