#include <lv/lv.hpp>

#include "check.hpp"

#include <cstring>

namespace {

using namespace lv::units;

std::size_t free_bytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.free_size;
}

void click(lv::obj o) { lv_obj_send_event(o.raw(), LV_EVENT_CLICKED, nullptr); }

constinit lv::state clicks{lv::counter{}};
struct mode {
  int value;
};
constinit lv::state current_mode{mode{0}};

// Sets a flag when it is destroyed: shows that a cancelled task cleans up.
struct at_exit {
  bool* flag;
  ~at_exit() { *flag = true; }
};

// Waits for a click, waits a moment, then counts it. Deleting the button ends it.
lv::task<> count_clicks(lv::view<"button"> button, bool* ended) {
  at_exit cleanup{ended};
  for (;;) {
    co_await button.next().clicked;
    co_await lv::sleep(100 * ms);
    ++clicks;
  }
}

lv::task<int> twice(int n) {
  co_await lv::sleep(10 * ms);
  co_return n * 2;
}

lv::task<> chain(int* out) {
  const int a = co_await twice(4);
  const int b = co_await twice(a);
  *out = b;
}

lv::task<> watch_mode(int* seen) {
  const mode m = co_await lv::changed(current_mode);
  *seen = m.value;
}

// Whichever comes first decides the answer.
lv::task<> confirm(lv::view<"button"> ok, lv::view<"button"> cancel, int* answer, bool* ended) {
  at_exit cleanup{ended};
  *answer = co_await lv::first(ok.next().clicked >> [] { return 1; },
                               cancel.next().clicked >> [] { return 2; },
                               lv::sleep(500 * ms) >> [] { return 3; });
}

// A task that wants to see the deletion waits for it.
lv::task<> confirm_or_gone(lv::view<"button"> ok, int* answer) {
  *answer = co_await lv::first(ok.next().clicked >> [] { return 1; },
                               ok.next().gone >> [] { return 4; });
}

// A handler can take the value of what it waited for, and can capture.
lv::task<> mode_or_timeout(int* out) {
  const int base = 100;
  *out = co_await lv::first(lv::changed(current_mode) >> [&](mode m) { return base + m.value; },
                            lv::sleep(50 * ms) >> [] { return -1; });
}

// Without handlers, lv::first says which one won.
lv::task<> which_first(lv::view<"button"> button, std::size_t* which) {
  *which = co_await lv::first(button.next().pressed, button.next().value_changed,
                              lv::sleep(50 * ms));
}

// A child task that waits on a widget; its parent waits on the child.
lv::task<int> wait_click(lv::view<"button"> button) {
  co_await button.next().clicked;
  co_return 7;
}
lv::task<> parent_of_child(lv::view<"button"> button, int* out, bool* ended) {
  at_exit cleanup{ended};
  *out = co_await wait_click(button);
}

void events_and_timers(lv::obj screen) {
  auto button = screen.add<"button">();
  bool ended = false;
  check(lv::spawn(count_clicks(button, &ended)));

  click(button);
  check(clicks.number() == 0);  // still sleeping
  lv_test_wait(150);
  check(clicks.number() == 1);

  click(button);
  lv_test_wait(150);
  check(clicks.number() == 2);

  // Deleting the widget cancels the task: its local objects are destroyed.
  check(!ended);
  lv_obj_delete(button.raw());
  check(ended);

  // The same while the task sleeps: it must not wake up to a deleted button, and
  // its timer must not outlive it.
  auto second = screen.add<"button">();
  ended = false;
  check(lv::spawn(count_clicks(second, &ended)));
  click(second);
  check(!ended);
  lv_obj_delete(second.raw());
  check(ended);
  lv_test_wait(150);
  check(clicks.number() == 2);  // the cancelled task did not count
}

