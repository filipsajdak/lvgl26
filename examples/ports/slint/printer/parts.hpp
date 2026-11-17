#pragma once

// The small components the pages are built from. Each is a function that returns a piece of
// tree. A page's file instantiates the ones it uses, so it pays for those alone. Fonts come in
// as arguments: a font costs compile time, and each page makes the sizes it shows.

#include "icons.hpp"
#include "state.hpp"
#include "ui.hpp"

namespace printer {

// A picture of an icon, tinted by a design color.
template <const auto& Icon>
constexpr auto picture(const auto& tint) {
  return lv::image({.align = "center",
                    .src = lv::image_file<Icon>,
                    .style = {.image_recolor = tint, .image_recolor_opa = tok.tint}});
}

// A square button with a picture: back, fewer, more.
template <const auto& Icon>
constexpr auto icon_button(int size, const auto& fill, const auto& tint, void (*clicked)()) {
  return lv::button({.on_clicked = clicked,
                     .size = {size * px, size * px},
                     .style = {.bg_color = fill, .bg_opa = tok.solid, .radius = tok.button.radius,
                               .shadow_width = lv::raw(0 * px)}})[picture<Icon>(tint)];
}

// The header of a sub-page: back, its title, and a line of what to do there.
constexpr auto back_header(const char* title, const char* subtitle, const auto& title_font, const auto& line_font) {
  return lv::group({.size = {100 * pct, header_height * px},
                    .style = {.bg_color = tok.header.background, .bg_opa = tok.solid}})[
      lv::group({.size = {36 * px, 36 * px}, .x = margin * px, .y = 8 * px})[
          icon_button<icons::back>(36, tok.header.background, tok.label.primary, &close_subpage)],
      lv::label({.style = {.text_color = tok.label.primary, .text_font = title_font},
                 .text = title, .x = (margin + 48) * px, .y = 8 * px}),
      lv::label({.style = {.text_color = tok.label.tertiary, .text_font = line_font},
                 .text = subtitle, .x = (margin + 48) * px, .y = 54 * px}),
      lv::group({.size = {100 * pct, 2 * px},
                 .style = {.bg_color = tok.header.separator, .bg_opa = tok.solid},
                 .y = (header_height - 2) * px})];
}

//! [spin_box]
// A number with a button on each side: "Copies  [-] 1 [+]".
template <class Count>
constexpr auto spin_box(const char* name, const Count& count, void (*fewer)(), void (*more)(), const auto& font) {
  return lv::group({.size = {item_width * px, control_height * px}})[
      lv::label({.align = "left_mid", .style = {.text_color = tok.label.primary, .text_font = font}, .text = name}),
      lv::group({.size = {control_height * px, control_height * px}, .x = 90 * px})[
          icon_button<icons::minus>(control_height, tok.spinbox.button, tok.spinbox.text, fewer)],
      lv::label({.align = "left_mid", .style = {.text_color = tok.label.primary, .text_font = font},
                 .text = lv::bind(count, "{}"), .x = 146 * px}),
      lv::group({.size = {control_height * px, control_height * px}, .x = 182 * px})[
          icon_button<icons::plus>(control_height, tok.spinbox.button, tok.spinbox.text, more)]];
}
//! [spin_box]

// A choice from a list, with its name above it.
constexpr auto choice(const char* name, const char* options, const auto& font, int width = item_width,
                      std::string_view id = "") {
  return lv::group({.size = {width * px, choice_height * px}})[
      lv::label({.style = {.text_color = tok.label.primary, .text_font = font}, .text = name}),
      lv::dropdown({.id = id,
                    .options = options,
                    .size = {width * px, 52 * px},
                    .style = {.bg_color = tok.control.background, .bg_opa = tok.solid,
                              .border_color = tok.control.border, .border_width = tok.control.line,
                              .image_recolor = tok.label.primary, .image_recolor_opa = tok.tint,
                              .radius = tok.control.radius, .text_font = font},
                    .symbol = lv::image_file<icons::down>,
                    .y = 26 * px})];
}

// A button with a name that fills its width. `Fill` is a color pair, or one color.
constexpr auto wide_button(const char* name, int width, int height, void (*clicked)(), const auto& from,
                           const auto& to, const auto& font) {
  return lv::button({.on_clicked = clicked,
                     .size = {width * px, height * px},
                     .style = {.bg_color = from, .bg_grad_color = to, .bg_grad_dir = "hor", .bg_opa = tok.solid,
                               .radius = tok.button.radius, .shadow_width = lv::raw(0 * px)}})[
      lv::label({.align = "center", .style = {.text_color = tok.label.on_button, .text_font = font}, .text = name})];
}

}  // namespace printer

