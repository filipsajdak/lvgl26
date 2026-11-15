#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// A position on the arc's scale, which runs from -20 to 220.
struct scale_value {
  std::int32_t value;
};

// One difference from the C example: that one limits its subject to 0..100, so
// dragging the arc past 100 puts it back to 100. A state has no limits, so here
// the arc can be dragged over its whole scale.
constinit lv::state reading{scale_value{50}};

constexpr auto ui = lv::group({.flex_flow = LV_FLEX_FLOW_ROW, .style = spread_out})[
    lv::arc({.bg_end_angle = 45 * deg, .bg_start_angle = 135 * deg, .max_value = 220,
             .min_value = -20, .size = {120 * px, 120 * px}, .value = lv::bind(reading)})[
        lv::label({.align = LV_ALIGN_CENTER, .text = lv::bind(reading, "{}"), .y = -10 * px}),
        lv::label({.align = LV_ALIGN_CENTER, .text = "[-20..220]", .y = 10 * px})]];

}  // namespace

void widgets::arc_value_range(lv::obj screen) {
  lv::place<ui>(screen);
}
