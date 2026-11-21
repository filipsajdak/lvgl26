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
  return lv::group({.size = {100 * pct, 100 * pct},
                    .style = {.bg_color = tok.window.background, .bg_opa = tok.solid}})[
      back_header(title, "Choose Settings", fonts.title, fonts.body),
      lv::group({.flex_flow = "column", .size = {100 * pct, lv::size_content},
                  .y = header_height * px, .style = {.pad_all = tok.space.edge, .pad_gap = tok.space.card}})[
          lv::group({.flex_flow = "row", .size = {100 * pct, lv::size_content}, .style = {.pad_gap = tok.space.card}})[
              spin_box("Copies", options::count, &options::fewer, &options::more, fonts.body),
              color_button(options::color_label, item_width, control_height, &options::toggle_color, fonts.body)],
          lv::group({.flex_flow = "row", .size = {100 * pct, lv::size_content}, .style = {.pad_gap = tok.space.card}})[
              choice("Quality", "Best\nMedium\nDraft", fonts.label),
              choice(second_name, second_options, fonts.label)],
          lv::group({.flex_flow = "row", .size = {100 * pct, lv::size_content}, .style = {.pad_gap = tok.space.card}})[
              choice("Paper Size", "Original\nA4", fonts.label), choice("Layout", "Portrait\nLandscape", fonts.label)],
          wide_button(start_name, page_width - 2 * margin, 60, &options::start, tok.button.primary,
                      tok.button.primary, fonts.label)]];
}

}  // namespace printer
