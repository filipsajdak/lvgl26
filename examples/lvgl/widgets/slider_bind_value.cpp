#include "widgets.hpp"

#include <mp-units/systems/si.h>

namespace {

using namespace widgets;
using namespace lv::units;

// One value for the slider and the label: drag the slider and the label follows.
constinit lv::state level{50 * mp_units::percent};

constexpr auto ui = lv::group(
  {.flex_flow = "column", .style = centered | lv::style({.pad_row = tok.gap.lg})},
  lv::slider({.value = lv::bind(level), .width = 90 * pct}),
  lv::label({.text = lv::bind(level, "Value: {}/100")})
);

}  // namespace

void widgets::slider_bind_value(lv::obj screen) { lv::place<ui>(screen); }
