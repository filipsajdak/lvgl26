#pragma once

// Editor stub of lv/signal_motion.hpp: Signal's durations and easings.

#include <lv/signal_tokens.hpp>
#include <lv/tokens.hpp>

namespace lv::signal {

/// The motion of Signal, the same in every theme: motion.duration.press, .enter and .settle
/// (120, 220 and 480 ms), and the curves motion.ease.out, .in_out and .back (which overshoots).
inline constexpr lv::stub::signal_motion_tokens motion{};

}  // namespace lv::signal
