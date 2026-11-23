#pragma once

// The screen of screen.hpp as it is written without looks: every element names each design
// value it takes. docs/looks.md sets the two side by side; tests/looks_example.cpp checks that
// they draw the same pixels.

#include <lv/font.hpp>

#include "screen.hpp"

namespace looks::before {

//! [before]
constexpr lv::typeface type({{"Barlow", 500, barlow}});

constexpr auto tile(const char* text, const auto& tint) {
  const auto& t = tok.tile;
  return lv::button(
    {
      .flex_align = {"center", "center", "center"},
      .flex_flow  = "column",
      .style = {
        .bg_color      = tint.bg_color,
        .bg_grad_color = tint.bg_grad_color,
        .bg_grad_dir   = "ver",
        .bg_opa        = t.bg_opa,
        .height        = t.height,
        .radius        = t.radius,
        .shadow_width  = t.shadow_width,
        .text_color    = t.text_color,
        .text_font     = type(t.text_font),
        .width         = t.width,
      },
      .style_pressed = {.bg_opa = t.pressed.bg_opa},
    },
    lv::label({.text = text})
  );
}

constexpr auto ui = lv::group(
  {
    .flex_flow = "column",
    .style = {
      .bg_color   = tok.page.bg_color,
      .bg_opa     = tok.page.bg_opa,
      .pad_all    = tok.page.pad_all,
      .pad_row    = tok.page.pad_row,
      .text_color = tok.page.text_color,
      .text_font  = type(tok.page.text_font),
    },
  },
  lv::label({
    .text  = "Printer",
    .style = {.text_font = type(tok.title.text_font)},
  }),
  lv::group(
    {
      .flex_flow = "row",
      .style = {
        .height     = tok.tiles.height,
        .pad_column = tok.tiles.pad_column,
        .width      = tok.tiles.width,
      },
    },
    tile(names::print, tok.tile_print), tile(names::scan, tok.tile_scan),
    tile(names::copy, tok.tile_copy)
  ),
  lv::label({
    .checked       = true,
    .text          = "Ready to print",
    .style         = {.text_color = tok.status.text_color},
    .style_checked = {.text_color = tok.status.checked.text_color},
  })
);
//! [before]

}  // namespace looks::before
