// Ports lv_example_buttonmatrix_one_checked. Re-ordered: lv_buttonmatrix_set_map runs first, and
// lv_buttonmatrix_set_one_checked before lv_obj_set_size. The controls, set by
// lv_buttonmatrix_set_button_ctrl and lv_buttonmatrix_set_button_ctrl_all in place of
// lv_buttonmatrix_set_ctrl_map, run last, as in the C example.

#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// The weekdays. Every key can be checked, one at a time, and Tuesday is.
constexpr const char* const days[] = {"Mon", "Tue", "Wed", "Thu", "Fri", nullptr};

constexpr auto ui = lv::widget<"buttonmatrix">({
  .align           = "center",
  .button_ctrl     = {1, "checked"},
  .button_ctrl_all = "checkable",
  .map             = days,
  .one_checked     = true,
  .size            = {90 * pct, 80 * px},
});

}  // namespace

void widgets::buttonmatrix_one_checked(lv::obj screen) { lv::place<ui>(screen); }
