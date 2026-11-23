#pragma once

// An application worth clicking, written with lvgl26 only. The same source runs on the host
// and in the browser: platform.cpp gives it a display, a clock and a pointer.
//
// A dial that moves with the two buttons, a slider and a switch that write to states, a
// counter of clicks, and a task that ticks on its own.
#include <lv/lv.hpp>

#include <cstdint>

namespace demo {

using namespace lv::units;

inline constexpr std::int32_t width        = 480;
inline constexpr std::int32_t height       = 360;
inline constexpr std::int32_t step_of_dial = 10;

inline constinit lv::state dial{lv::counter{}};    // the arc, moved by the buttons
inline constinit lv::state clicks{lv::counter{}};  // every click on a button
inline constinit lv::state ticks{lv::counter{}};   // counted by a task
inline constinit lv::state volume{lv::counter{}};  // the slider
inline constinit lv::state lamp{false};            // the switch

inline void more() {
  dial.update([](lv::counter c) {
    return c.value + step_of_dial > 100 ? c : c += step_of_dial;
  });
  ++clicks;
}
inline void fewer() {
  dial.update([](lv::counter c) {
    return c.value - step_of_dial < 0 ? c : c -= step_of_dial;
  });
  ++clicks;
}

inline constexpr auto ui = lv::column(
  {.flex_align = {"center", "center", "center"}, .size = {width * px, height * px}},
  lv::label({.text = "lvgl26 in the browser"}),
  lv::arc(
    {
      .range = {0, 100},
      .size  = {130 * px, 130 * px},
      .value = lv::bind(dial),
    },
    lv::label({.align = "center", .text = lv::bind(dial, "{}")})
  ),
  lv::row(
    {.flex_align = {"center", "center", "center"}, .size = {lv::size_content, lv::size_content}},
    lv::button({.on_clicked = &fewer}, lv::label({.text = "-"})),
    lv::label({.text = lv::bind(clicks, "{} clicks")}),
    lv::button({.on_clicked = &more}, lv::label({.text = "+"}))
  ),
  lv::row(
    {.flex_align = {"center", "center", "center"}, .size = {lv::size_content, lv::size_content}},
    lv::slider({
      .range = {0, 100},
      .size  = {200 * px, 12 * px},
      .value = lv::bind(volume),
    }),
    lv::toggle({.checked = lv::bind(lamp)}), lv::label({.text = lv::bind(ticks, "{} ticks")})
  )
);

// The task that makes the screen live without input.
inline lv::task<> tick() {
  for (;;) {
    co_await lv::sleep(250 * ms);
    ++ticks;
  }
}

}  // namespace demo

// What platform.cpp and main need of an application: its size, its tree, a way to start its task on the
// mounted tree, and the values of its states for a test. The other application, bounce/, gives the same.
// The platform mounts the tree and checks the result.
namespace app {

inline constexpr std::int32_t width  = demo::width;
inline constexpr std::int32_t height = demo::height;
inline constexpr const auto& ui      = demo::ui;

// The task belongs to the mounted tree: it ends when the tree is unmounted, so mounting it again
// starts exactly one.
template <class Mounted>
void start(const Mounted& root) {
  lv::spawn(demo::tick(), root->root());
}

// 0 the dial, 1 the clicks, 2 the ticks, 3 the slider, 4 the switch (1 when on). Else -1.
inline std::int32_t state(std::int32_t which) {
  switch (which) {
    case 0:
      return demo::dial.get().value;
    case 1:
      return demo::clicks.get().value;
    case 2:
      return demo::ticks.get().value;
    case 3:
      return demo::volume.get().value;
    case 4:
      return demo::lamp.get() ? 1 : 0;
    default:
      return -1;
  }
}

}  // namespace app
