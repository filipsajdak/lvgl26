// The screen of examples/fonts, with each font drawn in a file of its own. docs/fonts.md walks
// through these files.
//
//   fonts_split    writes fonts_split.png

#include <lv/lv.hpp>
#include <lv/png.hpp>

#include "design.hpp"

using namespace lv::units;

namespace {

//! [tree]
constexpr auto ui = lv::column(
  {.flex_align = {"center", "center", "center"}, .size = {320 * px, 240 * px}},
  lv::label({
    .text  = "Wake up",
    .style = {.text_font = lv::font_of<"text.title">},
  }),
  lv::label({
    .text  = "06:45",
    .style = {.text_font = lv::font_of<"text.numeral">},
  }),
  lv::label({
    .text  = "Tomorrow, in 7 hours",
    .style = {.text_font = lv::font_of<"text.label">},
  })
);
//! [tree]

constinit lv::frame<320, 240> frame;

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  auto root = lv::mount<ui>(display.screen());
  if (!root) return 1;
  return lv::write_png(display.screen(), "fonts_split.png") ? 0 : 2;
}
