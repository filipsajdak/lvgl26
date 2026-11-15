// The slice screen on a microcontroller: an Arm Cortex-M4 in QEMU (board mps2-an386).
// It mounts a compile-time tree, runs a task that waits and counts, binds a label to a
// state, and prints what it did and what it cost through the debugger's console.
#include <lv/lv.hpp>

#include <cstdio>

namespace {

using namespace lv::units;

constinit lv::state ticks{lv::counter{}};

constexpr auto ui = lv::column({.size = {320 * px, 240 * px}})[
    lv::label({.id = "title", .text = "Speed"}),
    lv::arc({.id = "speedo", .range = {0, 240}, .size = {110 * px, 110 * px}, .value = 120}),
    lv::label({.id = "count", .text = lv::bind(ticks, "{} ticks")})];

// A task: wait, count, and move the dial, three times.
lv::task<> run(lv::view<"arc"> dial) {
  for (int i = 0; i < 3; ++i) {
    co_await lv::sleep(100 * ms);
    ++ticks;
    dial.set({.value = 60 * (i + 1)});
  }
}

std::size_t heap_used() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.total_size - m.free_size;
}

// One line of the display at a time goes nowhere: there is no panel in QEMU.
std::uint8_t line_buffer[320 * 10 * 2];
void flush(lv_display_t* display, const lv_area_t*, std::uint8_t*) { lv_display_flush_ready(display); }

}  // namespace

int main() {
  lv_init();
  lv_display_t* display = lv_display_create(320, 240);
  lv_display_set_buffers(display, line_buffer, nullptr, sizeof line_buffer, LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(display, &flush);

  const std::size_t before = heap_used();
  auto root = lv::mount<ui>(lv::screen_active());
  if (!root) {
    std::printf("mount failed\n");
    return 1;
  }
  const std::size_t with_screen = heap_used();
  if (!lv::spawn(run(root->get<"speedo">()))) {
    std::printf("no memory for the task\n");
    return 1;
  }
  const std::size_t with_task = heap_used();

  for (int ms_passed = 0; ms_passed < 500; ms_passed += 5) {
    lv_tick_inc(5);
    lv_timer_handler();
  }

  std::printf("lvgl26 on Cortex-M4\n");
  std::printf("screen: %u bytes of LVGL's heap\n", static_cast<unsigned>(with_screen - before));
  std::printf("task:   %u bytes of LVGL's heap\n", static_cast<unsigned>(with_task - with_screen));
  std::printf("ticks:  %d\n", static_cast<int>(ticks.get().value));
  std::printf("dial:   %d\n", static_cast<int>(root->get<"speedo">().get<"value">()));
  std::printf("label:  %s\n", root->get<"count">().get<"text">());
  const bool ok = ticks.get().value == 3 && root->get<"speedo">().get<"value">() == 180;
  std::printf(ok ? "ok\n" : "FAILED\n");
  ticks.reset();
  return ok ? 0 : 1;
}
