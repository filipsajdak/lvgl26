#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// The main part: a wide, faint track on a white card with a shadow.
constexpr auto track = lv::style({.arc_color = tok.arc.gauge.track.color,
                                  .arc_opa = opacity_step(180),
                                  .arc_rounded = true,
                                  .arc_width = tok.arc.gauge.track.width,
                                  .bg_color = tok.arc.gauge.card.fill,
                                  .bg_opa = tok.opacity.opaque,
                                  .pad_all = tok.arc.gauge.card.pad,
                                  .radius = tok.arc.gauge.card.radius}) |
                       tok.arc.gauge.card.shadow;

// The indicator: narrower than the track, and set in from its edge.
constexpr auto indicator = lv::style({.arc_color = tok.arc.gauge.indicator.color,
                                      .arc_rounded = true,
                                      .arc_width = tok.arc.gauge.indicator.width,
                                      .pad_all = tok.arc.gauge.indicator.pad});

// The knob: filled, with a white border and a glow.
constexpr auto knob = lv::style({.bg_color = tok.arc.gauge.knob.fill,
                                 .bg_opa = tok.opacity.opaque,
                                 .pad_all = tok.arc.gauge.knob.pad,
                                 .shadow_opa = opacity_step(160)}) |
                      tok.arc.gauge.knob.border | tok.arc.gauge.knob.shadow;

constexpr auto ui = lv::group({.flex_flow = LV_FLEX_FLOW_COLUMN,
                               .style = centered | lv::style({.pad_row = tok.gap.xl})})[
    lv::arc({.max_value = 100, .min_value = 0, .size = {180 * px, 180 * px}, .style = track,
             .style_indicator = indicator, .style_knob = knob, .value = 65})];

}  // namespace

void widgets::arc_styling(lv::obj screen) {
  lv::place<ui>(screen);
}
