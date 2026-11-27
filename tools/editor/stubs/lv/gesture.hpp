#pragma once

// Editor stub of lv/gesture.hpp.

#include <lv/detail/missing.hpp>
#include <lv/state.hpp>
#include <lv/units.hpp>
#include <lv/view.hpp>

namespace lv::detail {
template <>
inline constexpr bool included<"lv/gesture.hpp"> = true;

template <swipe... Wanted>
struct swipe_source {
  template <class F>
  swipe_source then(F) && {
    __builtin_unreachable();
  }
  swipe await_resume() { __builtin_unreachable(); }
  bool await_ready() { __builtin_unreachable(); }
  template <class H>
  void await_suspend(H) {}
};

struct hold_source {
  template <class F>
  hold_source then(F) && {
    __builtin_unreachable();
  }
  bool await_resume() { __builtin_unreachable(); }
  bool await_ready() { __builtin_unreachable(); }
  template <class H>
  void await_suspend(H) {}
};
}  // namespace lv::detail

namespace lv {

/// The next swipe of a widget: co_await lv::swiped(page) gives an lv::swipe. Name directions to wait for only
/// those: lv::swiped<lv::swipe::left, lv::swipe::right>(page).
template <swipe... Wanted, view_value V>
detail::swipe_source<Wanted...> swiped(const V&) {
  __builtin_unreachable();
}

/// A finger that stays on a widget for a time: co_await lv::hold(ring, 1 * lv::sec, progress) gives true when it
/// did, false when it was lifted or the drag became a scroll. `progress` goes from 0 to 100.
template <view_value V>
detail::hold_source hold(const V&, duration, state<percent>&) {
  __builtin_unreachable();
}

/// The same, with no progress to show.
template <view_value V>
detail::hold_source hold(const V&, duration) {
  __builtin_unreachable();
}

}  // namespace lv
