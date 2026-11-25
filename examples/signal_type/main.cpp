// Signal with its own typefaces: the theme sets the text, and the program makes the two
// larger fonts it shows. docs/themes.md walks through this file.
//
//   signal_type    writes signal_type.png

#include <lv/lv.hpp>
#include <lv/png.hpp>
#include <lv/signal_type.hpp>

using namespace lv::units;

namespace {

//! [readme]
//! [fonts]
// A copy: a reference to the tokens would keep the token file in the program.
constexpr auto text = lv::signal::type_tokens.text;

constexpr auto title = lv::signal::type(text.title);  // 24 px, the space to the tilde
constexpr auto reading =
  lv::signal::numerals(text.numeral_l);  // 72 px, the sixteen characters of a reading
//! [fonts]

//! [tree]
constexpr auto ui = lv::column(
  {
    .flex_align = {"center", "center", "center"},
    .size       = {320 * px, 240 * px},
    .style      = {.pad_gap = lv::signal::dark_tokens.space._2},
  },
  lv::label(
    {.text = "LIVING ROOM", .style = lv::style({.text_font = title}) | text.title}
  ),  // and its spacing
  lv::label({
    .text  = "21.5°",
    .style = {.text_font = reading},
  }),
  lv::label({.text = "Heating to 22 degrees."}), lv::button(lv::label({.text = "HOLD"}))
);
//! [tree]
//! [readme]

constinit lv::frame<320, 240> frame;

}  // namespace

//! [main]
int main() {
  lv::application app;
  lv::display display{app, frame, lv::signal::dark_with_type};
  auto root = lv::mount<ui>(display.screen());
  if (!root) return 1;
  return lv::write_png(display.screen(), "signal_type.png") ? 0 : 2;
}
//! [main]
