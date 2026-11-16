#pragma once

// Editor stub of lv/events.hpp: one event of one widget, as something to wait for.
//
//   co_await button.next().clicked;
//
// The struct that view.next() returns is written by the stub generator: lv::stub::events.

#include <lvgl/lvgl.h>

#include <concepts>
#include <type_traits>

namespace lv {

namespace detail {
template <class Value, class F>
struct handled;
}  // namespace detail

/// One event of one widget: co_await it, or give it to lv::first with a handler.
template <lv_event_code_t Code>
struct event_ref {
  lv_obj_t* widget;

  /// For lv::first: what to do when this event is the first to happen.
  ///   button.next().clicked.then([] { ... })
  template <std::invocable F>
  detail::handled<void, F> then(F handler) const;
};

}  // namespace lv
