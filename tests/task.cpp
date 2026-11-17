#include <lv/lv.hpp>

#include "check.hpp"
#include "full_heap.hpp"

#include <cstring>
#include <optional>
#include <utility>

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
  *answer = co_await lv::first(ok.next().clicked.then([] { return 1; }),
                               cancel.next().clicked.then([] { return 2; }),
                               lv::sleep(500 * ms).then([] { return 3; }));
}

// A task that wants to see the deletion waits for it.
lv::task<> confirm_or_gone(lv::view<"button"> ok, int* answer) {
  *answer = co_await lv::first(ok.next().clicked.then([] { return 1; }),
                               ok.next().gone.then([] { return 4; }));
}

// A handler can take the value of what it waited for, and can capture.
lv::task<> mode_or_timeout(int* out) {
  const int base = 100;
  *out = co_await lv::first(lv::changed(current_mode).then([&](mode m) { return base + m.value; }),
                            lv::sleep(50 * ms).then([] { return -1; }));
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

// A task that deletes the widget it has waited on, while its own code runs.
lv::task<> deletes_its_own_widget(lv::view<"button"> button, int* steps, bool* ended) {
  at_exit cleanup{ended};
  co_await button.next().clicked;
  *steps = 1;
  lv_obj_delete(button.raw());  // cancels this task, but not in the middle of this line
  *steps = 2;                   // still runs: the task ends when it next waits
  co_await button.next().clicked;  // must not touch the deleted button
  *steps = 3;
}

// The same, with the next wait being a sleep, a child task and lv::first.
lv::task<> deletes_then_waits(lv::view<"button"> button, int kind, int* steps, bool* ended) {
  at_exit cleanup{ended};
  co_await button.next().clicked;
  lv_obj_delete(button.raw());
  *steps = 2;
  if (kind == 0) co_await lv::sleep(10 * ms);
  if (kind == 1) (void)co_await twice(1);
  if (kind == 2) (void)co_await lv::first(button.next().pressed, lv::sleep(10 * ms));
  *steps = 3;
}

void a_task_may_delete_its_own_widget(lv::obj screen) {
  const auto before = free_bytes();
  {
    auto button = screen.add<"button">();
    int steps = 0;
    bool ended = false;
    check(lv::spawn(deletes_its_own_widget(button, &steps, &ended)));
    click(button);
    check(steps == 2 && ended);
  }
  for (int kind = 0; kind < 3; ++kind) {
    auto button = screen.add<"button">();
    int steps = 0;
    bool ended = false;
    check(lv::spawn(deletes_then_waits(button, kind, &steps, &ended)));
    click(button);
    check(steps == 2 && ended);
    lv_test_wait(50);
    check(steps == 2);  // nothing of the ended task ran later
  }
  check(free_bytes() == before);
}

// A task that never waits for a widget: it only sleeps, or waits for a state.
lv::task<> ticks(int* count, bool* ended) {
  at_exit cleanup{ended};
  for (;;) {
    co_await lv::sleep(10 * ms);
    ++*count;
  }
}

lv::task<> follows(const lv::state<lv::counter>& source, int* seen, bool* ended) {
  at_exit cleanup{ended};
  for (;;) *seen = (co_await lv::changed(source)).value;
}

// A task that deletes the widget it belongs to, then goes on to its next wait.
lv::task<> deletes_its_owner(lv::obj owner, int* steps, bool* ended) {
  at_exit cleanup{ended};
  co_await lv::sleep(10 * ms);
  lv_obj_delete(owner.raw());
  *steps = 1;  // still runs: the task ends when it next waits
  co_await lv::sleep(10 * ms);
  *steps = 2;
}

lv::task<> returns_at_once(int* steps) {
  *steps = 1;
  co_return;
}

void a_task_ends_with_its_owner(lv::obj screen) {
  lv_obj_delete(screen.add<"obj">().raw());  // the screen keeps what it needs to have children
  const auto before = free_bytes();
  {
    auto panel = screen.add<"obj">();
    int count = 0;
    bool ended = false;
    check(lv::spawn(ticks(&count, &ended), panel));
    lv_test_wait(35);
    check(count == 3 && !ended);
    lv_obj_delete(panel.raw());
    check(ended);
    lv_test_wait(50);
    check(count == 3);  // nothing of it ran after its owner had gone
  }
  {
    // Without an owner the same task runs on: this is what an owner is for.
    int count = 0;
    bool ended = false;
    auto panel = screen.add<"obj">();
    lv::state<lv::counter> source;
    int seen = -1;
    bool follower_ended = false;
    check(lv::spawn(follows(source, &seen, &follower_ended), panel));
    source = lv::counter{4};
    check(seen == 4 && !follower_ended);
    source = lv::counter{7};  // it waits again inside the notification, and sees the next change
    check(seen == 7);
    source = lv::counter{4};
    check(seen == 4);
    lv_obj_delete(panel.raw());
    check(follower_ended);
    source = lv::counter{5};
    check(seen == 4);
    (void)count, (void)ended;
  }
  {
    auto panel = screen.add<"obj">();
    int steps = 0;
    bool ended = false;
    check(lv::spawn(deletes_its_owner(panel, &steps, &ended), panel));
    lv_test_wait(15);
    check(steps == 1 && ended);
    lv_test_wait(50);
    check(steps == 1);
  }
  {
    // A task that finishes at once is gone before its owner; the owner is deleted without it.
    auto panel = screen.add<"obj">();
    int steps = 0;
    auto work = returns_at_once(&steps);
    const bool started = lv::spawn(std::move(work), panel);
    check(started && steps == 1);
    lv::task<> none;  // an empty task: what a coroutine gives when LVGL has no memory for it
    check(!lv::spawn(std::move(none), panel));
    lv_obj_delete(panel.raw());
  }
  check(free_bytes() == before);
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

// Waits on two widgets in turn, then for a long time.
lv::task<> two_buttons(lv::view<"button"> a, lv::view<"button"> b, bool* ended) {
  at_exit cleanup{ended};
  co_await a.next().clicked;
  co_await b.next().clicked;
  co_await lv::sleep(60000 * ms);
}

// Deleting any widget a task has waited on ends it, also the second one, and
// whichever of the two has the lower address.
void a_task_is_tied_to_every_widget(lv::obj screen) {
  for (int order = 0; order < 2; ++order) {
    lv::view<"button"> pair[2] = {screen.add<"button">(), screen.add<"button">()};
    const auto first = pair[order];
    const auto second = pair[1 - order];
    bool ended = false;
    check(lv::spawn(two_buttons(first, second, &ended)));
    click(first);
    click(second);
    check(!ended);
    lv_obj_delete(second.raw());
    check(ended);
    lv_obj_delete(first.raw());
  }
}

// Waits for the deletion or a timeout, then for a long time.
lv::task<> gone_or_timeout(lv::view<"button"> button, int* out, bool* ended) {
  at_exit cleanup{ended};
  *out = co_await lv::first(button.next().gone.then([] { return 1; }),
                            lv::sleep(20 * ms).then([] { return 2; }));
  co_await lv::sleep(60000 * ms);
}

// A wait for the deletion that lost: the task is tied to the widget again.
void a_lost_wait_for_deletion(lv::obj screen) {
  auto button = screen.add<"button">();
  int out = 0;
  bool ended = false;
  check(lv::spawn(gone_or_timeout(button, &out, &ended)));
  lv_test_wait(40);
  check(out == 2 && !ended);
  lv_obj_delete(button.raw());
  check(ended && out == 2);
}

// A task can be moved into another, in either direction in memory, and onto itself.
void moving_a_task() {
  const auto before = free_bytes();
  {
    lv::task<int> pair[2] = {twice(1), twice(2)};
    pair[0] = std::move(pair[1]);
    check(pair[0] && !pair[1]);
    pair[1] = std::move(pair[0]);
    check(pair[1] && !pair[0]);
    auto& same = pair[1];
    pair[1] = std::move(same);
    check(static_cast<bool>(pair[1]));
  }
  check(free_bytes() == before);
}

lv::task<> one_click(lv::view<"button"> button) { co_await button.next().clicked; }
lv::task<> one_sleep() { co_await lv::sleep(1 * sec); }

// What a waiting task takes from LVGL's heap, in pointers, so that the limit holds on
// a 32-bit target as on this one: its frame, and LVGL's timer or event registration.
// The limits are a little above what an unoptimized build measures (29 and 39 pointers,
// on both LVGL versions); a change that makes a task larger must raise them on purpose.
void a_task_is_small(lv::obj screen) {
  // Naming an event costs one pointer, whatever the number of events LVGL has.
  static_assert(sizeof(screen.next()) == sizeof(void*));
  static_assert(sizeof(screen.next().clicked) == sizeof(void*));

  const auto nothing = free_bytes();
  auto button = screen.add<"button">();
  // LVGL makes a widget's event list at the first handler, and keeps it.
  check(lv::spawn(one_click(button)));
  click(button);

  const auto before = free_bytes();
  check(lv::spawn(one_sleep()));
  const auto sleeping = before - free_bytes();
  check(lv::spawn(one_click(button)));
  const auto waiting = before - sleeping - free_bytes();
  check(sleeping <= 32 * sizeof(void*));
  check(waiting <= 44 * sizeof(void*));
  if (sleeping > 32 * sizeof(void*) || waiting > 44 * sizeof(void*))
    std::printf("a sleeping task takes %zu bytes, one that waits for a click %zu\n", sleeping, waiting);

  lv_obj_delete(button.raw());  // ends the task that waits for the click
  lv_test_wait(1100);           // and the sleep ends by itself
  check(free_bytes() == nothing);
}

// Resetting a state deletes its subject, and LVGL frees every observer on it, also the
// one of a task that waits for a change. Ending the task later must not delete that
// observer again: LVGL may have given its memory to a new observer by then.
void a_reset_state_leaves_its_waiting_task_safe(lv::obj screen) {
  auto panel = screen.add<"obj">();
  lv::state<lv::counter> source;
  int seen = -1;
  bool ended = false;
  check(lv::spawn(follows(source, &seen, &ended), panel));
  source.reset();
  int calls = 0;
  check(source.observe([p = &calls](lv::counter) { ++*p; }) != nullptr);
  lv_obj_delete(panel.raw());  // ends the task, which must leave the new observer alone
  check(ended);
  const int before = calls;
  source = lv::counter{9};
  check(calls == before + 1);
  check(lv_mem_test() == LV_RESULT_OK);
  source.reset();
}

// Gives the heap back, then sleeps for a moment.
lv::task<> frees_the_heap_then_sleeps(std::optional<full_heap>* heap, bool* ended) {
  at_exit cleanup{ended};
  heap->reset();
  co_await lv::sleep(10 * ms);
}

// Without memory to tie a task to its owner, lv::spawn must not start it: the task
// would outlive the owner it was given.
void a_task_without_a_tie_to_its_owner_does_not_start(lv::obj screen) {
  auto panel = screen.add<"obj">();
  bool ended = false;
  std::optional<full_heap> heap;
  auto work = frees_the_heap_then_sleeps(&heap, &ended);
  heap.emplace(sizeof(lv::detail::widget_watch));  // the record fits, its LVGL handler not
  const bool started = lv::spawn(std::move(work), panel);
  heap.reset();
  check(!started);
  lv_obj_delete(panel.raw());
  check(!started || ended);
}

// Without memory to tie a task to a widget it waits on, the task ends at once:
// it must not wait on, and later touch, a widget that may be deleted.
void a_wait_without_a_tie_to_its_widget_ends_the_task(lv::obj screen) {
  auto button = screen.add<"button">();
  bool ended = false;
  auto work = count_clicks(button, &ended);
  std::optional<full_heap> heap{std::in_place, sizeof(lv::detail::widget_watch)};
  check(lv::spawn(std::move(work)));
  heap.reset();
  check(ended);
  lv_obj_delete(button.raw());
  check(ended);
}

// Waits for the deletion of a widget, then for a long time.
lv::task<> waits_for_the_end_of(lv::view<"button"> button, bool* gone, bool* ended) {
  at_exit cleanup{ended};
  co_await button.next().gone;
  *gone = true;
  co_await lv::sleep(60000 * ms);
}

// Without memory to tie a task to the widget whose deletion it waits for, the task
// ends at once: it must not wait for ever, still holding the widget.
void a_wait_for_deletion_without_a_tie_ends_the_task(lv::obj screen) {
  auto button = screen.add<"button">();
  bool gone = false;
  bool ended = false;
  auto work = waits_for_the_end_of(button, &gone, &ended);
  std::optional<full_heap> heap{std::in_place, sizeof(lv::detail::widget_watch)};
  check(lv::spawn(std::move(work)));
  heap.reset();
  check(ended && !gone);
  lv_obj_delete(button.raw());
  check(ended);
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

  a_task_ends_with_its_owner(screen);
  events_and_timers(screen);
  tasks_await_tasks(screen);
  state_changes();
  first_of_several(screen);
  a_task_may_delete_its_own_widget(screen);
  a_task_is_tied_to_every_widget(screen);
  a_lost_wait_for_deletion(screen);
  moving_a_task();
  a_task_is_small(screen);
  a_reset_state_leaves_its_waiting_task_safe(screen);
  a_task_without_a_tie_to_its_owner_does_not_start(screen);
  a_wait_without_a_tie_to_its_widget_ends_the_task(screen);
  a_wait_for_deletion_without_a_tie_ends_the_task(screen);
  nothing_is_left_behind(screen);

  clicks.reset();
  current_mode.reset();
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
