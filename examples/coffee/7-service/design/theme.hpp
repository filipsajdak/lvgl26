#pragma once

// The theme of every widget: Signal, in its form for a light room. Only the files that make a
// display include it, so no screen pays for Signal's token files.

#include <lv/signal.hpp>

namespace coffee {

inline constexpr lv::theme_styles theme = lv::signal::day;

}  // namespace coffee