namespace printer {

// A group of fonts a page hands to its components.
template <class Title, class Label, class Body>
struct page_fonts {
  Title title;
  Label label;
  Body body;
};

// What Copy and Scan share: a spin box and the color button, four choices, and a start button.
// The page's file owns the state and the functions that change it.
struct job_actions {
  void (*fewer)();
  void (*more)();
  void (*toggle_color)();
  void (*start)();
};

template <class Count, class ColorName, class Fonts>
constexpr auto job_page(const char* title, const Count& count, const ColorName& color_name, const job_actions& act,
                        const char* second_name, const char* second_options, const char* start_name,
                        const Fonts& fonts) {
  constexpr int right = margin + item_width + gap;
  constexpr int row_1 = header_height + margin;
  constexpr int row_2 = row_1 + control_height + gap;
  constexpr int row_3 = row_2 + choice_height + gap;
  constexpr int row_4 = row_3 + choice_height + gap;
  return lv::group({.size = {100 * pct, 100 * pct},
                    .style = {.bg_color = tok.window.background, .bg_opa = tok.solid}})[
      back_header(title, "Choose Settings", fonts.title, fonts.body),
      lv::group({.size = {item_width * px, control_height * px}, .x = margin * px, .y = row_1 * px})[
          spin_box("Copies", count, act.fewer, act.more, fonts.body)],
      lv::button({.on_clicked = act.toggle_color,
                  .size = {item_width * px, control_height * px},
                  .style = {.bg_color = tok.button.color.from, .bg_grad_color = tok.button.color.to,
                            .bg_grad_dir = "hor", .bg_opa = tok.solid, .radius = tok.button.radius,
                            .shadow_width = lv::raw(0 * px)},
                  .x = right * px,
                  .y = row_1 * px})[
          lv::label({.align = "center", .style = {.text_color = tok.label.on_button, .text_font = fonts.body},
                     .text = lv::bind(color_name)})],
      lv::group({.x = margin * px, .y = row_2 * px})[choice("Quality", "Best\nMedium\nDraft", fonts.label)],
      lv::group({.x = right * px, .y = row_2 * px})[choice(second_name, second_options, fonts.label)],
      lv::group({.x = margin * px, .y = row_3 * px})[choice("Paper Size", "Original\nA4", fonts.label)],
      lv::group({.x = right * px, .y = row_3 * px})[choice("Layout", "Portrait\nLandscape", fonts.label)],
      lv::group({.x = margin * px, .y = row_4 * px})[
          wide_button(start_name, page_width - 2 * margin, 60, act.start, tok.button.primary, tok.button.primary,
                      fonts.label)]];
}

}  // namespace printer

namespace printer {

// A button with a picture and a name, side by side: "Pause", "Delete".
template <const auto& Icon>
constexpr auto icon_text_button(std::string_view id, const char* name, void (*clicked)(), const auto& fill,
                                const auto& font) {
  return lv::button({.id = id,
                     .on_clicked = clicked,
                     .size = {110 * px, control_height * px},
                     .style = {.bg_color = fill, .bg_opa = tok.solid, .pad_all = tok.space.none,
                               .radius = tok.button.radius, .shadow_width = lv::raw(0 * px)}})[
      lv::group({.flex_align = {"center", "center", "center"}, .flex_flow = "row", .size = {100 * pct, 100 * pct},
               .style = {.pad_gap = tok.space.gap}})[
          lv::image({.src = lv::image_file<Icon>,
                     .style = {.image_recolor = tok.label.on_button, .image_recolor_opa = tok.tint}}),
          lv::label({.style = {.text_color = tok.label.on_button, .text_font = font}, .text = name})]];
}

}  // namespace printer
