#pragma once

// Signal: the design system of this project, as a ready theme.
//
//   #include <lv/signal.hpp>
//
//   lv::display display{app, frame, lv::signal::dark};
//
// `dark` is for indoor devices and OLED screens, `day` for screens read in sunlight.
// Both are made by lv::theme from Signal's design tokens (design/signal), so they
// show how to make a theme of your own. The tokens are here too, for your own styles:
//
//   lv::label({.style = {.text_color = lv::signal::dark_tokens.color.muted}})
//
// The theme covers the screen, boxes, buttons, arcs and bars. The other widgets keep
// LVGL's look, and text keeps LVGL's font: Signal's typefaces are not in the library.

#include <lv/theme.hpp>
#include <lv/tokens.hpp>

namespace lv::signal {

namespace detail {

constexpr char dark_json[] = {
#embed "signal/dark.json"
    , 0};
constexpr char day_json[] = {
#embed "signal/day.json"
    , 0};

// Shown or not shown: a switch, not a design value.
inline constexpr auto shown = lv::raw(lv::opa::cover);
inline constexpr auto hidden = lv::raw(lv::opa::transparent);

// Signal with the tokens of one of its themes.
template <const auto& Tok>
consteval lv::theme_styles themed() {
  return lv::theme({
      // A ring shows progress or a reading: square ends, no knob.
      .arc = {.style = {.arc_color = Tok.color.line, .arc_rounded = false, .arc_width = Tok.stroke.track},
              .style_indicator = {.arc_color = Tok.color.active, .arc_rounded = false, .arc_width = Tok.stroke.track},
              .style_knob = {.bg_opa = hidden, .pad_all = Tok.radius._0}},
      .bar = {.style = {.bg_color = Tok.color.line, .bg_opa = shown, .radius = Tok.radius._0},
              .style_indicator = {.bg_color = Tok.color.active, .bg_opa = shown, .radius = Tok.radius._0}},
      // The primary action: a filled block that a finger can hit.
      .button = {.style = {.bg_color = Tok.color.active,
                           .bg_opa = shown,
                           .border_width = Tok.radius._0,
                           .min_height = Tok.size.target,
                           .pad_hor = Tok.space._6,
                           .pad_ver = Tok.space._3,
                           .radius = Tok.radius._0,
                           .shadow_width = Tok.radius._0,
                           .text_color = Tok.color.on_active},
                 .style_pressed = {.bg_color = Tok.color.surface_raised,
                                   .color_filter_opa = hidden,
                                   .text_color = Tok.color.reading,
                                   .transform_height = Tok.radius._0,
                                   .transform_width = Tok.radius._0}},
      // A box lays its children out and draws nothing itself.
      .obj = {.style = {.bg_opa = hidden,
                        .border_width = Tok.radius._0,
                        .pad_all = Tok.space._4,
                        .pad_gap = Tok.space._3,
                        .radius = Tok.radius._0,
                        .shadow_width = Tok.radius._0,
                        .text_color = Tok.color.reading}},
      .screen = {.style = {.bg_color = Tok.color.ground, .bg_opa = shown, .text_color = Tok.color.reading}},
  });
}

}  // namespace detail

inline constexpr auto dark_tokens = lv::tokens<detail::dark_json>;
inline constexpr auto day_tokens = lv::tokens<detail::day_json>;

inline constexpr lv::theme_styles dark = detail::themed<dark_tokens>();
inline constexpr lv::theme_styles day = detail::themed<day_tokens>();

}  // namespace lv::signal
