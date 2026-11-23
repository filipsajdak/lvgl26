// The slice screen on a microcontroller: an Arm Cortex-M4 in QEMU (board mps2-an386).
// It mounts a compile-time tree, runs a task that waits and counts, binds a label to a
// state, and prints what it did and what it cost through the debugger's console.
#include <lv/lv.hpp>

#include <contracts>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <span>

// A contract violation on the device: say what was violated and stop. A program that
// defines this function does not get the standard library's handler, which brings the
// whole exception run-time with it.
void handle_contract_violation(const std::contracts::contract_violation& violation) {
  std::printf("contract violation: %s\n", violation.comment());
  std::abort();
}

namespace {

using namespace lv::units;

constinit lv::state ticks{lv::counter{}};

constexpr auto ui = lv::column(
  {.size = {320 * px, 240 * px}}, lv::label({.id = "title", .text = "Speed"}),
  lv::arc({
    .id    = "speedo",
    .range = {0, 240},
    .size  = {110 * px, 110 * px},
    .value = 120,
  }),
  lv::label({.id = "count", .text = lv::bind(ticks, "{} ticks")})
);

// A task: wait, count, and move the dial, three times.
lv::task<> run(lv::view<"arc"> dial) {
  for (int i = 0; i < 3; ++i) {
    co_await lv::sleep(100 * ms);
    ++ticks;
    dial.set({.value = 60 * (i + 1)});
  }
}

// Ten lines of the display at a time. They go nowhere: there is no panel in QEMU.
constinit lv::band<320, 240, 10> band;
void discard(lv::area, std::span<const std::byte>) {}

}  // namespace

int main() {
  lv::application app;
  lv::display display{app, band, &discard};

  const std::size_t before = app.heap().used;
  auto root                = lv::mount<ui>(display.screen());
  if (!root) {
    std::printf("mount failed\n");
    return 1;
  }
  const std::size_t with_screen = app.heap().used;
  if (!lv::spawn(run(root->get<"speedo">()))) {
    std::printf("no memory for the task\n");
    return 1;
  }
  const std::size_t with_task = app.heap().used;

  for (int ms_passed = 0; ms_passed < 500; ms_passed += 5) {
    app.advance(5 * ms);
    app.step();
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
