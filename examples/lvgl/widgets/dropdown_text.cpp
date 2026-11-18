#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// Without a text the button shows the chosen option. With one, it always shows the text.
constexpr auto ui = lv::group[
    lv::dropdown({.align = "center", .options = {"Cut", "Copy", "Paste"}, .width = 110 * px,
                  .x = -100 * px}),
    lv::dropdown({.align = "center", .options = {"Cut", "Copy", "Paste"}, .text = "Menu",
                  .width = 110 * px, .x = 100 * px})];

}  // namespace

void widgets::dropdown_text(lv::obj screen) {
  lv::place<ui>(screen);
}
