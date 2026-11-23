#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// The fruit that is chosen. The dropdown lists the enumerators, and the dropdown and the
// label follow the state, which the dropdown and the button write. The C example binds
// the dropdown to an integer subject.
//! [enum]
enum class fruit { Apple, Banana, Cherry, Date, Elderberry };
constinit lv::state<fruit> chosen{fruit::Apple};
//! [enum]

void pick_cherry() { chosen = fruit::Cherry; }

constexpr auto ui = lv::group(
  {.flex_flow = "column", .style = centered | lv::style({.pad_row = tok.gap.lg})},
  lv::label({.text = lv::bind(chosen, "Selected option: {}")}),
  //! [bound]
  lv::dropdown({
    .options  = lv::options_of<fruit>,
    .selected = lv::bind(chosen),
    .width    = 160 * px,
  }),
  //! [bound]
  lv::button({.on_clicked = &pick_cherry}, lv::label({.align = "center", .text = "Pick Cherry"}))
);

}  // namespace

void widgets::dropdown_bind_value(lv::obj screen) { lv::place<ui>(screen); }
