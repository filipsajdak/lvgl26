// Fonts from TrueType files, made by the compiler. docs/fonts.md walks through this file.
//
//   fonts    writes fonts.png

#include <lv/font.hpp>
#include <lv/lv.hpp>
#include <lv/png.hpp>

using namespace lv::units;

namespace {

//! [files]
constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr unsigned char condensed_ttf[] = {
#embed "fonts/BarlowCondensed-SemiBold.ttf"
};
//! [files]

//! [typeface]
constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

constexpr auto type = lv::typeface({
    {"Barlow", 500, barlow_ttf},
    {"Barlow Condensed", 600, condensed_ttf, "0123456789:"},  // only what a clock shows
});

constexpr auto label = type(tok.text.label);
constexpr auto title = type(tok.text.title);
constexpr auto numeral = type(tok.text.numeral);
//! [typeface]

//! [tree]
constexpr auto ui = lv::column({.flex_align = {"center", "center", "center"}, .size = {320 * px, 240 * px}})[
    lv::label({.text = "Wake up", .style = {.text_font = title}}),
    lv::label({.text = "06:45", .style = {.text_font = numeral}}),
    lv::label({.text = "Tomorrow, in 7 hours", .style = {.text_font = label}})];
//! [tree]

constinit lv::frame<320, 240> frame;

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  auto root = lv::mount<ui>(display.screen());
  if (!root) return 1;
  return lv::write_png(display.screen(), "fonts.png") ? 0 : 2;
}
