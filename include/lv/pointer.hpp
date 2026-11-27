#pragma once

// A pointer: a mouse or a touch panel, as an object of the program, tied to a display.
//
//   lv::display display{app, frame};
//   lv::pointer mouse{display};          // a queue of 16 samples, inside the object
//   mouse.press({60, 35});               // the platform pushes what happened
//   mouse.move({70, 35});
//   mouse.release();
//   app.step();                          // LVGL reads every sample: nothing is lost
//
//   lv::pointer panel{display, &read};   // or LVGL asks a function at each read:
//                                        //   lv::pointer_sample read() { return {at, lv::pressed}; }
//
//   lv::pointer mouse{display, window};  // or a platform object feeds it, such as a window:
//                                        // it gets an lv::pointer_feed and calls it
//
// LVGL reads an input device every 33 ms and sees the state at that moment, so a press and
// a release between two reads would be lost. The queue keeps them, and LVGL takes them all
// in one read. A sample makes LVGL read at its next step, without waiting for the period.
//
// A full queue merges a move into the last sample it holds; it keeps every press and release,
// and refuses a sample it cannot keep: feed() returns false. Feed it from the code that steps
// the application, not from an interrupt.
//
// The pointer is deleted when the object ends, so declare it after its display. LVGL keeps
// 488 bytes of its heap for it on a 64-bit host.

#include <lv/detail/missing.hpp>
#include <lv/display.hpp>
#include <lv/detail/input_queue.hpp>

#include <lvgl/lvgl.h>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace lv::detail {
// This header is read: see lv/detail/missing.hpp.
template <>
inline constexpr bool included<"lv/pointer.hpp"> = true;
}  // namespace lv::detail

namespace lv {

// Whether a pointer touches the screen: a button of a mouse held, or a finger on a panel.
enum class contact : bool { released, pressed };
inline constexpr contact released = contact::released;
inline constexpr contact pressed = contact::pressed;

// What a pointer reports: where it is, and whether it touches.
struct pointer_sample {
  point at;
  contact state = released;
  friend constexpr bool operator==(pointer_sample, pointer_sample) = default;
};

// The thresholds of the gestures a pointer makes. The defaults are LVGL's own. A design's token group `gesture`
// has these members, so a pointer takes `tok.gesture` itself: lv::pointer mouse{display, tok.gesture}.
struct gesture_limits {
  length swipe_distance = length::px(50);  // how far the finger goes before a swipe is sent
  length swipe_speed = length::px(3);      // how far it goes in each read of LVGL, 33 ms: slower is not a swipe
  length scroll_limit = length::px(10);    // how far it goes before a drag is a scroll
  duration long_press = duration{400};   // the time of LVGL's own long press, one for the device
};

// What a pointer takes as its gestures: a struct with these four members, as a `gesture_limits` or a group
// of design tokens.
template <class L>
concept gesture_settings = requires(const L& g) {
  { static_cast<length>(g.swipe_distance) };
  { static_cast<length>(g.swipe_speed) };
  { static_cast<length>(g.scroll_limit) };
  { static_cast<duration>(g.long_press) };
};

namespace detail {

// A pointer sample can be merged into the one before when both have the same contact: the
// newer position wins, and no press or release is lost.
struct merge_pointer_moves {
  static constexpr bool merge(pointer_sample& last, pointer_sample next) noexcept {
    if (last.state != next.state) return false;
    last.at = next.at;
    return true;
  }
};

// One LVGL pointer device, with a read function that `Self` provides.
class pointer_device {
 public:
  pointer_device(const pointer_device&) = delete;
  pointer_device& operator=(const pointer_device&) = delete;

  ~pointer_device() { lv_indev_delete(raw_); }

  // The LVGL input device, for code that still calls the C API.
  lv_indev_t* raw() const { return raw_; }