void tasks_await_tasks(lv::obj screen) {
  int result = 0;
  check(lv::spawn(chain(&result)));
  check(result == 0);
  lv_test_wait(15);
  check(result == 0);  // the second step is still waiting
  lv_test_wait(15);
  check(result == 16);

  // Cancelling reaches through a child task to the task that was spawned.
  auto button = screen.add<"button">();
  int out = 0;
  bool ended = false;
  check(lv::spawn(parent_of_child(button, &out, &ended)));
  lv_obj_delete(button.raw());
  check(ended && out == 0);

  auto again = screen.add<"button">();
  ended = false;
  check(lv::spawn(parent_of_child(again, &out, &ended)));
  click(again);
  check(ended && out == 7);
  lv_obj_delete(again.raw());
}

void state_changes() {
  int seen = -1;
  check(lv::spawn(watch_mode(&seen)));
  check(seen == -1);  // subscribing is not a change
  current_mode = mode{3};
  check(seen == 3);
  current_mode = mode{4};  // the task is over: nothing listens any more
  check(seen == 3);
}

void first_of_several(lv::obj screen) {
  const auto before = free_bytes();
  for (int scenario = 1; scenario <= 4; ++scenario) {
    auto ok = screen.add<"button">();
    auto cancel = screen.add<"button">();
    int answer = 0;
    bool ended = false;
    check(lv::spawn(confirm(ok, cancel, &answer, &ended)));
    check(answer == 0 && !ended);

    if (scenario == 1) click(ok);
    if (scenario == 2) click(cancel);
    if (scenario == 3) lv_test_wait(600);
    if (scenario == 4) lv_obj_delete(ok.raw());  // cancels: no answer
    check(answer == (scenario == 4 ? 0 : scenario));
    check(ended);

    // The losers were cancelled: none of this reaches the finished task.
    answer = 0;
    if (scenario != 4) click(ok);
    click(cancel);
    lv_test_wait(600);
    check(answer == 0);

    if (scenario != 4) lv_obj_delete(ok.raw());
    lv_obj_delete(cancel.raw());
  }
  check(free_bytes() == before);  // no timer, handler or frame is left

  // Waiting for the deletion turns it into an ordinary outcome.
  auto ok = screen.add<"button">();
  int answer = 0;
  check(lv::spawn(confirm_or_gone(ok, &answer)));
  lv_obj_delete(ok.raw());
  check(answer == 4);

  int out = 0;
  check(lv::spawn(mode_or_timeout(&out)));
  current_mode = mode{7};
  check(out == 107);
  check(lv::spawn(mode_or_timeout(&out)));
  lv_test_wait(60);
  check(out == -1);

  auto b = screen.add<"button">();
  std::size_t which = 99;
  check(lv::spawn(which_first(b, &which)));
  lv_obj_send_event(b.raw(), LV_EVENT_VALUE_CHANGED, nullptr);
  check(which == 1);
  check(lv::spawn(which_first(b, &which)));
  lv_test_wait(60);
  check(which == 2);
  lv_obj_delete(b.raw());
}

void nothing_is_left_behind(lv::obj screen) {
  // Run the main paths once more and compare the heap before and after.
  const auto before = free_bytes();
  {
    auto button = screen.add<"button">();
    bool ended = false;
    lv::spawn(count_clicks(button, &ended));
    click(button);
    lv_test_wait(150);
    lv_obj_delete(button.raw());
    check(ended);
    int result = 0;
    lv::spawn(chain(&result));
    lv_test_wait(40);
    check(result == 16);
  }
  check(free_bytes() == before);

  // A task that is never started frees its frame too.
  const auto unused_before = free_bytes();
  { auto unused = twice(1); }
  check(free_bytes() == unused_before);
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);
  auto screen = lv::screen_active();

  // Event names are generated from LVGL's list.
  static_assert(std::is_same_v<decltype(screen.next().clicked), lv::event_ref<LV_EVENT_CLICKED>>);
  static_assert(std::is_same_v<decltype(screen.next().gone), lv::event_ref<LV_EVENT_DELETE>>);

  events_and_timers(screen);
  tasks_await_tasks(screen);
  state_changes();
  first_of_several(screen);
  nothing_is_left_behind(screen);

  clicks.reset();
  current_mode.reset();
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
