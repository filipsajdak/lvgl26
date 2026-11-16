#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// The index of the chosen option. The dropdown and the label follow it, and the
// button and the dropdown write it. The C example binds the dropdown to an integer
// subject, and LVGL's bind function writes the choice back. A state follows a
// dropdown in one direction only, so the dropdown's own handler writes it back.
struct option {
  std::int32_t index;
};
constinit lv::state<option> chosen{{0}};

void pick_cherry() {
  chosen = option{2};
}

void remember_choice(lv::event e) {
  chosen = option{static_cast<std::int32_t>(e.target<lv::view<"dropdown">>().get<"selected">())};
}

constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = centered | lv::style({.pad_row = tok.gap.lg})})[
    lv::label({.text = lv::bind(chosen, "Selected option: {}")}),
    lv::dropdown({.on_value_changed = &remember_choice,
                  .options = "Apple\nBanana\nCherry\nDate\nElderberry",
                  .selected = lv::bind(chosen), .width = 160 * px}),
    lv::button({.on_clicked = &pick_cherry})[
        lv::label({.align = "center", .text = "Pick Cherry"})]];

}  // namespace

void widgets::dropdown_bind_value(lv::obj screen) {
  lv::place<ui>(screen);
}
