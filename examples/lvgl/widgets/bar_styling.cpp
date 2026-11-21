#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// A dark track with a thin border.
constexpr auto track = lv::style({.bg_color = tok.bar.track.fill,
                                  .bg_opa = tok.opacity.opaque,
                                  .radius = tok.bar.track.radius}) |
                       tok.bar.track.border;

// An indicator that goes from green to cyan, with a tight glow.
constexpr auto indicator = lv::style({.bg_color = tok.bar.indicator.from,
                                      .bg_grad_color = tok.bar.indicator.to,
                                      .bg_grad_dir = "hor",
                                      .bg_opa = tok.opacity.opaque,
                                      .radius = tok.bar.indicator.radius,
                                      .shadow_opa = opacity_step(180)}) |
                           tok.bar.indicator.shadow;

constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = centered | lv::style({.pad_row = tok.gap.xl})})[
    // A tall bar with a named style for each part.
    lv::bar({.max_value = 100, .min_value = 0, .size = {90 * pct, 22 * px}, .value = {65, false},
             .style = track, .style_indicator = indicator}),
    // A thin, flat bar with a style written in place.
    lv::bar({.max_value = 100, .min_value = 0, .size = {90 * pct, 10 * px},
             .value = {35, false},
             .style = lv::style({.bg_color = tok.bar.flat.fill,
                                 .bg_opa = tok.opacity.opaque,
                                 .radius = tok.bar.flat.radius}) |
                      tok.bar.flat.border})];

}  // namespace

void widgets::bar_styling(lv::obj screen) {
  lv::place<ui>(screen);
}
