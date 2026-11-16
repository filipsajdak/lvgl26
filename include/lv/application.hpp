#pragma once

// LVGL's lifetime as an object, and the loop that keeps a user interface alive.
//
//   int main() {
//     lv::application app;                     // LVGL starts
//     lv::display display{app, frame};         // see lv/display.hpp
//     auto root = lv::mount<ui>(display.screen());
//     return app.run();                        // timers, animations, input; until app.quit()
//   }                                          // the widgets, the display and LVGL end, in that order
//
// Declare the application first. What is declared after it ends before it.
//
// Time. LVGL has no clock of its own. The application gets the time in one of three ways:
//
//   lv::application app{{.clock = &since_start}};  // a function that returns the time since the start
//   lv::application app{lv::host};                 // the operating system's clock: lv/host.hpp
//   lv::application app;                           // no clock: the program says how much time passed,
//   app.advance(5 * ms);                           //   from a timer interrupt, or step by step in a test
//
// The loop. run() calls step() until quit(). Between two steps it calls `idle` with the
// time until something is due: sleep there, or wait for an interrupt. Without `idle` it
// calls step() again at once. A program that only draws a picture needs neither.
//
// There is one application at a time: LVGL's state is global.

#include <lv/units.hpp>

#include <lvgl/lvgl.h>

#include <cstddef>
#include <cstdint>

namespace lv {

// LVGL's own heap, in bytes. All zero when LVGL is built to use another allocator.
struct heap_use {
  std::size_t total = 0;         // without the bookkeeping of the blocks, so it varies a little
  std::size_t used = 0;
  std::size_t largest_free = 0;  // the largest block that can still be allocated
};

namespace detail {

// LVGL asks for the time through a function without an argument, so the clock of the
// one application lives here.
inline constinit duration (*application_clock)() = nullptr;

inline std::uint32_t application_time() { return application_clock().ms; }

}  // namespace detail

class application {
 public:
  struct options {
    // The time since the program started. Without it, the program calls advance().
    duration (*clock)() = nullptr;
    // What run() does while nothing is due for the given time.
    void (*idle)(duration) = nullptr;
  };

  application() : application(options{}) {}

  explicit application(options how)
    pre(!lv_is_initialized())
      : idle_(how.idle) {
    lv_init();
    detail::application_clock = how.clock;
    if (how.clock != nullptr) lv_tick_set_cb(&detail::application_time);
  }

  application(const application&) = delete;
  application& operator=(const application&) = delete;

  ~application() {
    lv_deinit();
    detail::application_clock = nullptr;
  }

  // Tells LVGL that `passed` has gone by. Safe to call from an interrupt.
  // Only for an application without a clock.
  void advance(duration passed) { lv_tick_inc(passed.ms); }

  // Runs what is due now: timers, animations, input, drawing.
  // Returns the time until something is due again.
  duration step() { return duration{lv_timer_handler()}; }

  // Steps until quit() is called. Returns the result given to quit().
  int run() {
    while (!quit_) {
      const duration wait = step();
      if (!quit_ && idle_ != nullptr) idle_(wait);
    }
    quit_ = false;
    return result_;
  }

  // How much of LVGL's heap is in use now.
  heap_use heap() const {
    lv_mem_monitor_t now;
    lv_mem_monitor(&now);
    return {now.total_size, now.total_size - now.free_size, now.free_biggest_size};
  }

  // Ends run() after the step that is running. Call it from a handler or a task.
  void quit(int result = 0) {
    result_ = result;
    quit_ = true;
  }

 private:
  void (*idle_)(duration);
  int result_ = 0;
  bool quit_ = false;
};

}  // namespace lv