  // The thresholds of swipes, of scrolls and of LVGL's long press, in pixels and milliseconds.
  template <gesture_settings L>
  void set_gestures(const L& limits) {
    lv_indev_set_gesture_min_distance(raw_, whole_pixels(static_cast<length>(limits.swipe_distance)));
    lv_indev_set_gesture_min_velocity(raw_, whole_pixels(static_cast<length>(limits.swipe_speed)));
    lv_indev_set_scroll_limit(raw_, whole_pixels(static_cast<length>(limits.scroll_limit)));
    lv_indev_set_long_press_time(raw_, static_cast<std::uint16_t>(static_cast<duration>(limits.long_press).ms));
  }

 protected:
  pointer_device(display& screen, void* self, lv_indev_read_cb_t read) : raw_(lv_indev_create()) {
    contract_assert(raw_ != nullptr);  // LVGL's heap is too small for an input device
    lv_indev_set_type(raw_, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(raw_, screen.raw());
    lv_indev_set_driver_data(raw_, self);
    lv_indev_set_read_cb(raw_, read);
  }

  // LVGL reads at its next step, not when the period of the device is over.
  void read_soon() const { lv_timer_ready(lv_indev_get_read_timer(raw_)); }

  // LVGL holds a threshold in 8 bits: more than 255 pixels is 255.
  static std::uint8_t whole_pixels(length l) {
    const std::int32_t px_count = l.raw();
    return static_cast<std::uint8_t>(px_count < 0 ? 0 : (px_count > 255 ? 255 : px_count));
  }

  template <class Self>
  static Self& self_of(lv_indev_t* raw) {
    return *static_cast<Self*>(lv_indev_get_driver_data(raw));
  }

  static void report(lv_indev_data_t& data, pointer_sample s) {
    data.point.x = s.at.x;
    data.point.y = s.at.y;
    data.state = s.state == contact::pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
  }

 private:
  lv_indev_t* raw_;
};

}  // namespace detail

// What a platform object keeps to feed a pointer: feed(at, state) as the pointer's own.
// Copied freely; the pointer tells its feeder with detach() before it ends.
class pointer_feed {
  void* target_ = nullptr;
  bool (*feed_)(void* target, point at, contact state) = nullptr;

 public:
  constexpr pointer_feed() = default;
  constexpr pointer_feed(void* target, bool (*feed)(void*, point, contact)) noexcept : target_(target), feed_(feed) {}

  // Whether it leads to a pointer.
  explicit operator bool() const noexcept { return feed_ != nullptr; }

  // Feeds the pointer, as lv::pointer::feed does. It must lead to one.
  bool operator()(point at, contact state) const
    pre(feed_ != nullptr)
  {
    return feed_(target_, at, state);
  }

  friend bool operator==(const pointer_feed& a, const pointer_feed& b) noexcept { return a.target_ == b.target_; }
};

// A platform object that feeds a pointer: lv::pointer mouse{display, window} calls
// window.attach(feed), and window.detach(feed) when the pointer ends.
template <class F>
concept pointer_feeder = requires(F& f, pointer_feed feed) {
  f.attach(feed);
  f.detach(feed);
};

// The largest queue of a pointer: LVGL takes a queue in one read, sample after sample.

// A queue of `Capacity` samples, or none (Capacity 0) for a pointer that LVGL asks.
template <std::size_t Capacity>
  requires detail::pointer_capacity<Capacity> && detail::included<"lv/pointer.hpp">
class pointer<Capacity> : public detail::pointer_device {
  detail::input_queue<pointer_sample, Capacity, detail::merge_pointer_moves> queue_{};
  pointer_sample last_{};

  void* feeder_ = nullptr;
  void (*detach_)(void* feeder, pointer_feed) = nullptr;

  static void read(lv_indev_t* raw, lv_indev_data_t* data) {
    auto& self = self_of<pointer>(raw);
    if (auto next = self.queue_.pop()) self.last_ = *next;
    report(*data, self.last_);
    data->continue_reading = !self.queue_.empty();
  }

  static bool feed_of(void* self, point at, contact state) { return static_cast<pointer*>(self)->feed(at, state); }

  template <class F>
  static void detach_from(void* feeder, pointer_feed feed) {
    static_cast<F*>(feeder)->detach(feed);
  }

 public:
  explicit pointer(display& screen) : pointer_device(screen, this, &read) {}

