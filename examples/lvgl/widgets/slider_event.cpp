#include "widgets.hpp"

#include <format>

namespace {

using namespace widgets;
using namespace lv::units;

constexpr auto ui = lv::group({.flex_flow = "column",
                               .style = {.flex_cross_place = "center",
                                         .pad_row = tok.gap.md}})[
    lv::slider({.id = "slider", .width = 200 * px}),
    lv::label({.id = "status", .text = "Drag the slider"})];

// Says in the label what the slider is doing: being dragged, or let go.
// The C example has one event handler that asks which event it got. Here the two
// events are the two things the task waits for. It ends when the slider is deleted.
lv::task<> report(lv::view<"slider"> slider, lv::view<"label"> status) {
  for (;;) {
    const bool released = co_await lv::first(slider.next().value_changed.then([] { return false; }),
                                             slider.next().released.then([] { return true; }));
    const std::int32_t value = slider.get<"value">();
    const std::string text =
        released ? std::format("Released at: {}", value) : std::format("Dragging: {}", value);
    status.set({.text = text.c_str()});  // the label keeps its own copy
  }
}

}  // namespace

void widgets::slider_event(lv::obj screen) {
  auto built = lv::mount<ui>(screen);
  if (!built) return;
  lv::spawn(report(built->get<"slider">(), built->get<"status">()));
  built->detach();
}
