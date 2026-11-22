// Ports lv_example_roller_event. Re-ordered: the state that takes the place of the event
// handler (lv_obj_add_event_cb) is bound before lv_roller_set_visible_row_count.

#include "widgets.hpp"

#include <utility>

namespace {

using namespace widgets;
using namespace lv::units;

// The day on the roller. The roller lists the enumerators, and writes the user's choice here.
enum class day { Mon, Tue, Wed, Thu, Fri, Sat, Sun };
constinit lv::state chosen{day::Mon};

// What the label says for each day. The C example writes it in the roller's event handler.
constexpr const char* status_texts[] = {"Day: Mon", "Day: Tue", "Day: Wed", "Day: Thu",
                                        "Day: Fri", "Day: Sat", "Day: Sun"};

constinit lv::computed status{
  [](day d) {
    return status_texts[std::to_underlying(d)];
  },
  chosen,
};

constexpr auto ui = lv::group(
  {
    .flex_flow = "column",
    .style     = lv::style({.flex_cross_place = "center", .pad_row = tok.gap.md}),
  },
  lv::roller({
    .options           = {lv::options_of<day>, "normal"},
    .selected          = lv::bind(chosen),
    .visible_row_count = 3,
  }),
  lv::label({.text = lv::bind(status)})
);

}  // namespace

void widgets::roller_event(lv::obj screen) {
  lv::place<ui>(screen);
}
