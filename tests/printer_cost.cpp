// What the printer panel (examples/ports/slint/printer) costs while it runs: the heap it holds
// after the home page and after the Print page are open, and what one change of the queue does
// to the widgets that exist: how many are deleted, how many are made, how many heap blocks the
// change leaves behind. The numbers are printed for docs/port-slint-printer.md.

#include "check.hpp"

#include <lv/lv.hpp>
#include <queue.hpp>
#include <scenes.hpp>
#include <shell.hpp>

#include <cstdio>
#include <vector>

namespace {

using namespace printer;

constinit lv::frame<window_width, window_height> frame;

lv_mem_monitor_t heap() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m;
}

std::size_t used_bytes() {
  const lv_mem_monitor_t m = heap();
  return m.total_size - m.free_size;
}

void collect(lv_obj_t* w, std::vector<lv::weak<lv::obj>>& out) {
  out.emplace_back(lv::obj{w});
  for (std::uint32_t i = 0; i < lv_obj_get_child_count(w); ++i) collect(lv_obj_get_child(w, static_cast<std::int32_t>(i)), out);
}

std::size_t widgets(lv::obj screen) {
  std::vector<lv::weak<lv::obj>> all;
  collect(screen.raw(), all);
  return all.size();
}

struct change {
  std::size_t deleted;
  std::size_t made;
  long blocks;  // heap blocks in use after the change, minus before
  long bytes;   // heap bytes in use after the change, minus before
};

// Runs `f`, and tells which widgets of the screen it deleted and made.
template <class F>
change measured(lv::application& app, lv::obj screen, F f) {
  // The drawing makes and frees heap blocks of its own, so it is stopped while the change runs.
  app.step();
  lv_timer_pause(lv_display_get_refr_timer(nullptr));
  std::vector<lv::weak<lv::obj>> before;
  collect(screen.raw(), before);
  const lv_mem_monitor_t was = heap();
  f();
  const lv_mem_monitor_t now = heap();
  lv_timer_resume(lv_display_get_refr_timer(nullptr));
  std::size_t deleted = 0;
  for (const auto& w : before) deleted += w.expired() ? std::size_t{1} : std::size_t{0};
  const std::size_t after = widgets(screen);
  return {deleted, after - (before.size() - deleted),
          static_cast<long>(now.used_cnt) - static_cast<long>(was.used_cnt),
          static_cast<long>(now.total_size - now.free_size) - static_cast<long>(was.total_size - was.free_size)};
}

void show(const char* what, const change& c) {
  std::printf("%-22s deleted %2zu, made %2zu, blocks %+ld, bytes %+ld\n", what, c.deleted, c.made, c.blocks, c.bytes);
}

void a_second(lv::application& app) {
  for (int step = 0; step < 100; ++step) {
    app.advance(10 * lv::ms);
    app.step();
  }
}

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, frame};
  const lv::obj screen = display.screen();
  const std::size_t empty = used_bytes();
  check(start(screen));
  play(app, scenes[0]);
  const std::size_t home = used_bytes();
  const std::size_t home_widgets = widgets(screen);
  play(app, scenes[3]);
  const std::size_t print = used_bytes();
  const std::size_t print_widgets = widgets(screen);
  std::printf("heap: empty %zu, home %zu, print %zu bytes\n", empty, home, print);
  std::printf("widgets: home %zu, print %zu\n", home_widgets, print_widgets);
  check(home > empty && print > home);

  const std::size_t jobs_before = jobs().size();
  // Each change twice: the first run makes what later runs share.
  for (int round = 0; round < 2; ++round) {
    const change tick = measured(app, screen, [&] { a_second(app); });
    const change added = measured(app, screen, [] { start_job("New.pdf"); });
    const change cancelled = measured(app, screen, [] { cancel_job(1); });
    if (round == 1) {
      show("one second of printing", tick);
      show("a job is added", added);
      show("a job is cancelled", cancelled);
    }
  }
  check(jobs().size() == jobs_before);
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
