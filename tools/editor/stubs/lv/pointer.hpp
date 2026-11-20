#pragma once

// Editor stub of lv/pointer.hpp: a mouse or a touch panel, tied to a display.
//
//   lv::pointer mouse{display};          // a queue of 16 samples
//   mouse.press({60, 35}); mouse.release();
//   lv::pointer panel{display, &read};   // LVGL asks lv::pointer_sample read()

#include <lv/detail/missing.hpp>
#include <lv/display.hpp>

#include <cstddef>
#include <type_traits>

namespace lv::detail {
template <>
inline constexpr bool included<"lv/pointer.hpp"> = true;
}  // namespace lv::detail

namespace lv {

/// Whether a pointer touches the screen.
enum class contact : bool { released, pressed };
inline constexpr contact released = contact::released;
inline constexpr contact pressed = contact::pressed;

/// What a pointer reports: where it is, and whether it touches.
struct pointer_sample {
  point at;
  contact state = released;
  friend constexpr bool operator==(pointer_sample, pointer_sample) = default;
};

/// What a platform object keeps to feed a pointer.
class pointer_feed {
 public:
  constexpr pointer_feed() = default;
  explicit operator bool() const noexcept { __builtin_unreachable(); }
  bool operator()(point, contact) const { __builtin_unreachable(); }
  friend bool operator==(const pointer_feed&, const pointer_feed&) noexcept { __builtin_unreachable(); }
};

/// A platform object that feeds a pointer: attach(feed) and detach(feed).
template <class F>
concept pointer_feeder = requires(F& f, pointer_feed feed) {
  f.attach(feed);
  f.detach(feed);
};

/// A pointer fed by the platform through a queue of `Capacity` samples.
template <std::size_t Capacity>
  requires detail::pointer_capacity<Capacity> && detail::included<"lv/pointer.hpp">
class pointer<Capacity> {
 public:
  explicit pointer(display&) {}
  /// A pointer that `feeder`, a platform object such as a window, feeds.
  template <pointer_feeder F>
  pointer(display&, F&) {}
  /// What a platform object keeps to feed this pointer.
  pointer_feed handle() noexcept { __builtin_unreachable(); }
  pointer(const pointer&) = delete;
  pointer& operator=(const pointer&) = delete;
  /// What happened, in order. False when the queue is full and could not merge it.
  bool feed(point, contact) { __builtin_unreachable(); }
  bool press(point) { __builtin_unreachable(); }
  bool move(point) { __builtin_unreachable(); }
  bool release() { __builtin_unreachable(); }
  lv_indev_t* raw() const { __builtin_unreachable(); }
};

/// A pointer that LVGL asks at each read.
template <>
class pointer<0> {
 public:
  pointer(display&, pointer_sample (*)()) {}
  template <class F>
    requires(std::is_convertible_v<F, pointer_sample (*)()> && !std::is_pointer_v<std::remove_cvref_t<F>>)
  pointer(display&, F&&) {}
  pointer(const pointer&) = delete;
  pointer& operator=(const pointer&) = delete;
  lv_indev_t* raw() const { __builtin_unreachable(); }
};

explicit pointer(display&) -> pointer<>;
template <pointer_feeder F>
pointer(display&, F&) -> pointer<>;
template <class F>
  requires(!pointer_feeder<std::remove_cvref_t<F>>)
pointer(display&, F&&) -> pointer<0>;

}  // namespace lv
