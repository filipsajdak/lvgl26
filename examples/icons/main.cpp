// Icons from Phosphor: a name checked by the compiler, drawn in a font of only those icons.
// docs/icons.md walks through this file.
//
//   icons    writes icons.png

#include <lv/font.hpp>
#include <lv/icons.hpp>
#include <lv/icons_font.hpp>
#include <lv/lv.hpp>
#include <lv/png.hpp>

using namespace lv::units;

namespace {

//! [type]
constexpr char tokens_json[] = {
#embed "tokens.json"
  , 0
};
constexpr auto tok = lv::tokens<tokens_json>;

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};

constexpr auto type = lv::typeface({
  {"Barlow", 500, barlow_ttf},
  {"Phosphor",
   700,
   lv::icons::phosphor_bold_ttf,
   lv::glyphs<"arrow-left", "gear", "plus", "minus">},
});

constexpr auto label_font = type(tok.text.label);
constexpr auto icon_font  = type(tok.icon.md);
//! [type]

//! [tree]
constexpr auto ui = lv::column(
  {.flex_align = {"center", "center", "center"}, .size = {320 * px, 240 * px}},
  lv::row(
    {.flex_align = {"center", "center", "center"}, .size = {300 * px, 60 * px}},
    lv::icon<"arrow-left">({.style = {.text_color = tok.color.accent, .text_font = icon_font}}),
    lv::label({.text = " Back", .style = {.text_color = tok.color.ink, .text_font = label_font}})
  ),
  lv::row(
    {.flex_align = {"center", "center", "center"}, .size = {300 * px, 60 * px}},
    lv::icon<"minus">({.style = {.text_color = tok.color.ink, .text_font = icon_font}}),
    lv::label(
      {.text = " 2 cups ", .style = {.text_color = tok.color.ink, .text_font = label_font}}
    ),
    lv::icon<"plus">({.style = {.text_color = tok.color.ink, .text_font = icon_font}})
  ),
  lv::icon<"gear">({.style = {.text_color = tok.color.accent, .text_font = icon_font}})
);
//! [tree]

constinit lv::frame<320, 240> frame;

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  auto root = lv::mount<ui>(display.screen());
  if (!root) return 1;
  return lv::write_png(display.screen(), "icons.png") ? 0 : 2;
}
