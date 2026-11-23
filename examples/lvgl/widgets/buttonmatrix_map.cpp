#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// A 3 by 3 keypad: the "\n" entries end a row, and the last entry is null.
// The widget keeps a pointer to the map, not a copy.
constexpr const char* const keypad[] = {
  "1", "2", "3", "\n", "4", "5", "6", "\n", "7", "8", "9", nullptr
};

constexpr auto ui = lv::widget<"buttonmatrix">({
  .align = "center",
  .map   = keypad,
  .size  = {90 * pct, 200 * px},
});

}  // namespace

void widgets::buttonmatrix_map(lv::obj screen) { lv::place<ui>(screen); }
