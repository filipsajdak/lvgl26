// A font whose size changes while the program runs. docs/fonts.md shows this file.
// It needs LVGL's TinyTTF: set(LVGL26_TTF ON) in CMake.

#include <lv/lv.hpp>
#include <lv/scaled_font.hpp>

using namespace lv::units;

namespace {

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr char tokens_json[] = {
#embed "tokens.json"
  , 0
};
constexpr auto tok = lv::tokens<tokens_json>;

//! [font]
constinit lv::scaled_font reading{barlow_ttf, tok.text.label};  // the size of the token: 16 px

constexpr auto ui = lv::column(
  {.size = {320 * px, 240 * px}}, lv::label({
                                    .id    = "reading",
                                    .text  = "21.5",
                                    .style = {.text_font = lv::bind(reading)},
                                  })
);
//! [font]

constinit lv::frame<320, 240> frame;

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  auto root = lv::mount<ui>(display.screen());
  if (!root) return 1;
  app.advance(50 * ms), app.step();  // one frame: the layout is worked out
  const auto small = root->get<"reading">().get<"height">();

  //! [resize]
  reading.resize(tok.text.title);    // 28 px: the label follows
  reading.resize(lv::raw(31 * px));  // a size that no token names
  //! [resize]

  app.advance(50 * ms), app.step();  // one frame: the layout is worked out
  const bool grew = root->get<"reading">().get<"height">() > small;
  return grew ? 0 : 2;
}