  // A pointer with the thresholds of a design: lv::pointer mouse{display, tok.gesture}.
  template <gesture_settings L>
  pointer(display& screen, const L& limits) : pointer(screen) {
    set_gestures(limits);
  }
  pointer(display&&) = delete("the pointer keeps a pointer to its display: declare the display first, then the pointer");

  // A pointer that `feeder`, a platform object such as a window, feeds.
  template <pointer_feeder F>
  pointer(display& screen, F& feeder) : pointer(screen) {
    feeder_ = &feeder;
    detach_ = &detach_from<F>;
    feeder.attach(handle());
  }

  // A pointer that `feeder` feeds, with the thresholds of a design.
  template <pointer_feeder F, gesture_settings L>
  pointer(display& screen, F& feeder, const L& limits) : pointer(screen, feeder) {
    set_gestures(limits);
  }

  pointer(const pointer&) = delete;
  pointer& operator=(const pointer&) = delete;
  ~pointer() {
    if (detach_ != nullptr) detach_(feeder_, handle());
  }

  // What a platform object keeps to feed this pointer.
  pointer_feed handle() noexcept { return {this, &feed_of}; }

  // What happened, in order. False when the queue is full and could not merge the sample.
  bool feed(point at, contact state) {
    const bool kept = queue_.push({at, state});
    read_soon();
    return kept;
  }

  // The pointer touches at `at`, moves to `to`, lets go where it is.
  bool press(point at) { return feed(at, contact::pressed); }
  bool move(point to) { return feed(to, newest().state); }
  bool release() { return feed(newest().at, contact::released); }

 private:
  // The sample LVGL will see last: the newest one queued, or the one it saw.
  pointer_sample newest() const { return queue_.empty() ? last_ : queue_.back(); }
};

// A pointer that LVGL asks at each read, for a panel that is polled.
template <>
class pointer<0> : public detail::pointer_device {
  pointer_sample (*ask_)();

  static void read(lv_indev_t* raw, lv_indev_data_t* data) { report(*data, self_of<pointer>(raw).ask_()); }

 public:
  pointer(display& screen, pointer_sample (*ask)())
    pre(ask != nullptr)
      : pointer_device(screen, this, &read), ask_(ask) {}

  // A lambda without captures, as the function it converts to.
  template <class F>
    requires(std::is_convertible_v<F, pointer_sample (*)()> && !std::is_pointer_v<std::remove_cvref_t<F>>)
  pointer(display& screen, F&& ask) : pointer(screen, static_cast<pointer_sample (*)()>(ask)) {}

  // A lambda without captures converts to the function, and is taken.
  template <class F>
    requires(!std::is_convertible_v<F, pointer_sample (*)()>)
  pointer(display&, F&&) = delete(
      "a polled pointer takes a function lv::pointer_sample(): LVGL calls it at each read. "
      "A lambda that captures is not one; keep its state in a global or a static");
  pointer(display&&, pointer_sample (*)()) = delete(
      "the pointer keeps a pointer to its display: declare the display first, then the pointer");

  // A polled pointer has no queue: LVGL asks its function.
  bool feed(point, contact) = delete("this pointer is polled: LVGL asks its function. Use lv::pointer mouse{display} to feed one");
  bool press(point) = delete("this pointer is polled: LVGL asks its function. Use lv::pointer mouse{display} to feed one");
  bool move(point) = delete("this pointer is polled: LVGL asks its function. Use lv::pointer mouse{display} to feed one");
  bool release() = delete("this pointer is polled: LVGL asks its function. Use lv::pointer mouse{display} to feed one");
};

explicit pointer(display&) -> pointer<>;
template <gesture_settings L>
pointer(display&, const L&) -> pointer<>;
template <pointer_feeder F>
pointer(display&, F&) -> pointer<>;
template <pointer_feeder F, gesture_settings L>
pointer(display&, F&, const L&) -> pointer<>;
template <class F>
  requires(!pointer_feeder<std::remove_cvref_t<F>> && !gesture_settings<std::remove_cvref_t<F>>)
pointer(display&, F&&) -> pointer<0>;

}  // namespace lv
