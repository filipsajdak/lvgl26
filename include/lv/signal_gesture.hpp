#pragma once

// Signal's gesture limits: how far and how fast a finger goes before it is a swipe, and a scroll, and the time
// of LVGL's long press. They are LVGL's own defaults, in a token file so that a design can change them.
//
//   #include <lv/signal_gesture.hpp>
//
//   lv::pointer mouse{display, lv::signal::gesture};
//
// `gesture` holds `swipe_distance` (50 px), `swipe_speed` (3 px in each read), `scroll_limit` (10 px) and
// `long_press` (400 ms). It is in a token file of its own, include/lv/signal/gesture.json, and a header of
// its own, as the motion is: a program that takes no swipe does not pay for it.

#include <lv/tokens.hpp>

namespace lv::signal {

namespace detail {

constexpr char gesture_json[] = {
#embed "signal/gesture.json"
    , 0};

// The settings of Signal's own token files: they are in the standard's forms whatever the project sets.
struct gesture_settings {};

}  // namespace detail

// `gesture.swipe_distance`, `.swipe_speed`, `.scroll_limit`, `.long_press`: what lv::pointer takes.
inline constexpr auto gesture = lv::tokens<detail::gesture_json, detail::gesture_settings>;

}  // namespace lv::signal
