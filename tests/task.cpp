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

// Waits for whichever comes first and says which it was.
lv::task<> confirm(lv::view<"button"> ok, lv::view<"button"> cancel, int* answer) {
  auto outcome = co_await lv::first(lv::next<LV_EVENT_CLICKED>(ok) | lv::as<"accepted">,
                                    lv::next<LV_EVENT_CLICKED>(cancel) | lv::as<"rejected">,
                                    lv::sleep(500 * ms));
  static_assert(std::is_same_v<decltype(outcome),
                               lv::outcome<lv::named<"accepted">, lv::gone, lv::named<"rejected">,
                                           lv::elapsed>>);
  *answer = outcome.match<int>({.accepted = [] { return 1; },
                                .elapsed = [] { return 3; },
                                .gone = [] { return 4; },
                                .rejected = [] { return 2; }});
}

// A case can carry a value, and a handler can capture.
lv::task<> mode_or_timeout(int* out) {
  const int base = 100;
  auto outcome = co_await lv::first(lv::changed(current_mode), lv::sleep(50 * ms));
  *out = outcome.match<int>({.changed_to = [&](lv::changed_to<mode> c) { return base + c.value.value; },
                             .elapsed = [] { return -1; }});
}

// The outcome of a single wait is matched the same way; its case is named after the event.
lv::task<> one_event(lv::view<"button"> button, const char** what) {
  (co_await lv::next<LV_EVENT_VALUE_CHANGED>(button))
      .match({.gone = [&] { *what = "gone"; }, .value_changed = [&] { *what = "value_changed"; }});
}

void first_of_several(lv::obj screen) {
  const auto before = free_bytes();
  for (int scenario = 1; scenario <= 4; ++scenario) {
    auto ok = screen.add<"button">();
    auto cancel = screen.add<"button">();
    int answer = 0;
    check(lv::spawn(confirm(ok, cancel, &answer)));
    check(answer == 0);

    if (scenario == 1) click(ok);
    if (scenario == 2) click(cancel);
    if (scenario == 3) lv_test_wait(600);
    if (scenario == 4) lv_obj_delete(ok.raw());
    check(answer == scenario);

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

  int out = 0;
  check(lv::spawn(mode_or_timeout(&out)));
  current_mode = mode{7};
  check(out == 107);
  check(lv::spawn(mode_or_timeout(&out)));
  lv_test_wait(60);
  check(out == -1);

  const char* what = "";
  auto b = screen.add<"button">();
  check(lv::spawn(one_event(b, &what)));
  lv_obj_send_event(b.raw(), LV_EVENT_VALUE_CHANGED, nullptr);
  check(std::strcmp(what, "value_changed") == 0);
  check(lv::spawn(one_event(b, &what)));
  lv_obj_delete(b.raw());
  check(std::strcmp(what, "gone") == 0);
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
  first_of_several(screen);
  nothing_is_left_behind(screen);

  clicks.reset();
  current_mode.reset();
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
