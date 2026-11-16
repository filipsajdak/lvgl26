// The quick start of the documentation: a speedometer, drawn into memory and
// saved as a picture. docs/quick-start.md shows this file piece by piece, and
// tools/docs.py checks that the page and the file agree.
//
//   quick_start            writes quick_start.png
//   quick_start out.png    writes out.png

#include <lv/lv.hpp>
#include <lv/png.hpp>

#include <mp-units/systems/si.h>

#include <cstdio>

using namespace lv::units;
using namespace mp_units::si::unit_symbols;

namespace {

//! [state]
constinit lv::state speed{0 * km / h};

void reset() { speed = 0 * km / h; }
//! [state]

//! [tokens]
constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};

constexpr auto tok = lv::tokens<tokens_json>;
//! [tokens]

//! [tree]
constexpr auto ui = lv::column({.flex_align = {"center", "center", "center"},
                                .size = {320 * px, 240 * px},
                                .style = {.bg_color = tok.color.ground,
                                          .bg_opa = tok.opacity.opaque,
                                          .border_width = tok.space.none,
                                          .pad_all = tok.space.edge,
                                          .pad_row = tok.space.gap,
                                          .radius = tok.space.none,
                                          .text_color = tok.color.muted}})[
    lv::label({.id = "title", .text = "SPEED"}),
    lv::arc({.id = "speedo",
             .range = {0, 240},
             .size = {108 * px, 108 * px},
             .style = {.arc_color = tok.color.line, .arc_rounded = false, .arc_width = tok.stroke.track},
             .style_indicator = {.arc_color = tok.color.active, .arc_rounded = false, .arc_width = tok.stroke.track},
             .style_knob = {.bg_opa = tok.opacity.none, .pad_all = tok.space.none},
             .value = lv::bind(speed)})[
        lv::label({.align = "center", .id = "reading", .style = {.text_color = tok.color.reading}, .text = lv::bind(speed)})],
    lv::button({.id = "reset",
                .on_clicked = &reset,
                .style = {.bg_color = tok.color.active,
                          .pad_hor = tok.space.button_x,
                          .pad_ver = tok.space.button_y,
                          .radius = tok.space.none,
                          .shadow_width = tok.space.none,
                          .text_color = tok.color.on_active}})[
        lv::label({.text = "RESET"})]];
//! [tree]

//! [display]
constinit lv::frame<320, 240> frame;  // the display's memory
//! [display]

}  // namespace

//! [main]
int main(int argc, char** argv) {
  lv::application app;              // LVGL starts; it stops when app goes out of scope
  lv::display display{app, frame};  // no panel: the picture stays in the frame

  auto root = lv::mount<ui>(display.screen());  // std::expected; owns the widgets
  if (!root) return 1;

  speed = 120 * km / h;  // the arc and the label follow

  const char* path = argc > 1 ? argv[1] : "quick_start.png";
  if (!lv::write_png(display.screen(), path)) return 2;
  std::printf("wrote %s\n", path);
  return 0;
}
//! [main]
