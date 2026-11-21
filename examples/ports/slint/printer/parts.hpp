#pragma once

// The small components the pages are built from. Each is a function that returns a piece of
// tree. A page's file instantiates the ones it uses, so it pays for those alone. Fonts come in
// as arguments: a font costs compile time, and each page makes the sizes it shows.

#include <lv/image.hpp>

#include "icons.hpp"
#include "state.hpp"
#include "ui.hpp"

namespace printer {

// ---- looks the parts share ----------------------------------------------------------

// A button has no shadow. No token says "no shadow", so the one place that does is here.
constexpr auto no_shadow = lv::raw(0 * px);

// A button in one color: rounded and flat.
constexpr auto flat_look(const auto& fill) {
  return lv::style({.bg_color = fill, .bg_opa = tok.solid, .pad_all = tok.space.none, .radius = tok.button.radius,
                    .shadow_width = no_shadow});
}

// A button that fades from one color to another, from left to right.
constexpr auto across_look(const auto& from, const auto& to) {
  return lv::style({.bg_color = from, .bg_grad_color = to, .bg_grad_dir = "hor", .bg_opa = tok.solid,
                    .pad_all = tok.space.none, .radius = tok.button.radius, .shadow_width = no_shadow});
}

// The box of a choice, with its text in `font`.
constexpr auto dropdown_look(const auto& font) {
  return lv::style({.bg_color = tok.control.background, .bg_opa = tok.solid, .border_color = tok.control.border,
                    .border_width = tok.control.line, .image_recolor = tok.label.primary,
                    .image_recolor_opa = tok.tint, .radius = tok.control.radius, .text_font = font});
}

// A picture of an icon, tinted by a design color.
template <const auto& Icon>
constexpr auto picture(const auto& tint) {
  return lv::image({.align = "center",
                    .src = lv::image_file<Icon>,
                    .style = {.image_recolor = tint, .image_recolor_opa = tok.tint}});
}

// ---- headers ------------------------------------------------------------------------

// The line under a header of `height`.
constexpr auto header_rule(int height) {
  return lv::group({.size = {100 * pct, 2 * px},
                    .y = (height - 2) * px,
                    .style = {.bg_color = tok.header.separator, .bg_opa = tok.solid}});
}

// The look of a header's ground.
constexpr auto header_look = lv::style({.bg_color = tok.header.background, .bg_opa = tok.solid});

// The header of a page that has no way back: its title, and a line under it.
constexpr auto plain_header(const char* title, const auto& title_font) {
  return lv::group({.size = {100 * pct, plain_header_height * px}, .style = header_look})[
      lv::label({.text = title, .x = margin * px,
                 .y = 28 * px, .style = {.text_color = tok.label.primary, .text_font = title_font}}),
      header_rule(plain_header_height)];
}

// A square button with a picture: back, fewer, more.
template <const auto& Icon>
constexpr auto icon_button(int size, const auto& fill, const auto& tint, void (*clicked)()) {
  return lv::button({.size = {size * px, size * px}, .style = flat_look(fill), .on_clicked = clicked})[
      picture<Icon>(tint)];
}

// The header of a sub-page: back, its title, and a line of what to do there.
constexpr auto back_header(const char* title, const char* subtitle, const auto& title_font, const auto& line_font) {
  return lv::group({.size = {100 * pct, header_height * px}, .style = header_look})[
      lv::group({.size = {36 * px, 36 * px}, .x = margin * px, .y = 8 * px})[
          icon_button<icons::back>(36, tok.header.background, tok.label.primary, &close_subpage)],
      lv::label({.text = title,
                 .x = (margin + 48) * px, .y = 8 * px, .style = {.text_color = tok.label.primary, .text_font = title_font}}),
      lv::label({.text = subtitle,
                 .x = (margin + 48) * px, .y = 54 * px, .style = {.text_color = tok.label.tertiary, .text_font = line_font}}),
      header_rule(header_height)];
}

// ---- controls -----------------------------------------------------------------------

//! [spin_box]
// A number with a button on each side: "Copies  [-] 1 [+]".
template <class Count>
constexpr auto spin_box(const char* name, const Count& count, void (*fewer)(), void (*more)(), const auto& font) {
  return lv::group({.flex_align = {"start", "center", "center"}, .flex_flow = "row",
                    .size = {item_width * px, control_height * px}})[
      lv::group({.size = {spin_name_width * px, control_height * px}})[
          lv::label({.align = "left_mid", .text = name,
                     .style = {.text_color = tok.label.primary, .text_font = font}})],
      icon_button<icons::minus>(control_height, tok.spinbox.button, tok.spinbox.text, fewer),
      lv::group({.size = {spin_value_width * px, control_height * px}, .style = {.pad_left = tok.space.card}})[
          lv::label({.align = "left_mid", .text = lv::bind(count, "{}"),
                     .style = {.text_color = tok.label.primary, .text_font = font}})],
      icon_button<icons::plus>(control_height, tok.spinbox.button, tok.spinbox.text, more)];
}
//! [spin_box]

// A choice from a list, with its name above it.
constexpr auto choice(const char* name, const char* options, const auto& font, int width = item_width,
                      std::string_view id = "") {
  return lv::group({.size = {width * px, choice_height * px}})[
      lv::label({.text = name, .style = {.text_color = tok.label.primary, .text_font = font}}),
      lv::dropdown({.id = id,
                    .options = options,
                    .size = {width * px, 52 * px},
                    .symbol = lv::image_file<icons::down>,
                    .y = 26 * px,
                    .style = dropdown_look(font)})];
}

// A button with a name that fills its width, in the colors `from` to `to`.
constexpr auto wide_button(const char* name, int width, int height, void (*clicked)(), const auto& from,
                           const auto& to, const auto& font) {
  return lv::button({.size = {width * px, height * px}, .style = across_look(from, to), .on_clicked = clicked})[
      lv::label({.align = "center", .text = name, .style = {.text_color = tok.label.on_button, .text_font = font}})];
}

// The button that switches between color and grayscale, and says which it is.
template <class ColorName>
constexpr auto color_button(const ColorName& color_name, int width, int height, void (*clicked)(),
                            const auto& font) {
  return lv::button({.size = {width * px, height * px},
                     .style = across_look(tok.button.color.from, tok.button.color.to),
                     .on_clicked = clicked})[
      lv::label({.align = "center", .text = lv::bind(color_name),
                 .style = {.text_color = tok.label.on_button, .text_font = font}})];
}

// A button with a picture and a name, side by side: "Pause", "Delete".
template <const auto& Icon>
constexpr auto icon_text_button(std::string_view id, const char* name, void (*clicked)(), const auto& fill,
                                const auto& font) {
  return lv::button({.id = id,
                     .size = {110 * px, control_height * px},
                     .style = flat_look(fill),
                     .on_clicked = clicked})[
      lv::group({.flex_align = {"center", "center", "center"}, .flex_flow = "row", .size = {100 * pct, 100 * pct},
               .style = {.pad_gap = tok.space.gap}})[
          lv::image({.src = lv::image_file<Icon>,
                     .style = {.image_recolor = tok.label.on_button, .image_recolor_opa = tok.tint}}),
          lv::label({.text = name, .style = {.text_color = tok.label.on_button, .text_font = font}})]];
}

}  // namespace printer
