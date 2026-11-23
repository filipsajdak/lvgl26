#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// The day that the roller shows. The button writes it, and so does the roller. The C
// example binds the roller to an integer subject and writes the place of Wednesday.
enum class day { Mon, Tue, Wed, Thu, Fri, Sat, Sun };
constinit lv::state<day> shown{day::Mon};

void jump_to_wednesday() { shown = day::Wed; }

constexpr auto ui = lv::group(
  {.flex_flow = "column", .style = centered | lv::style({.pad_row = tok.gap.lg})},
  lv::roller({
    .options           = {lv::options_of<day>, "normal"},
    .selected          = lv::bind(shown),
    .visible_row_count = 3,
    .width             = 160 * px,
  }),
  lv::button(
    {.on_clicked = &jump_to_wednesday}, lv::label({.align = "center", .text = "Jump to Wed"})
  )
);

}  // namespace

void widgets::roller_bind_value(lv::obj screen) { lv::place<ui>(screen); }
