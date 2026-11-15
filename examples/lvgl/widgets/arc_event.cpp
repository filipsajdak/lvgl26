#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// What the arc reports as its value.
struct arc_value {
  std::int32_t value;
};

// The label follows this; the arc's event handler writes it.
// The C example writes the label's text in its handler instead.
constinit lv::state<arc_value> reading{};

void show_value(lv::event e) {
  reading = arc_value{e.target<lv::view<"arc">>().get<"value">()};
}

constexpr auto ui = lv::group({.flex_flow = LV_FLEX_FLOW_COLUMN,
                               .style = {.flex_cross_place = LV_FLEX_ALIGN_CENTER,
                                         .pad_row = tok.gap.md}})[
    lv::arc({.id = "dial", .on_value_changed = &show_value, .size = {150 * px, 150 * px}}),
    lv::label({.text = lv::bind(reading, "Value: {}")})];

}  // namespace

void widgets::arc_event(lv::obj screen) {
  auto built = lv::mount<ui>(screen);
  if (!built) return;
  // An arc has no value until one is set, and LVGL reports that as -32768. The C
  // example shows this number at the start, so the label starts from the arc too.
  reading = arc_value{built->get<"dial">().get<"value">()};
  built->detach();
}
