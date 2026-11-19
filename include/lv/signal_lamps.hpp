#pragma once

// Signal's tell-tale lamp colours, for a program that shows tell-tales.
//
//   #include <lv/signal_lamps.hpp>
//
//   lv::box({.style = {.bg_color = lv::signal::lamps.dark.green}})
//
// The law defines the colour of some lamps: direction indicators and lights on are green,
// the main beam is blue (design/signal, "Colour"). `lamps.dark` and `lamps.day` hold
// `green` and `blue` for each theme. They are never a value, a state or a control.
//
// The colours are in a token file of their own, include/lv/signal/lamps.json, and this is a
// header of its own, because every token costs compile time in the header that reads it:
// lv/signal.hpp and lv/signal_type.hpp do not include it, and a program that shows no
// tell-tale does not pay for it.

#include <lv/tokens.hpp>

namespace lv::signal {

namespace detail {

constexpr char lamps_json[] = {
#embed "signal/lamps.json"
    , 0};

// The settings of Signal's own token files: they are in the standard's forms whatever the project sets.
struct lamp_settings {};

}  // namespace detail

// `lamps.dark.green`, `lamps.dark.blue`, `lamps.day.green` and `lamps.day.blue`.
inline constexpr auto lamps = lv::tokens<detail::lamps_json, detail::lamp_settings>;

}  // namespace lv::signal
