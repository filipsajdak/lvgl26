#include <lv/lv.hpp>
#include <lv/ui_scheduler.hpp>

#include "check.hpp"

#include <array>
#include <cstddef>
#include <thread>
#include <vector>

namespace {

using namespace lv::units;

std::size_t free_bytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.free_size;
}

void click(lv::obj o) { lv_obj_send_event(o.raw(), LV_EVENT_CLICKED, nullptr); }

// Sets a flag when it is destroyed.
struct at_exit {
  bool* flag;
  ~at_exit() { *flag = true; }
};

// Counts its own destruction, once, however often it was moved.
struct counted {
  int* destroyed;
  explicit counted(int* d) : destroyed(d) {}
  counted(counted&& other) noexcept : destroyed(other.destroyed) { other.destroyed = nullptr; }
  ~counted() {
    if (destroyed != nullptr) ++*destroyed;
  }
};

// Work posted from the UI thread itself runs in a later turn, in the order of posting.
void posting_from_the_ui_thread() {
  const auto before = free_bytes();
  {
    lv::ui_scheduler ui{5 * ms};
    check(static_cast<bool>(ui));
    std::vector<int> order;
    for (int i = 0; i < 3; ++i) check(ui.post([&order, i] { order.push_back(i); }));
    check(order.empty());  // not inside post()
    lv_test_wait(10);
    check(order == std::vector<int>{0, 1, 2});
    lv_test_wait(10);
    check(order.size() == 3);  // each ran once

    // Work that posts more work: the new work waits for the next turn.
    int depth = 0;
    check(ui.post([&] {
      depth = 1;
      ui.post([&] { depth = 2; });
    }));
    lv_test_wait(5);
    check(depth == 1);
    lv_test_wait(5);
    check(depth == 2);
  }
  {
    // The period that is used when none is given: one display refresh.
    lv::ui_scheduler ui;
    bool ran = false;
    check(ui.post([&ran] { ran = true; }));
    lv_test_wait(LV_DEF_REFR_PERIOD + 5);
    check(ran);
  }
  check(free_bytes() == before);  // the scheduler's timer is gone
}

// What one worker thread reports back. Only that thread writes it, and the UI
// thread reads it after join().
struct worker_report {
  bool all_posted = true;
};

constexpr int workers = 4;
constexpr int jobs_per_worker = 250;

// What the UI thread has seen. Only posted work touches it, so only the UI thread.
struct seen {
  int total = 0;
  bool in_order = true;
  bool on_ui_thread = true;
  std::array<int, workers> next{};  // the number each worker's next job must carry
};

void post_all(lv::ui_scheduler& ui, int worker, seen& s, std::thread::id ui_thread,
              worker_report& report) {
  for (int n = 0; n < jobs_per_worker; ++n) {
    const bool posted = ui.post([&s, worker, n, ui_thread] {
      const auto w = static_cast<std::size_t>(worker);
      s.in_order = s.in_order && s.next[w] == n;
      s.next[w] = n + 1;
      s.on_ui_thread = s.on_ui_thread && std::this_thread::get_id() == ui_thread;
      ++s.total;
    });
    report.all_posted = report.all_posted && posted;
  }
}

// Real threads post while the UI thread waits, and then while it runs its loop.
void posting_from_other_threads(bool ui_runs_meanwhile) {
  const auto before = free_bytes();
  {
    lv::ui_scheduler ui{1 * ms};
    seen s;
    std::array<worker_report, workers> reports{};
    std::vector<std::thread> threads;
    const auto ui_thread = std::this_thread::get_id();
    for (int w = 0; w < workers; ++w)
      threads.emplace_back(post_all, std::ref(ui), w, std::ref(s), ui_thread,
                           std::ref(reports[static_cast<std::size_t>(w)]));

    if (ui_runs_meanwhile) {
      // The loop ends when the last job has run: the workers post a fixed number.
      while (s.total < workers * jobs_per_worker) lv_test_wait(1);
    }
    for (auto& t : threads) t.join();
    if (!ui_runs_meanwhile) {
      check(s.total == 0);  // nothing runs on the posting threads
      lv_test_wait(2);
    }

    check(s.total == workers * jobs_per_worker);
    check(s.in_order);      // each thread's work in the order it posted it
    check(s.on_ui_thread);
    for (const auto& r : reports) check(r.all_posted);
    lv_test_wait(5);
    check(s.total == workers * jobs_per_worker);  // each ran once
  }
  check(free_bytes() == before);
}

