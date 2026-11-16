#pragma once

// Editor stub of lv/easing.hpp: how an animation moves between its ends.

#include <lvgl/lvgl.h>

#include <lv/token_types.hpp>
#include <lv/values.hpp>

namespace lv {

/// How an animation moves between its ends: nothing (one speed), an lv::bezier, a
/// "cubicBezier" design token, or a name from lv::ease.
class easing {
 public:
  /// One speed.
  constexpr easing() = default;
  /// Along a curve, as in CSS.
  constexpr easing(const bezier&) {}
  /// Along the curve of a design token: tok.motion.fall.
  constexpr easing(const token<bezier>&) {}
  /// One of LVGL's own paths, such as lv_anim_path_bounce.
  static constexpr easing of(lv_anim_path_cb_t) { return {}; }

  lv_anim_path_cb_t path() const;
  const bezier& curve() const;
  bool is_curve() const;
  /// Gives the animation this easing.
  void apply(lv_anim_t* animation) const;
};

/// The easings that have a name: LVGL's animation paths and its named curves.
namespace ease {

/// One speed.
inline constexpr easing linear{};
/// LVGL's animation paths.
inline constexpr easing in{}, out{}, in_out{}, overshoot{}, bounce{};
/// The end value, at the end.
inline constexpr easing step{};
/// LVGL's named curves, the ones of easings.net.
inline constexpr easing in_sine{}, out_sine{}, in_out_sine{};
inline constexpr easing in_quad{}, out_quad{}, in_out_quad{};
inline constexpr easing in_cubic{}, out_cubic{}, in_out_cubic{};
inline constexpr easing in_quart{}, out_quart{}, in_out_quart{};
inline constexpr easing in_quint{}, out_quint{}, in_out_quint{};
inline constexpr easing in_expo{}, out_expo{}, in_out_expo{};
inline constexpr easing in_circ{}, out_circ{}, in_out_circ{};
inline constexpr easing in_back{}, out_back{}, in_out_back{};

}  // namespace ease

}  // namespace lv
