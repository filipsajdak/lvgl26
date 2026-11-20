#pragma once

// The small components the screens are built from. Each is a function that returns a piece of
// tree. A page's file instantiates the ones it uses. Fonts come in as arguments: a font costs
// compile time, and each page makes the sizes it shows.

#include <lv/image.hpp>

#include "images.hpp"
#include "state.hpp"
#include "ui.hpp"

namespace treadmill {

// An empty box at a place, holding the children.
template <class... Child>
constexpr auto at(const box& b, const Child&... children) {
  if constexpr (sizeof...(Child) == 0)
    return lv::group({.size = {b.w * px, b.h * px}, .x = b.x * px, .y = b.y * px});
  else
    return lv::group({.size = {b.w * px, b.h * px}, .x = b.x * px, .y = b.y * px})[children...];
}

// A box with a color.
constexpr auto filled(const box& b, const auto& color) {
  return lv::group({.size = {b.w * px, b.h * px},
                    .style = {.bg_color = color, .bg_opa = tok.solid},
                    .x = b.x * px,
                    .y = b.y * px});
}

// A picture at a place.
template <const auto& Picture>
constexpr auto picture(int x, int y) {
  return lv::image({.src = lv::image_file<Picture>, .x = x * px, .y = y * px});
}

// Text in the middle of a box, on the left of it, or at its top. The text is a string, or a binding.
constexpr auto text_middle(const box& b, const auto& text, const auto& font, const auto& color) {
  return at(b, lv::label({.align = "center", .style = {.text_color = color, .text_font = font}, .text = text}));
}

constexpr auto text_left(const box& b, const auto& text, const auto& font, const auto& color) {
  return at(b, lv::label({.align = "left_mid", .style = {.text_color = color, .text_font = font}, .text = text}));
}

constexpr auto text_top(const box& b, const auto& text, const auto& font, const auto& color) {
  return at(b, lv::label({.align = "top_mid", .style = {.text_color = color, .text_font = font}, .text = text}));
}

// A button with a small arrow on it. A rim of the outer color, and a face that the finger changes.
template <bool Flipped>
constexpr auto arrow_button(const box& b, void (*clicked)()) {
  const auto style = lv::style({.bg_color = tok.arrow.face,
                                .bg_opa = tok.solid,
                                .border_color = tok.arrow.rim,
                                .border_width = tok.arrow.rim_width,
                                .pad_all = tok.space.none,
                                .radius = tok.arrow.radius,
                                .shadow_width = lv::raw(0 * px)});
  const auto pressed = lv::style({.bg_color = tok.arrow.face_down, .border_color = tok.arrow.rim_down});
  if constexpr (Flipped)
    return lv::button({.on_clicked = clicked,
                       .size = {b.w * px, b.h * px},
                       .style = style,
                       .style_pressed = pressed,
                       .x = b.x * px,
                       .y = b.y * px})[
        lv::image({.align = "center", .rotation = 180 * deg, .src = lv::image_file<pictures::arrow>})];
  else
    return lv::button({.on_clicked = clicked,
                       .size = {b.w * px, b.h * px},
                       .style = style,
                       .style_pressed = pressed,
                       .x = b.x * px,
                       .y = b.y * px})[lv::image({.align = "center", .src = lv::image_file<pictures::arrow>})];
}

// START, PAUSE and STOP: a rim of the button's color, and a black face that the finger takes away.
template <const auto& Icon>
constexpr auto big_button(const box& b, const auto& rim, const auto& face, const char* name, const auto& font,
                          void (*clicked)()) {
  return lv::button({.on_clicked = clicked,
                     .size = {b.w * px, b.h * px},
                     .style = {.bg_color = face,
                               .bg_opa = tok.solid,
                               .border_color = rim,
                               .border_width = tok.button.rim_width,
                               .pad_all = tok.space.none,
                               .radius = tok.button.radius,
                               .shadow_width = lv::raw(0 * px)},
                     .style_pressed = {.bg_color = rim},
                     .x = b.x * px,
                     .y = b.y * px})[
      lv::image({.align = "left_mid", .src = lv::image_file<Icon>, .x = button_icon_x * px}),
      lv::label({.align = "left_mid",
                 .style = {.text_color = tok.label.on_button, .text_font = font},
                 .text = name,
                 .x = button_text_x * px})];
}

// The switch between manual and auto: a black track and a handle that sits over the chosen word.
constexpr auto mode_toggle(const box& b, const auto& track, const auto& handle) {
  const auto handle_at = [&](int x, const auto& hidden) {
    return lv::group({.hidden = hidden,
                      .size = {switch_handle.w * px, switch_handle.h * px},
                      .style = {.bg_color = handle, .bg_opa = tok.solid, .radius = tok.toggle.radius},
                      .x = x * px,
                      .y = switch_handle.y * px});
  };
  return lv::button({.on_clicked = &toggle_mode,
                     .size = {b.w * px, b.h * px},
                     .style = {.bg_color = track,
                               .bg_opa = tok.solid,
                               .pad_all = tok.space.none,
                               .radius = tok.toggle.radius,
                               .shadow_width = lv::raw(0 * px)},
                     .x = b.x * px,
                     .y = b.y * px})[
      handle_at(switch_manual_x, lv::bind(auto_mode)), handle_at(switch_auto_x, lv::bind(manual_mode)),
      picture<pictures::manual_mode>(switch_manual_icon_x, switch_icon_y),
      picture<pictures::auto_mode>(switch_auto_icon_x, switch_icon_y)];
}

// One of the three looks. The one that is on has its name dark and a bar under it.
template <look L, bool On>
constexpr auto look_button(int column, const char* name, const auto& font, const auto& on, const auto& off) {
  const auto style = lv::style({.bg_opa = tok.clear, .pad_all = tok.space.none, .shadow_width = lv::raw(0 * px)});
  if constexpr (On)
    return lv::button({.on_clicked = &choose_look<L>,
                       .size = {theme_button_width * px, theme_button_height * px},
                       .style = style,
                       .x = column * theme_button_step * px})[
        text_top(theme_title, name, font, on), filled(theme_marker, on)];
  else
    return lv::button({.on_clicked = &choose_look<L>,
                       .size = {theme_button_width * px, theme_button_height * px},
                       .style = style,
                       .x = column * theme_button_step * px})[text_top(theme_title, name, font, off)];
}

// A reading in a ring: a picture, what it is, and the number.
template <const auto& Icon>
constexpr auto info_panel(const box& b, const char* title, const auto& count, const auto& caption_font,
                          const auto& count_font, const auto& color) {
  return at(b, picture<Icon>(info_icon.x, info_icon.y), text_middle(info_title, title, caption_font, color),
            text_middle(info_count, count, count_font, color));
}

// A setting with an arrow on each side: its picture, its value, its name and its unit.
template <const auto& Icon>
constexpr auto settings_panel(const box& b, const char* title, const char* unit, const auto& value, void (*less)(),
                              void (*more)(), const auto& value_font, const auto& title_font, const auto& unit_font,
                              const auto& color) {
  return at(b, picture<Icon>(settings_icon.x, settings_icon.y), text_middle(settings_value, value, value_font, color),
            text_left(settings_title, title, title_font, color), text_left(settings_unit, unit, unit_font, color),
            arrow_button<false>(arrow_left, less), arrow_button<true>(arrow_right, more));
}

// The ring behind a reading: a pale disc with a red rim.
constexpr auto ring(const box& b) {
  return lv::group({.size = {b.w * px, b.h * px},
                    .style = {.bg_color = tok.ring.fill,
                              .bg_opa = tok.ring.fill_strength,
                              .border_color = tok.ring.color,
                              .border_width = tok.ring.width,
                              .radius = tok.ring.radius},
                    .x = b.x * px,
                    .y = b.y * px});
}

// The track with the time and the pace in it, and the dot that goes round it.
template <const auto& Colors>
constexpr auto track_view_of(const auto& label_font, const auto& unit_font, const auto& pace_font,
                             const auto& clock_font) {
  return at(track_view, picture<pictures::track>(track_picture.x, track_picture.y),
            at(pace_display, text_middle(pace_value, lv::bind(pace_text), pace_font, Colors.primary),
               text_middle(pace_unit, "(MINS/Mi)", unit_font, Colors.primary),
               text_middle(pace_title, "PACE", label_font, Colors.primary)),
            at(time_display, text_top(time_value, lv::bind(time_text), clock_font, Colors.primary),
               text_middle(time_title, "Time", label_font, Colors.primary),
               picture<pictures::clock>(time_icon.x, time_icon.y)));
}

// The dot that goes round the track. It is a child of the screen, not of the track's box, which would
// cut off the half of it that passes the box's edge.
constexpr auto runner_dot() {
  return lv::image({.id = "runner",
                    .src = lv::image_file<pictures::runner>,
                    .x = (track_view.x + runner_start_x) * px,
                    .y = (track_view.y + runner_start_y) * px});
}

}  // namespace treadmill
