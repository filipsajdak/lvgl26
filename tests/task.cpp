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

// Waits for a click, waits a moment, then counts it. Ends when the button is deleted.
lv::task<> count_clicks(lv::view<"button"> button, bool* ended) {
  while (co_await lv::next<LV_EVENT_CLICKED>(button)) {
    co_await lv::sleep(100 * ms);
    clicks.update([](lv::counter c) { return ++c; });
  }
  *ended = true;
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

void events_and_timers(lv::obj screen) {
  auto button = screen.add<"button">();
  bool ended = false;
  check(lv::spawn(count_clicks(button, &ended)));

  click(button);
  check(clicks.get() == lv::counter{0});  // still sleeping
  lv_test_wait(150);
  check(clicks.get() == lv::counter{1});

  click(button);
  lv_test_wait(150);
  check(clicks.get() == lv::counter{2});

  // Deleting the widget ends the loop and frees the coroutine.
  check(!ended);
  lv_obj_delete(button.raw());
  check(ended);
}

void tasks_await_tasks() {
  int result = 0;
  check(lv::spawn(chain(&result)));
  check(result == 0);
  lv_test_wait(15);
  check(result == 0);  // the second step is still waiting
  lv_test_wait(15);
  check(result == 16);
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

void nothing_is_left_behind(lv::obj screen) {
  // Run the whole thing once more and compare the heap before and after.
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

  events_and_timers(screen);
  tasks_await_tasks();
  state_changes();
  nothing_is_left_behind(screen);

  clicks.reset();
  current_mode.reset();
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
