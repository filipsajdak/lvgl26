#include "widgets.hpp"

namespace {

using namespace widgets;

// A pill: a filled label with round ends.
constexpr auto badge = lv::style({.bg_color = tok.label.badge.fill,
                                  .bg_opa = tok.opacity.opaque,
                                  .pad_hor = tok.label.badge.pad_hor,
                                  .pad_ver = tok.label.badge.pad_ver,
                                  .radius = tok.label.badge.radius,
                                  .text_color = tok.label.badge.text});

// A call-out: a border, a halo around it (the outline) and a shadow below.
constexpr auto callout = lv::style({.bg_color = tok.label.callout.fill,
                                    .bg_opa = tok.opacity.opaque,
                                    .outline_color = tok.label.callout.halo.color,
                                    .outline_opa = opacity_step(80),
                                    .outline_pad = tok.label.callout.halo.pad,
                                    .outline_width = tok.label.callout.halo.width,
                                    .pad_all = tok.label.callout.pad,
                                    .radius = tok.label.callout.radius,
                                    .shadow_opa = opacity_step(100),
                                    .text_color = tok.label.callout.text}) |
                         tok.label.callout.border | tok.label.callout.shadow;

constexpr auto ui = lv::group({.flex_flow = LV_FLEX_FLOW_COLUMN,
                               .style = centered | lv::style({.pad_row = tok.gap.lg})})[
    lv::label({.style = badge, .text = "NEW"}),
    lv::label({.style = callout, .text = "Heads up"})];

}  // namespace

void widgets::label_styling(lv::obj screen) {
  lv::place<ui>(screen);
}
