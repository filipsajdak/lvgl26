// One screen, drawn with two themes: Signal, which comes with lvgl26, and a theme
// made here from a token file. docs/themes.md walks through this file.
//
//   theme    writes theme_signal.png and theme_paper.png

#include <lv/lv.hpp>
#include <lv/png.hpp>
#include <lv/signal.hpp>
#include <lv/theme.hpp>

using namespace lv::units;

namespace {

//! [tree]
constexpr auto ui = lv::column(
  {.flex_align = {"center", "center", "center"}, .size = {320 * px, 240 * px}},
  lv::label({.text = "Battery"}),
  lv::arc(
    {
      .range = {0, 100},
      .size  = {110 * px, 110 * px},
      .value = 72,
    },
    lv::label({.align = "center", .text = "72 %"})
  ),
  lv::button(lv::label({.text = "Charge"}))
);
//! [tree]

//! [paper]
constexpr char tokens_json[] = {
#embed "tokens.json"
  , 0
};
constexpr auto tok = lv::tokens<tokens_json>;

constexpr auto paper = lv::theme({
  .arc = {
    .style           = {.arc_color = tok.color.rule, .arc_width = tok.stroke.ring},
    .style_indicator = {.arc_color = tok.color.accent, .arc_width = tok.stroke.ring},
    .style_knob      = {.bg_opa = tok.opacity.none},
  },
  .button = {
    .style = {
      .bg_color     = tok.color.accent,
      .pad_hor      = tok.space.button_x,
      .pad_ver      = tok.space.button_y,
      .radius       = tok.radius.button,
      .shadow_width = tok.space.none,
      .text_color   = tok.color.on_accent,
    },
    .style_pressed = {.bg_color = tok.color.ink},
  },
  .obj = {
    .style = {
      .bg_opa       = tok.opacity.none,
      .border_width = tok.space.none,
      .pad_all      = tok.space.edge,
      .pad_gap      = tok.space.gap,
      .text_color   = tok.color.ink,
    },
  },
  .screen = {
    .style = {.bg_color = tok.color.paper, .bg_opa = tok.opacity.opaque},
  },
});
//! [paper]

constinit lv::frame<320, 240> frame;

//! [draw]
bool draw(lv::application& app, lv::theme_styles theme, const char* file) {
  lv::display display{app, frame, theme};
  auto root = lv::mount<ui>(display.screen());
  return root && lv::write_png(display.screen(), file);
}
//! [draw]

}  // namespace

//! [main]
int main() {
  lv::application app;
  if (!draw(app, lv::signal::dark, "theme_signal.png")) return 1;
  if (!draw(app, paper, "theme_paper.png")) return 2;
  return 0;
}
//! [main]
