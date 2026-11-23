// detail::deferred_queue: work that waits for the next refresh, in the order it was queued, once each; and defer(),
// which wakes the refresh of the display that shows a widget.
#include <lv/lv.hpp>

#include "check.hpp"

#include <type_traits>

namespace {

using lv::detail::deferred_node;
using lv::detail::deferred_queue;

static_assert(!std::is_copy_constructible_v<deferred_node> && !std::is_copy_assignable_v<deferred_node>);
static_assert(!std::is_copy_constructible_v<deferred_queue> && !std::is_copy_assignable_v<deferred_queue>);

// A piece of work that records its place in the order it ran.
struct work : deferred_node {
  int id = 0;
  explicit work(int n, void (*what)(deferred_node&) = nullptr) : id(n) { run = what; }
};

int order[8];
int ran = 0;
deferred_queue* requeue_into = nullptr;
work* requeue_this = nullptr;

void record(deferred_node& n) { order[ran++] = static_cast<work&>(n).id; }
void record_and_queue_again(deferred_node& n) {
  record(n);
  if (requeue_this != nullptr) {
    requeue_into->add(*requeue_this);
    requeue_this = nullptr;
  }
}

void in_the_order_of_queueing() {
  ran = 0;
  deferred_queue q;
  work a{1, &record}, b{2, &record}, c{3, &record};
  q.add(a);
  q.add(b);
  q.add(c);
  q.drain();
  check(ran == 3 && order[0] == 1 && order[1] == 2 && order[2] == 3);
  check(!a.queued && !b.queued && !c.queued);
  check(a.next == nullptr && a.prev == nullptr && b.next == nullptr && b.prev == nullptr && c.next == nullptr && c.prev == nullptr);
  q.drain();  // empty: nothing runs
  check(ran == 3);
}

void once_each() {
  ran = 0;
  deferred_queue q;
  work a{1, &record}, b{2, &record};
  q.add(a);
  q.add(b);
  q.add(a);  // already queued: keeps its place
  q.drain();
  check(ran == 2 && order[0] == 1 && order[1] == 2);
  q.add(b);  // can be queued again once it ran
  q.drain();
  check(ran == 3 && order[2] == 2);
}

void remove_from_each_place() {
  ran = 0;
  deferred_queue q;
  work a{1, &record}, b{2, &record}, c{3, &record}, d{4, &record};
  q.remove(a);  // not queued: nothing happens
  check(!a.queued);
  q.add(a);
  q.add(b);
  q.add(c);
  q.add(d);
  q.remove(b);  // from the middle
  check(!b.queued && b.next == nullptr && b.prev == nullptr);
  q.remove(a);  // from the front
  q.remove(d);  // from the back
  check(!a.queued && !d.queued && c.queued);
  q.drain();
  check(ran == 1 && order[0] == 3);
  q.remove(c);  // ran already
  q.add(a);
  q.add(b);
  q.remove(a);
  q.remove(b);  // the last one
  q.drain();
  check(ran == 1);
  q.add(d);  // the queue is usable after it emptied
  q.drain();
  check(ran == 2 && order[1] == 4);
}

// Taking out a node that is not queued leaves the queue as it is.
void remove_of_an_unqueued_node() {
  ran = 0;
  deferred_queue q;
  work a{1, &record}, b{2, &record}, stray{3, &record};
  q.add(a);
  q.add(b);
  q.remove(stray);
  q.drain();
  check(ran == 2 && order[0] == 1 && order[1] == 2);
}

// settle() runs what waits in the queue of the program at once.
void settle_runs_the_queue_of_the_program() {
  ran = 0;
  work a{1, &record}, b{2, &record};
  lv::detail::deferred().add(a);
  lv::detail::deferred().add(b);
  lv::settle();
  check(ran == 2 && order[0] == 1 && order[1] == 2);
  check(!a.queued && !b.queued);
}

void work_queued_while_it_runs_runs_too() {
  ran = 0;
  deferred_queue q;
  work a{1, &record_and_queue_again}, b{2, &record};
  q.add(a);
  requeue_into = &q;
  requeue_this = &b;
  q.drain();
  check(ran == 2 && order[0] == 1 && order[1] == 2);
  check(!a.queued && !b.queued);
}

// A node out of the queue while it waits is never run.
void removed_before_the_drain() {
  ran = 0;
  deferred_queue q;
  work a{1, &record}, b{2, &record};
  q.add(a);
  q.add(b);
  q.remove(a);
  q.drain();
  check(ran == 1 && order[0] == 2);
}

bool woke = false;
void note_wake(deferred_node&) { woke = true; }

// defer() queues the node and starts the refresh timer of the display that shows the widget.
void defer_wakes_the_refresh(lv_obj_t* screen) {
  lv_display_t* d = lv_obj_get_display(screen);
  lv_timer_t* refresh = lv_display_get_refr_timer(d);
  lv_display_add_event_cb(d, &lv::detail::drain_deferred, LV_EVENT_REFR_START, nullptr);
  work w{1, &note_wake};
  lv_timer_pause(refresh);
  lv::detail::defer(w, screen);
  check(w.queued);
  lv_timer_ready(refresh);
  lv_timer_handler();
  check(woke);
  check(!w.queued);
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);
  in_the_order_of_queueing();
  once_each();
  remove_from_each_place();
  remove_of_an_unqueued_node();
  settle_runs_the_queue_of_the_program();
  work_queued_while_it_runs_runs_too();
  removed_before_the_drain();
  defer_wakes_the_refresh(lv_screen_active());
  return failures;
}
