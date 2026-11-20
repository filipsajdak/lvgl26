#pragma once

// Work that waits for the next refresh of a display: the switch of an lv::shown node. A node
// that has work queues itself; the queue is drained at the start of a display's refresh
// (lv::display adds the handler when it is made), or at once by lv::settle(). So several
// changes in one handler do the work once, with the last value, and before the next frame.
//
// The queue is intrusive: a node lives in what owns it, and the queue allocates nothing.
// Like all of LVGL, use it from the UI thread only.

#include <lvgl/lvgl.h>

namespace lv {

namespace detail {

// One piece of waiting work. `queued` is true while it is in the queue.
struct deferred_node {
  deferred_node* next = nullptr;
  deferred_node* prev = nullptr;
  bool queued = false;
  void (*run)(deferred_node& self) = nullptr;

  constexpr deferred_node() = default;
  deferred_node(const deferred_node&) = delete;
  deferred_node& operator=(const deferred_node&) = delete;
};

class deferred_queue {
  deferred_node* first_ = nullptr;
  deferred_node* last_ = nullptr;

 public:
  constexpr deferred_queue() = default;
  deferred_queue(const deferred_queue&) = delete;
  deferred_queue& operator=(const deferred_queue&) = delete;

  // Adds `n` after the others, unless it is already queued.
  void add(deferred_node& n) {
    if (n.queued) return;
    n.queued = true;
    n.next = nullptr;
    n.prev = last_;
    (last_ != nullptr ? last_->next : first_) = &n;
    last_ = &n;
  }

  // Takes `n` out, if it is queued.
  void remove(deferred_node& n) {
    if (!n.queued) return;
    (n.prev != nullptr ? n.prev->next : first_) = n.next;
    (n.next != nullptr ? n.next->prev : last_) = n.prev;
    n.next = n.prev = nullptr;
    n.queued = false;
  }

  // Runs the work in the order it was queued. Work queued while it runs runs too.
  void drain() {
    while (first_ != nullptr) {
      deferred_node& n = *first_;
      remove(n);
      n.run(n);
    }
  }
};

// The queue of the program.
inline deferred_queue& deferred() {
  static constinit deferred_queue queue;
  return queue;
}

// Queues `n`, and wakes the refresh of the display that shows `where`: a refresh starts even
// when nothing is marked to draw, also for a hidden widget or a screen that is not loaded.
inline void defer(deferred_node& n, lv_obj_t* where) {
  deferred().add(n);
  lv_display_t* d = lv_obj_get_display(where);
  lv_timer_t* refresh = d != nullptr ? lv_display_get_refr_timer(d) : nullptr;
  if (refresh != nullptr) lv_timer_resume(refresh);
}

// LV_EVENT_REFR_START of a display: the waiting work runs before anything is drawn.
inline void drain_deferred(lv_event_t*) { deferred().drain(); }

}  // namespace detail

// Does the waiting work now, such as the switch of an lv::shown node whose state changed:
// for a test, or for code that needs the new page in the same step.
inline void settle() { detail::deferred().drain(); }

}  // namespace lv
