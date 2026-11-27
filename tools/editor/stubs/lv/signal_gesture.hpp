#pragma once

// Editor stub of lv/signal_gesture.hpp: Signal's gesture limits.

#include <lv/signal_tokens.hpp>
#include <lv/tokens.hpp>

namespace lv::signal {

/// The gesture limits of Signal: gesture.swipe_distance (50 px), .swipe_speed (3 px in each read),
/// .scroll_limit (10 px) and .long_press (400 ms). A pointer takes it: lv::pointer mouse{display, lv::signal::gesture}.
inline constexpr lv::stub::signal_gesture_tokens gesture{};

}  // namespace lv::signal
