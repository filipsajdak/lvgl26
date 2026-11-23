// Ports lv_example_buttonmatrix_recolor. Re-ordered: lv_buttonmatrix_set_map runs first. The
// controls, set by lv_buttonmatrix_set_button_ctrl_all in place of lv_buttonmatrix_set_ctrl_map,
// run before lv_obj_set_size.

#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// Three keys, each with one word in a colour of its own. "#ff0000 Stop#" is LVGL's recolor text:
// the colour of the word between the two marks.
constexpr const char* const keys[] = {"#ff0000 Stop#", "#e08800 Warn#", "#00a000 Go#", nullptr};

constexpr auto ui = lv::widget<"buttonmatrix">({
  .align           = "center",
  .button_ctrl_all = "recolor",
  .map             = keys,
  .size            = {90 * pct, 80 * px},
});

}  // namespace

void widgets::buttonmatrix_recolor(lv::obj screen) {
  lv::place<ui>(screen);
}
