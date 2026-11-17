#pragma once

// Editor stub of lv/style.hpp.
//
//   constexpr auto card = lv::style({.bg_color = tok.color.card, .radius = tok.radius.card});
//   lv::box({.style = card})
//
// The members of a style are in lv/generated.hpp: lv::stub::style_t.

#include <lv/generated.hpp>
#include <lv/token_types.hpp>
#include <lv/values.hpp>

namespace lv {

namespace detail {
using style_t = stub::style_t;
}  // namespace detail

/// A style value: lv::style({.bg_color = tok.color.card, .radius = tok.radius.card}).
/// Keep it in a constexpr variable to give it a name and reuse it.
constexpr stub::style_t style(stub::style_t s) {
  return s;
}

/// A style for one part in one state, merged into a style with |:
///   .style = base | lv::on("knob", "pressed", {.bg_color = tok.color.active})
/// The names are LVGL's parts (main, indicator, knob, items, cursor, ...) and states
/// (pressed, checked, focused, disabled, ...).
constexpr stub::style_t on(const char*, const char*, stub::style_t s) {
  return s;
}
/// One name: a part in the default state, or a state of the main part.
constexpr stub::style_t on(const char*, stub::style_t s) {
  return s;
}

namespace stub {

/// Two styles merge with |: what the right one sets wins.
constexpr style_t operator|(style_t base, const style_t&) {
  return base;
}
/// A shadow token sets the shadow's color, offsets, width and spread.
constexpr style_t operator|(style_t s, const token<shadow_token>&) {
  return s;
}
/// A border token sets the border's color and width.
constexpr style_t operator|(style_t s, const token<border_token>&) {
  return s;
}
/// A typography token sets the letter spacing. The font comes from an lv::typeface.
constexpr style_t operator|(style_t s, const token<typography_token>&) {
  return s;
}
/// A transition token sets the duration of animations.
constexpr style_t operator|(style_t s, const token<transition_token>&) {
  return s;
}

}  // namespace stub

}  // namespace lv
