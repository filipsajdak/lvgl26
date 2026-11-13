#include <lv/lv.hpp>
using namespace lv::literals;
constexpr auto early = lv::style({.radius = lv::raw(8_px)});  // reads the settings
template <>
inline constexpr lv::config lv::cfg<lv::override>{.raw_style_values = true};
