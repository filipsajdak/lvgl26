#pragma once

// How an animation moves between its ends: lv::easing.
//
//   co_await lv::animate(ball, {.y = {top, floor}}, 400 * lv::ms, lv::ease::in_quad);
//   co_await lv::animate(ball, {.y = {top, floor}}, 400 * lv::ms, tok.motion.fall);
//   co_await lv::animate(ball, {.y = {top, floor}}, 400 * lv::ms, lv::bezier{0.33, 0, 0.67, 0.33});
//
// An easing is one of three things:
//   - nothing, the default: the value moves at one speed;
//   - a curve: an lv::bezier, the two control points of a cubic Bezier curve as in CSS.
//     A "cubicBezier" design token is one, so easing can come from the token file;
//   - a name from lv::ease. These are LVGL's own: its animation paths (in, out,
//     in_out, overshoot, bounce, step) and its named curves (in_sine ... in_out_back,
//     the ones of easings.net).
//
// The x of both control points is from 0 to 1: time only moves forward. A y may
// leave that range, for a curve that overshoots; LVGL keeps each number in 16 bits
// with 1024 steps for one, so a y is from -32 to just under 32.

#include <lvgl/lvgl.h>

#include <lv/token_types.hpp>
#include <lv/values.hpp>

#include <cstdint>

namespace lv {

namespace detail {

// A control point of an easing curve as LVGL counts it: 0 to 1 is 0 to 1024.
constexpr std::int32_t bezier_part(double v) {
  return static_cast<std::int32_t>(v * 1024 + (v < 0 ? -0.5 : 0.5));
}

// Whether LVGL can hold this number of a control point: its 16 bits.
constexpr bool fits_bezier(double v) {
  return v >= -32 && v < 32 && bezier_part(v) <= INT16_MAX;
}

// Whether LVGL can follow this curve. See the top of this file.
constexpr bool usable(const bezier& curve) {
  return curve.x1 >= 0 && curve.x1 <= 1 && curve.x2 >= 0 && curve.x2 <= 1 && fits_bezier(curve.y1) &&
         fits_bezier(curve.y2);
}

}  // namespace detail

// How an animation moves between its ends. See the top of this file.
class easing {
  lv_anim_path_cb_t path_ = &lv_anim_path_linear;
  bezier curve_{};

  constexpr explicit easing(lv_anim_path_cb_t lvgl_path) : path_(lvgl_path) {}

 public:
  // One speed.
  constexpr easing() = default;

  // Along a curve. The curve of a straight line is one speed.
  constexpr easing(const bezier& curve)
      : path_(curve == bezier{} ? &lv_anim_path_linear : &lv_anim_path_custom_bezier3), curve_(curve) {}
  constexpr easing(const token<bezier>& curve) : easing(*curve) {}

  // One of LVGL's own paths, such as lv_anim_path_bounce: what lv::ease::bounce is.
  static constexpr easing of(lv_anim_path_cb_t lvgl_path)
    pre(lvgl_path != nullptr)
  {
    return easing{lvgl_path};
  }

  constexpr lv_anim_path_cb_t path() const { return path_; }
  constexpr const bezier& curve() const { return curve_; }
  constexpr bool is_curve() const { return path_ == &lv_anim_path_custom_bezier3; }

  // Gives the animation this easing.
  void apply(lv_anim_t* animation) const {
    lv_anim_set_path_cb(animation, path_);
    if (!is_curve()) return;
    lv_anim_set_bezier3_param(animation, static_cast<std::int16_t>(detail::bezier_part(curve_.x1)),
                              static_cast<std::int16_t>(detail::bezier_part(curve_.y1)),
                              static_cast<std::int16_t>(detail::bezier_part(curve_.x2)),
                              static_cast<std::int16_t>(detail::bezier_part(curve_.y2)));
  }
};

namespace detail {

// The curve that one of LVGL's LV_ANIM_SET_EASE_... macros gives an animation. The
// numbers stay LVGL's: nothing is copied here.
template <class Set>
constexpr easing named_curve(Set set) {
  lv_anim_t animation{};
  set(&animation);
  const lv_anim_bezier3_para_t p = animation.parameter.bezier3;
  return bezier{p.x1 / 1024.0, p.y1 / 1024.0, p.x2 / 1024.0, p.y2 / 1024.0};
}

}  // namespace detail

// The easings that have a name. See the top of this file.
namespace ease {

inline constexpr easing linear{};

// LVGL's animation paths.
inline constexpr easing in = easing::of(&lv_anim_path_ease_in);
inline constexpr easing out = easing::of(&lv_anim_path_ease_out);
inline constexpr easing in_out = easing::of(&lv_anim_path_ease_in_out);
inline constexpr easing overshoot = easing::of(&lv_anim_path_overshoot);
inline constexpr easing bounce = easing::of(&lv_anim_path_bounce);
inline constexpr easing step = easing::of(&lv_anim_path_step);  // the end value, at the end

// LVGL's named curves.
inline constexpr easing in_sine = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_IN_SINE(a); });
inline constexpr easing out_sine = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_OUT_SINE(a); });
inline constexpr easing in_out_sine = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_IN_OUT_SINE(a); });
inline constexpr easing in_quad = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_IN_QUAD(a); });
inline constexpr easing out_quad = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_OUT_QUAD(a); });
inline constexpr easing in_out_quad = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_IN_OUT_QUAD(a); });
inline constexpr easing in_cubic = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_IN_CUBIC(a); });
inline constexpr easing out_cubic = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_OUT_CUBIC(a); });
inline constexpr easing in_out_cubic = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_IN_OUT_CUBIC(a); });
inline constexpr easing in_quart = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_IN_QUART(a); });
inline constexpr easing out_quart = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_OUT_QUART(a); });
inline constexpr easing in_out_quart = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_IN_OUT_QUART(a); });
inline constexpr easing in_quint = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_IN_QUINT(a); });
inline constexpr easing out_quint = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_OUT_QUINT(a); });
inline constexpr easing in_out_quint = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_IN_OUT_QUINT(a); });
inline constexpr easing in_expo = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_IN_EXPO(a); });
inline constexpr easing out_expo = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_OUT_EXPO(a); });
inline constexpr easing in_out_expo = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_IN_OUT_EXPO(a); });
inline constexpr easing in_circ = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_IN_CIRC(a); });
inline constexpr easing out_circ = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_OUT_CIRC(a); });
inline constexpr easing in_out_circ = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_IN_OUT_CIRC(a); });
inline constexpr easing in_back = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_IN_BACK(a); });
inline constexpr easing out_back = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_OUT_BACK(a); });
inline constexpr easing in_out_back = detail::named_curve([](lv_anim_t* a) { LV_ANIM_SET_EASE_IN_OUT_BACK(a); });

}  // namespace ease

}  // namespace lv
