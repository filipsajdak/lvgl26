#pragma once

// Editor stub of lv/animate.hpp: animations a coroutine can wait for.
//
//   co_await lv::animate(arc, {.value = {0, 100}}, 300 * lv::ms);
//   co_await lv::animate(arc, {.width = {80 * lv::px, 120 * lv::px}}, 200 * lv::ms, lv::ease::in_quad);

#include <lv/easing.hpp>
#include <lv/generated.hpp>
#include <lv/props.hpp>
#include <lv/task.hpp>
#include <lv/units.hpp>
#include <lv/values.hpp>
#include <lv/view.hpp>

namespace lv {

/// The properties of a widget that can be animated, each with its two ends: those
/// that take one number, length, angle or opacity.
template <detail::fixed W>
  requires detail::known_widget<W>
using animatable = typename stub::animatable_for<W>::type;

/// Runs one property of a widget from one value to another:
///   bool finished = co_await lv::animate(arc, {.value = {0, 100}}, 300 * lv::ms);
/// The result is false when something stopped the animation. A fourth argument says
/// how the value moves: lv::ease::in_quad, a cubicBezier token, or an lv::bezier.
template <detail::fixed W>
detail::source<bool> animate(view<W> widget, const typename stub::animatable_for<W>::type& what, duration time,
                             const easing& how = {});

}  // namespace lv
