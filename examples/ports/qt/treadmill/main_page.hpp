#pragma once

// The main screen as one tree, for one look. The three looks differ in their colors and in two
// pictures, so the tree takes the colors of a look and its pictures as parameters. The file of
// a look (main_light.cpp and the others) instantiates it once.

#include "fonts.hpp"
#include "pages.hpp"
#include "parts.hpp"

namespace treadmill {

// `C` is the look's colors (tok.theme.light and the others); `Incline` and `Speed` are the
// pictures that stand out on its ground. `Active` is the look itself, for the buttons.
template <const auto& C, const auto& Incline, const auto& Speed, look Active>
constexpr auto main_screen() {
  constexpr auto unit_font = words_face(tok.text.unit);
  constexpr auto caption_font = words_face(tok.text.caption);
  constexpr auto label_font = words_face(tok.text.label);
  constexpr auto button_font = capitals_face(tok.text.button);
  constexpr auto count_font = digits_face(tok.text.count);
  constexpr auto pace_font = digits_face(tok.text.pace);
  constexpr auto value_font = digits_face(tok.text.value);
  constexpr auto clock_font = digits_face(tok.text.clock);

  return lv::group({.size = {screen_width * px, screen_height * px},
                    .style = {.bg_color = C.white, .bg_opa = tok.solid}})[
      // The ground: the background, the bar at the top, the bar of the buttons, and the lines.
      filled(ground, C.background), filled(nav_bar, C.nav_bar), filled(hairline, tok.line.stroke),
      lv::group({.size = {button_bar.w * px, button_bar.h * px},
                 .style = {.bg_color = C.button_bar,
                           .bg_opa = tok.solid,
                           .border_color = tok.line.stroke,
                           .border_width = tok.line.width,
                           .radius = tok.bar.radius},
                 .x = button_bar.x * px,
                 .y = button_bar.y * px}),
      picture<pictures::wordmark_small>(wordmark.x, wordmark.y),
      filled(divider_left, tok.line.divider), filled(divider_right, tok.line.divider),
      ring(heart_panel), ring(elevation_panel),

      big_button<pictures::start>(start_button, C.start_green, C.black, "START", button_font, &start),
      big_button<pictures::pause>(pause_button, C.pause_gray, C.black, "PAUSE", button_font, &pause),
      big_button<pictures::stop>(stop_button, C.stop_red, C.black, "STOP", button_font, &stop),

      track_view_of<C>(label_font, unit_font, pace_font, clock_font), runner_dot(),

      info_panel<pictures::heart>(heart_panel, "HEART RATE", lv::bind(heart_rate, "{}"), caption_font, count_font,
                                  C.black),
      info_panel<pictures::elevation>(elevation_panel, "ELEVATION", lv::bind(elevation, "{}"), caption_font,
                                      count_font, C.black),
      mode_toggle(mode_switch, C.black, C.secondary),
      at(theme_buttons, look_button<look::light, Active == look::light>(0, "Light", label_font, C.primary, C.secondary),
         look_button<look::dark, Active == look::dark>(1, "Dark", label_font, C.primary, C.secondary),
         look_button<look::sporty, Active == look::sporty>(2, "Sporty", label_font, C.primary, C.secondary)),
      settings_panel<Incline>(incline_panel, "INCLINE", "(%)", lv::bind(incline, "{}"), &flatter, &steeper,
                              value_font, label_font, unit_font, C.primary),
      settings_panel<Speed>(speed_panel, "SPEED", "(MPH)", lv::bind(speed_text), &slower, &faster, value_font,
                            label_font, unit_font, C.primary)];
}

}  // namespace treadmill