// A task goes on in a later turn.
lv::task<> two_turns(lv::ui_scheduler& ui, int* step, bool* ended) {
  at_exit cleanup{ended};
  co_await ui.schedule();
  *step = 1;
  co_await ui.schedule();
  *step = 2;
}

lv::task<> turn_or_timeout(lv::ui_scheduler& ui, int* answer) {
  *answer = co_await lv::first(ui.schedule().then([] { return 1; }),
                               lv::sleep(1000 * ms).then([] { return 2; }));
}

lv::task<> turn_or_click(lv::ui_scheduler& ui, lv::view<"button"> button, std::size_t* which) {
  *which = co_await lv::first(ui.schedule(), button.next().clicked);
}

// Waits for a click, then for its turn, then for a long time.
lv::task<> click_then_turn(lv::ui_scheduler& ui, lv::view<"button"> button, int* step,
                           bool* ended) {
  at_exit cleanup{ended};
  co_await button.next().clicked;
  *step = 1;
  co_await ui.schedule();
  *step = 2;
  co_await lv::sleep(60000 * ms);
}

lv::task<> set_flag(bool* flag) {
  co_await lv::sleep(5 * ms);
  *flag = true;
}

void tasks_wait_for_their_turn(lv::obj screen) {
  // The first child makes the screen allocate its list of children, which it keeps.
  lv_obj_delete(screen.add<"button">().raw());
  const auto before = free_bytes();
  {
    lv::ui_scheduler ui{5 * ms};
    int step = 0;
    bool ended = false;
    check(lv::spawn(two_turns(ui, &step, &ended)));
    check(step == 0);
    lv_test_wait(6);
    check(step == 1 && !ended);  // the second wait is for the next turn
    lv_test_wait(5);
    check(step == 2 && ended);

    // A turn comes after the work that was posted before it.
    std::vector<int> order;
    step = 0;
    check(ui.post([&order] { order.push_back(1); }));
    check(lv::spawn(two_turns(ui, &step, &ended)));
    check(ui.post([&] { order.push_back(step == 1 ? 2 : -2); }));
    lv_test_wait(10);
    check(order == std::vector<int>{1, 2});

    int answer = 0;
    check(lv::spawn(turn_or_timeout(ui, &answer)));
    lv_test_wait(6);
    check(answer == 1);

    // A turn that loses in lv::first is taken out of the queue.
    auto button = screen.add<"button">();
    std::size_t which = 99;
    check(lv::spawn(turn_or_click(ui, button, &which)));
    click(button);
    check(which == 1);
    lv_test_wait(10);
    check(which == 1);
    check(lv::spawn(turn_or_click(ui, button, &which)));
    lv_test_wait(10);
    check(which == 0);

    // A task that is cancelled while it waits for its turn, among other entries.
    std::vector<int> around;
    step = 0;
    ended = false;
    check(lv::spawn(click_then_turn(ui, button, &step, &ended)));
    check(ui.post([&around] { around.push_back(1); }));
    click(button);
    check(ui.post([&around] { around.push_back(2); }));
    check(step == 1 && !ended);
    lv_obj_delete(button.raw());
    check(ended);
    lv_test_wait(10);
    check(step == 1);  // nothing of the ended task ran later
    check(around == std::vector<int>{1, 2});

    // Another thread starts a task by posting the work that spawns it.
    bool flag = false;
    std::thread starter{[&] { ui.post([&flag] { lv::spawn(set_flag(&flag)); }); }};
    starter.join();
    lv_test_wait(15);
    check(flag);
  }
  check(free_bytes() == before);
}

// What is still queued when the scheduler is destroyed.
void destroying_the_scheduler(lv::obj screen) {
  const auto before = free_bytes();
  auto button = screen.add<"button">();
  int step = 0;
  bool ended = false;
  int destroyed = 0;
  bool ran = false;
  {
    lv::ui_scheduler ui{5 * ms};
    check(ui.post([&ran, token = counted{&destroyed}] { ran = true; }));
    check(lv::spawn(click_then_turn(ui, button, &step, &ended)));
    click(button);
    check(ui.post([&ran, token = counted{&destroyed}] { ran = true; }));
    check(step == 1 && destroyed == 0);
  }
  check(!ran && destroyed == 2);  // posted work is destroyed, not run
  lv_test_wait(20);
  check(step == 1 && !ended);  // the task's turn never comes
  lv_obj_delete(button.raw()); // and it is still ended with its widget
  check(ended);
  check(free_bytes() == before);
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);
  auto screen = lv::screen_active();

  posting_from_the_ui_thread();
  posting_from_other_threads(false);
  posting_from_other_threads(true);
  tasks_wait_for_their_turn(screen);
  destroying_the_scheduler(screen);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
