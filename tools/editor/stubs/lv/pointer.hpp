#pragma once

// Editor stub of lv/pointer.hpp: a mouse or a touch panel, tied to a display.
//
//   lv::pointer mouse{display};          // a queue of 16 samples
//   mouse.press({60, 35}); mouse.release();
//   lv::pointer panel{display, &read};   // LVGL asks lv::pointer_sample read()

#include <lv/display.hpp>

#include <cstddef>
#include <type_traits>

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

/// The largest queue of a pointer.
inline constexpr std::size_t most_pointer_samples = 256;

/// A pointer fed by the platform through a queue of `Capacity` samples.
template <std::size_t Capacity = 16>
  requires(Capacity <= most_pointer_samples)
class pointer {
 public:
  explicit pointer(display&) {}
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
template <class F>
pointer(display&, F&&) -> pointer<0>;

}  // namespace lv
