// Ports lv_example_buttonmatrix_popover. Re-ordered: lv_buttonmatrix_set_map runs first. The
// controls, set by lv_buttonmatrix_set_button_ctrl_all in place of lv_buttonmatrix_set_ctrl_map,
// run last, as in the C example.

#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// One row of keys. A held key shows itself in a popover above the row.
constexpr const char* const keys[] = {"Q", "W", "E", "R", "T", "Y", nullptr};

constexpr auto ui = lv::widget<"buttonmatrix">({
  .align           = "center",
  .button_ctrl_all = "popover",
  .map             = keys,
  .size            = {90 * pct, 80 * px},
});

}  // namespace

void widgets::buttonmatrix_popover(lv::obj screen) { lv::place<ui>(screen); }
