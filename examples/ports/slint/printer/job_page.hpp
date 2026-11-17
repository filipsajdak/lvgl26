#pragma once

// The page that Copy and Scan are: a spin box and the color button, four choices, and a start button.

#include "job_options.hpp"
#include "parts.hpp"

namespace printer {

// A group of fonts a page hands to its components.
template <class Title, class Label, class Body>
struct page_fonts {
  Title title;
  Label label;
  Body body;
};

// What Copy and Scan share: a spin box and the color button, four choices, and a start button.
// The state and the functions that change it are `job_options<Kind>`.
template <job_kind Kind, class Fonts>
constexpr auto job_page(const char* title, const char* second_name, const char* second_options,
                        const char* start_name, const Fonts& fonts) {
  using options = job_options<Kind>;
  constexpr int right = margin + item_width + gap;
  constexpr int row_1 = header_height + margin;
  constexpr int row_2 = row_1 + control_height + gap;
  constexpr int row_3 = row_2 + choice_height + gap;
  constexpr int row_4 = row_3 + choice_height + gap;
  return lv::group({.size = {100 * pct, 100 * pct},
                    .style = {.bg_color = tok.window.background, .bg_opa = tok.solid}})[
      back_header(title, "Choose Settings", fonts.title, fonts.body),
      lv::group({.size = {item_width * px, control_height * px}, .x = margin * px, .y = row_1 * px})[
          spin_box("Copies", options::count, &options::fewer, &options::more, fonts.body)],
      lv::group({.x = right * px, .y = row_1 * px})[
          color_button(options::color_label, item_width, control_height, &options::toggle_color, fonts.body)],
      lv::group({.x = margin * px, .y = row_2 * px})[choice("Quality", "Best\nMedium\nDraft", fonts.label)],
      lv::group({.x = right * px, .y = row_2 * px})[choice(second_name, second_options, fonts.label)],
      lv::group({.x = margin * px, .y = row_3 * px})[choice("Paper Size", "Original\nA4", fonts.label)],
      lv::group({.x = right * px, .y = row_3 * px})[choice("Layout", "Portrait\nLandscape", fonts.label)],
      lv::group({.x = margin * px, .y = row_4 * px})[
          wide_button(start_name, page_width - 2 * margin, 60, &options::start, tok.button.primary,
                      tok.button.primary, fonts.label)]];
}

}  // namespace printer
