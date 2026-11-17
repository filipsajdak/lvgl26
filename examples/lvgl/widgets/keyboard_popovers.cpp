#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// A keyboard whose keys show a magnified copy above the finger while they are held.
constexpr auto ui = lv::group[
    lv::textarea({.align = "top_mid", .one_line = true, .placeholder_text = "Press and hold a key",
                  .width = 90 * pct, .y = 10 * px}),
    lv::widget<"keyboard">({.align = "bottom_mid", .mode = "text_lower", .popovers = true,
                  .size = {100 * pct, 60 * pct}})];

}  // namespace

void widgets::keyboard_popovers(lv::obj screen) {
  lv::place<ui>(screen);
}
