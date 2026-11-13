#include "get_started.hpp"

#include <mp-units/systems/si.h>

namespace {

using namespace lv::units;

// One value shared by the slider and the label: drag the slider and the label follows.
constinit lv::state level{50 * mp_units::percent};

constexpr auto ui = lv::group[
    lv::label({.align = LV_ALIGN_CENTER, .text = lv::bind(level, "{}"), .y = -20 * px}),
    lv::slider({.align = LV_ALIGN_CENTER, .value = lv::bind(level), .width = 90 * pct,
                .y = 20 * px})];

}  // namespace

void get_started::slider(lv::obj screen) {
  lv::place<ui>(screen);
}
