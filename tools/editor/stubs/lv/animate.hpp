#pragma once

// Editor stub of lv/animate.hpp: animations a coroutine can wait for.
//
//   co_await lv::animate(arc, {.value = {0, 100}}, 300 * lv::ms);
//   co_await lv::animate(arc, {.width = {80 * lv::px, 120 * lv::px}}, 200 * lv::ms, lv::ease::in_quad);
//   co_await lv::animate(label, {.opa = {0 * lv::pct, 100 * lv::pct}}, 220 * lv::ms);
//   co_await lv::animate(lv::in<"knob", "pressed">(arc), {.opa = {20 * lv::pct, 100 * lv::pct}}, 120 * lv::ms);

#include <lv/easing.hpp>
#include <lv/generated.hpp>
#include <lv/props.hpp>
#include <lv/task.hpp>
#include <lv/units.hpp>
#include <lv/values.hpp>
#include <lv/view.hpp>

namespace lv {

/// The properties of a widget that can be animated, each with its two ends: those
/// that take one number, length, angle or opacity, and then the style properties of the main part that
/// take one. The width "" is the style properties alone, as lv::in takes them.
template <detail::fixed W>
  requires(W.sv().empty() || detail::known_widget<W>)
using animatable = typename stub::animatable_for<W>::type;

/// The style properties alone, as lv::in takes them.
using animatable_style = typename stub::animatable_for<"">::type;

/// Runs one property of a widget from one value to another:
///   bool finished = co_await lv::animate(arc, {.value = {0, 100}}, 300 * lv::ms);
/// The result is false when something stopped the animation. A fourth argument says
/// how the value moves: lv::ease::in_quad, a cubicBezier token, or an lv::bezier.
template <detail::fixed W>
detail::source<bool> animate(view<W> widget, const typename stub::animatable_for<W>::type& what, duration time,
                             const easing& how = {});

/// A widget and the part and the state whose local style values lv::animate moves: lv::in<"knob">(view).
template <detail::fixed W, unsigned Selector>
struct view_in {
  view<W> widget;
};

/// Names the part and the state of a style animation: one name is a part or a state, two are the part
/// and then the state. lv::animate(lv::in<"knob", "pressed">(arc), {.opa = {...}}, 120 * lv::ms).
template <detail::fixed First, detail::fixed... Rest, detail::fixed W>
constexpr view_in<W, 0> in(view<W> widget) {
  return {widget};
}

template <detail::fixed W, unsigned Selector>
detail::source<bool> animate(view_in<W, Selector> target, const typename stub::animatable_for<"">::type& what,
                             duration time, const easing& how = {});

}  // namespace lv
