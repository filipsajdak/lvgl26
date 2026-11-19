#pragma once

// An application worth clicking, written with lvgl26 only. It knows no platform: the window
// (main.cpp) and the test (tests/window_app.cpp) give it a display and a pointer, as a device
// or a browser page would.
//
// A dial that the two buttons move, a slider and a switch that write to states, a count of
// clicks, and a task that ticks on its own.
#include <lv/lv.hpp>

#include <cstdint>

namespace demo {

using namespace lv::units;

inline constexpr std::int32_t width = 480;
inline constexpr std::int32_t height = 360;
inline constexpr std::int32_t step_of_dial = 10;
inline constexpr std::int32_t most_of_dial = 100;

inline constinit lv::state dial{lv::counter{}};    // the arc, moved by the buttons
inline constinit lv::state clicks{lv::counter{}};  // every click on a button
inline constinit lv::state ticks{lv::counter{}};   // counted by a task
inline constinit lv::state volume{lv::counter{}};  // the slider
inline constinit lv::state lamp{false};            // the switch

inline void more() {
  dial.update([](lv::counter c) { return c.value + step_of_dial > most_of_dial ? c : c += step_of_dial; });
  ++clicks;
}
inline void fewer() {
  dial.update([](lv::counter c) { return c.value - step_of_dial < 0 ? c : c -= step_of_dial; });
  ++clicks;
}

//! [ui]
inline constexpr auto ui = lv::column(
    {.flex_align = {"center", "center", "center"}, .size = {width * px, height * px}})[
    lv::label({.text = "lvgl26 in a window"}),
    lv::arc({.range = {0, most_of_dial}, .size = {130 * px, 130 * px}, .value = lv::bind(dial)})[
        lv::label({.align = "center", .text = lv::bind(dial, "{}")})],
    lv::row({.flex_align = {"center", "center", "center"}, .size = {lv::size_content, lv::size_content}})[
        lv::button({.id = "fewer", .on_clicked = &fewer})[lv::label({.text = "-"})],
        lv::label({.text = lv::bind(clicks, "{} clicks")}),
        lv::button({.id = "more", .on_clicked = &more})[lv::label({.text = "+"})]],
    lv::row({.flex_align = {"center", "center", "center"}, .size = {lv::size_content, lv::size_content}})[
        lv::slider({.id = "volume", .range = {0, most_of_dial}, .size = {200 * px, 12 * px}, .value = lv::bind(volume)}),
        lv::toggle({.checked = lv::bind(lamp)}),
        lv::label({.text = lv::bind(ticks, "{} ticks")})]];
//! [ui]

// The task that makes the screen live without input.
inline lv::task<> tick() {
  for (;;) {
    co_await lv::sleep(250 * ms);
    ++ticks;
  }
}

}  // namespace demo
