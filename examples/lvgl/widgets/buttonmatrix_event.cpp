#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

constexpr const char* const keypad[] = {"1", "2", "3", "\n", "4", "5", "6", "\n", "7", "8", "9", nullptr};

// What the label says for each key, by the key's number. The label follows `status`,
// and the matrix's event handler writes it. The C example writes the label's text in
// its handler instead.
constexpr const char* said[] = {"Pressed: 1", "Pressed: 2", "Pressed: 3", "Pressed: 4", "Pressed: 5",
                                "Pressed: 6", "Pressed: 7", "Pressed: 8", "Pressed: 9"};
constexpr const char* none_yet = "Pressed: \xE2\x80\x94";  // a dash
constinit lv::state<const char*> status{none_yet};

void show_key(lv::event e) {
  status = said[e.target<lv::view<"buttonmatrix">>().get<"selected_button">()];
}

constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = {.flex_cross_place = "center", .pad_row = tok.gap.md}})[
    lv::widget<"buttonmatrix">({.map = keypad, .size = {200 * px, 180 * px},
                                .on_value_changed = &show_key}),
    lv::label({.text = lv::bind(status)})];

}  // namespace

void widgets::buttonmatrix_event(lv::obj screen) {
  status = none_yet;  // as the C example: every time it is built no key is pressed yet
  lv::place<ui>(screen);
}
