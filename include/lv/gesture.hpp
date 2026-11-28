#pragma once

// Swipes and holds: a finger that moves across a page, and a finger that stays on a widget.
//
//   const lv::swipe d = co_await lv::swiped(page);                          // any of the four directions
//   co_await lv::swiped<lv::swipe::left, lv::swipe::right>(page);           // only these two
//   lv::swiped(page).then([](lv::swipe d) { ... })                          // an arm of lv::first
//   lv::box({.on_swiped = &on_swipe})                                       // a handler: void on_swipe(lv::swipe)
//
//   lv::state<lv::percent> progress{lv::percent{0}};
//   const bool held = co_await lv::hold(ring, 1 * lv::sec, progress);       // true: the finger stayed down
//
// LVGL sends a swipe while the finger is still down, when it has moved the distance of the pointer's
// gesture limits (lv::pointer takes them), in the direction of the larger movement. It goes to the widget
// that was pressed if that keeps gestures, otherwise to its first ancestor that does: every widget passes
// them on, and a swipe that reaches no such ancestor is lost. A wait for a swipe, and `.on_swiped`, make
// their widget one that keeps them, so a wait on a page sees a swipe that starts anywhere on it, and the
// nearest page that waits gets it. `.gesture_bubble = false` on a widget with no wait turns the swipes off
// for its subtree: nothing above it sees them. A drag that LVGL takes as a scroll is no swipe.
// A swipe that a wait or `.on_swiped` takes ends its press: the button it started on is not clicked.

#include <lv/detail/missing.hpp>
#include <lv/task.hpp>
#include <lv/state.hpp>
#include <lv/units.hpp>
#include <lv/view.hpp>

#include <lvgl/lvgl.h>

#include <cstdint>
#include <type_traits>

namespace lv::detail {
// This header is read: see lv/detail/missing.hpp.
template <>
inline constexpr bool included<"lv/gesture.hpp"> = true;

// Whether the swipe `direction` is one of the wanted ones; none listed means every direction.
template <swipe... Wanted>
constexpr bool swipe_wanted(swipe direction) {
  if constexpr (sizeof...(Wanted) == 0)
    return true;
  else
    return ((direction == Wanted) || ...);
}

// The next swipe of a widget, of the directions `Wanted` (every direction when none is named).
template <swipe... Wanted>
struct swipe_source {
  lv_obj_t* widget = nullptr;
  void (*done)(void*) = nullptr;
  void* context = nullptr;
  lv_event_dsc_t* on_code = nullptr;
  swipe seen = swipe::left;

  swipe_source() = default;
  explicit swipe_source(lv_obj_t* w) : widget(w) {}
  swipe_source(swipe_source&&) = default;  // only moved before it is armed
  ~swipe_source() { disarm(); }

  bool arm(void (*done_fn)(void*), void* done_context, task_promise_base* owner)
    pre(widget != nullptr)
    pre(owner != nullptr)
  {
    done = done_fn;
    context = done_context;
    if (watch(owner, widget) == nullptr) {
      cancel(owner);
      return true;  // stay suspended, with nothing armed, until the task is ended
    }
    lv_obj_set_gesture_bubble(widget, false);  // LVGL sends a swipe to a widget that keeps it
    on_code = lv_obj_add_event_cb(widget, &fired_cb, LV_EVENT_GESTURE, this);
    return true;
  }

  void disarm() noexcept {
    if (on_code != nullptr) lv_obj_remove_event_dsc(widget, on_code);
    on_code = nullptr;
  }

  // What the wait gives: the direction of the swipe.
  swipe value() const noexcept { return seen; }

  // For lv::first: what to do when a swipe comes first. The handler may take the direction.
  template <class F>
    requires std::invocable<F&, swipe> || std::invocable<F&>
  handled<swipe_source, F> then(F handler) && {
    return {std::move(*this), std::move(handler)};
  }

  static void fired_cb(lv_event_t* e) {
    auto* self = static_cast<swipe_source*>(lv_event_get_user_data(e));
    const swipe direction = swipe_of(lv_indev_get_gesture_dir(lv_indev_active()));
    if (!swipe_wanted<Wanted...>(direction)) return;  // another direction: go on waiting
    self->seen = direction;
    take_swipe();
    self->disarm();
    self->done(self->context);  // may destroy this source: nothing after this line
  }
};

// A finger that stays on a widget for a time, with its progress in a state. Done when the time is up
// (the value is true) or when the finger is lifted, slides off, or the drag becomes a scroll (false).
struct hold_source {
  lv_obj_t* widget = nullptr;
  std::uint32_t ms = 0;
  state<percent>* progress = nullptr;
  void (*done)(void*) = nullptr;
  void* context = nullptr;
  lv_event_dsc_t* on_event = nullptr;
  bool running = false;
  bool held = false;

