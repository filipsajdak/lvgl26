#pragma once

// Editor stub of lv/ui_scheduler.hpp: work from another thread, run on the UI thread.

#include <lv/task.hpp>
#include <lv/units.hpp>

#include <concepts>

namespace lv {

namespace detail {
template <class F>
concept postable = std::invocable<F&> && std::move_constructible<F>;
}  // namespace detail

/// Runs work from any thread on the UI thread. Make it on the UI thread.
class ui_scheduler {
 public:
  /// `period`: how often the UI thread looks at the queue.
  explicit ui_scheduler(duration period = duration{LV_DEF_REFR_PERIOD});
  ui_scheduler(const ui_scheduler&) = delete;
  ui_scheduler& operator=(const ui_scheduler&) = delete;
  ~ui_scheduler();

  /// False when LVGL had no memory for the timer.
  explicit operator bool() const noexcept;

  /// From any thread: runs `work` on the UI thread, in a later turn of the LVGL loop.
  /// Returns false if there is no memory to queue it.
  template <detail::postable F>
  bool post(F) { return true; }

  /// In a task: co_await ui.schedule() goes on in a later turn of the loop.
  detail::source<void> schedule() noexcept;
};

}  // namespace lv
