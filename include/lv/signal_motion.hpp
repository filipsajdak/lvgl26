#pragma once

// Signal's motion: how long its changes take and how they move, the same in every theme.
//
//   #include <lv/signal_motion.hpp>
//
//   co_await lv::animate(view, {.value = {0, 100}}, lv::signal::motion.duration.settle,
//                        *lv::signal::motion.ease.out);
//
// `motion.duration` holds `press` (120 ms), `enter` (220 ms) and `settle` (480 ms).
// `motion.ease` holds the curves `out`, `in_out` and `back`; `back` overshoots a little and comes
// back, so its y leaves [0, 1]. design/signal/README.md says which change takes which.
//
// The tokens are in a token file of their own, include/lv/signal/motion.json, and this is a
// header of its own, because every token costs compile time in the header that reads it:
// lv/signal.hpp and lv/signal_type.hpp do not include it, and a program that animates nothing
// does not pay for it.

#include <lv/tokens.hpp>

namespace lv::signal {

namespace detail {

constexpr char motion_json[] = {
#embed "signal/motion.json"
    , 0};

// The settings of Signal's own token files: they are in the standard's forms whatever the
// project sets.
struct motion_settings {};

}  // namespace detail

// `motion.duration.press`, `.enter`, `.settle`; `motion.ease.out`, `.in_out`, `.back`.
inline constexpr auto motion = lv::tokens<detail::motion_json, detail::motion_settings>;

}  // namespace lv::signal