  hold_source() = default;
  hold_source(lv_obj_t* w, std::uint32_t time, state<percent>* p) : widget(w), ms(time), progress(p) {}
  hold_source(hold_source&&) = default;  // only moved before it is armed
  ~hold_source() { disarm(); }

  bool arm(void (*done_fn)(void*), void* done_context, task_promise_base* owner)
    pre(widget != nullptr)
    pre(owner != nullptr)
  {
    done = done_fn;
    context = done_context;
    if (watch(owner, widget) == nullptr) {
      cancel(owner);
      return true;
    }
    on_event = lv_obj_add_event_cb(widget, &event_cb, LV_EVENT_ALL, this);
    if (lv_obj_has_state(widget, LV_STATE_PRESSED)) begin();  // the finger is down already: the time runs from now
    return true;
  }

  void disarm() noexcept {
    stop_animation();
    if (on_event != nullptr) lv_obj_remove_event_dsc(widget, on_event);
    on_event = nullptr;
  }

  bool value() const noexcept { return held; }

  template <class F>
    requires std::invocable<F&, bool> || std::invocable<F&>
  handled<hold_source, F> then(F handler) && {
    return {std::move(*this), std::move(handler)};
  }

 private:
  void show(std::int32_t percent_done) {
    if (progress != nullptr) progress->set(percent{percent_done});
  }

  void stop_animation() noexcept {
    if (running) lv_anim_delete(this, &step_cb);
    running = false;
  }

  void begin() {
    if (running) return;
    show(0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, this);
    lv_anim_set_user_data(&a, this);
    lv_anim_set_exec_cb(&a, &step_cb);
    lv_anim_set_values(&a, 0, full_percent);
    lv_anim_set_duration(&a, ms);
    lv_anim_set_completed_cb(&a, &completed_cb);
    running = true;
    lv_anim_start(&a);
  }

  // The finger left before the time was up: the progress goes back to nothing.
  void give_up() {
    stop_animation();
    show(0);
    finish(false);
  }

  void finish(bool result) {
    held = result;
    disarm();
    done(context);  // may destroy this source: nothing after this line
  }

  static constexpr std::int32_t full_percent = 100;

  static void step_cb(void* var, std::int32_t value) {
    auto* self = static_cast<hold_source*>(var);
    // A drag that LVGL took as a scroll takes the pressed state off the widget: that is no hold.
    if (!lv_obj_has_state(self->widget, LV_STATE_PRESSED)) {
      self->give_up();
      return;
    }
    self->show(value);
  }

  static void completed_cb(lv_anim_t* a) {
    auto* self = static_cast<hold_source*>(lv_anim_get_user_data(a));
    self->running = false;
    if (lv_obj_has_state(self->widget, LV_STATE_PRESSED)) {
      self->show(full_percent);
      self->finish(true);
    } else {
      self->give_up();
    }
  }

  static void event_cb(lv_event_t* e) {
    auto* self = static_cast<hold_source*>(lv_event_get_user_data(e));
    switch (lv_event_get_code(e)) {
      case LV_EVENT_PRESSED:
        self->begin();
        break;
      case LV_EVENT_RELEASED:
      case LV_EVENT_PRESS_LOST:
        if (self->running) self->give_up();
        break;
      default:
        break;
    }
  }
};

}  // namespace lv::detail

namespace lv {

// The next swipe of a widget: co_await lv::swiped(page). Name directions to wait for only those:
// lv::swiped<lv::swipe::left, lv::swipe::right>(page).
template <swipe... Wanted, view_value V>
detail::swipe_source<Wanted...> swiped(const V& widget) {
  return detail::swipe_source<Wanted...>{widget.raw()};
}

// A finger that stays on a widget for `time`: co_await lv::hold(ring, 1 * lv::sec, progress) gives true when
// it did, false when the finger was lifted, slid off, or the drag became a scroll. `progress` goes from 0 to
// 100 while the finger stays, and back to 0 when it leaves. The state must outlive the wait.
template <view_value V>
detail::hold_source hold(const V& widget, duration time, state<percent>& progress) {
  return detail::hold_source{widget.raw(), static_cast<std::uint32_t>(time.ms), &progress};
}

// The progress of a hold is a state of percents.
template <view_value V, class T>
  requires(!std::same_as<T, percent>)
void hold(const V&, duration, state<T>&) = delete(
    "lv: the progress of lv::hold is an lv::state<lv::percent>: lv::state<lv::percent> progress{lv::percent{0}}. "
    "It goes from 0 to 100 while the finger stays.");

// The same, with no progress to show.
template <view_value V>
detail::hold_source hold(const V& widget, duration time) {
  return detail::hold_source{widget.raw(), static_cast<std::uint32_t>(time.ms), nullptr};
}

}  // namespace lv
