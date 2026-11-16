#pragma once

// Editor stub of lv/application.hpp: LVGL's lifetime as an object, and its loop.

#include <lv/units.hpp>

#include <cstddef>

namespace lv {

/// LVGL's own heap, in bytes. All zero when LVGL is built to use another allocator.
struct heap_use {
  std::size_t total = 0;
  std::size_t used = 0;
  std::size_t largest_free = 0;  // the largest block that can still be allocated
};

/// LVGL starts when the application is made, and stops when it goes out of scope.
/// Declare it first. There is one at a time.
class application {
 public:
  /// Where the time comes from, and what the loop does while it waits.
  struct options {
    /// The time since the program started. Without it, the program calls advance().
    duration (*clock)() = nullptr;
    /// What run() does while nothing is due for the given time.
    void (*idle)(duration) = nullptr;
  };

  /// No clock: the program says how much time passed, with advance().
  application();
  /// With a clock: lv::application app{lv::host}, or {{.clock = &since_start}}.
  explicit application(options how);
  application(const application&) = delete;
  application& operator=(const application&) = delete;
  ~application();

  /// Tells LVGL that `passed` has gone by. Safe to call from an interrupt.
  void advance(duration passed);
  /// Runs what is due now: timers, animations, input, drawing.
  /// Returns the time until something is due again.
  duration step();
  /// Steps until quit() is called. Returns the result given to quit().
  int run();
  /// How much of LVGL's heap is in use now.
  heap_use heap() const;
  /// Ends run() after the step that is running. Call it from a handler or a task.
  void quit(int result = 0);
};

}  // namespace lv
