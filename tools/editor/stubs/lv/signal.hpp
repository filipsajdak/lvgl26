#pragma once

// Editor stub of lv/signal.hpp: Signal, the design system of lvgl26, as a ready theme.

#include <lv/signal_tokens.hpp>
#include <lv/theme.hpp>
#include <lv/tokens.hpp>

namespace lv::signal {

/// The two looks of Signal as a state a display can follow: display.follow(mode, {day, dark}).
enum class mode { day, dark };

/// The design tokens of Signal's dark theme, for your own styles:
/// lv::signal::dark_tokens.color.muted.
inline constexpr lv::stub::signal_dark_tokens dark_tokens{};
/// The design tokens of Signal's day theme.
inline constexpr lv::stub::signal_day_tokens day_tokens{};

/// Signal for indoor devices and OLED screens: lv::display display{app, frame, lv::signal::dark}.
inline constexpr lv::theme_styles dark = lv::stub::theme_access::some();
/// Signal for screens read in sunlight.
inline constexpr lv::theme_styles day = lv::stub::theme_access::some();

}  // namespace lv::signal
