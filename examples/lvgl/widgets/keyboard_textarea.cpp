#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// A field and a keyboard under it. The C example does not join them either: the
// keyboard writes into the field that has the focus, once an input device gives it.
constexpr auto ui = lv::group(
  lv::textarea({
    .align            = "top_mid",
    .one_line         = true,
    .placeholder_text = "Type here...",
    .width            = 90 * pct,
    .y                = 10 * px,
  }),
  lv::widget<"keyboard">({
    .align = "bottom_mid",
    .mode  = "text_lower",
    .size  = {100 * pct, 60 * pct},
  })
);

}  // namespace

void widgets::keyboard_textarea(lv::obj screen) { lv::place<ui>(screen); }
