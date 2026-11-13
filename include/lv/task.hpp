#pragma once

// Coroutines on LVGL's loop: write a sequence of UI steps as straight code.
//
//   lv::task<> count_clicks(lv::view<"button"> button, lv::state<lv::counter>& clicks) {
//     while (co_await lv::next<LV_EVENT_CLICKED>(button)) {   // false when the button is deleted
//       co_await lv::sleep(100 * lv::ms);
//       clicks.update([](lv::counter c) { return ++c; });
//     }
//   }
//
//   lv::spawn(count_clicks(button, clicks));
//
// Everything runs on the UI thread, inside lv_timer_handler(): a coroutine resumes
// in the event, timer or observer callback it was waiting for. Nothing here is
// thread-safe, like LVGL itself.
//
// A coroutine frame is allocated with lv_malloc. If that fails, the task is empty
// and lv::spawn returns false; no exception is thrown.

#include <lv/state.hpp>
#include <lv/units.hpp>
#include <lv/view.hpp>

#include <coroutine>
#include <exception>
#include <utility>

namespace lv {

template <class T = void>
class task;

namespace detail {

struct task_promise_base {
  std::coroutine_handle<> continuation{};  // who awaits this task, if anyone
  bool detached = false;                   // started by lv::spawn: frees itself

  static void* operator new(std::size_t size) noexcept { return lv_malloc(size); }
  static void operator delete(void* frame) noexcept { lv_free(frame); }
  static void operator delete(void* frame, std::size_t) noexcept { lv_free(frame); }

  std::suspend_always initial_suspend() noexcept { return {}; }

  struct final_awaiter {
    bool await_ready() const noexcept { return false; }
    template <class Promise>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> finished) const noexcept {
      const std::coroutine_handle<> next = finished.promise().continuation;
      if (finished.promise().detached) finished.destroy();
      return next ? next : std::noop_coroutine();
    }
    void await_resume() const noexcept {}
  };
  final_awaiter final_suspend() noexcept { return {}; }

  // The library is built to work without exceptions; one that escapes a task ends the program.
  [[noreturn]] void unhandled_exception() noexcept { std::terminate(); }
};

template <class T>
struct task_promise : task_promise_base {
  T value{};
  task<T> get_return_object() noexcept;
  static task<T> get_return_object_on_allocation_failure() noexcept;
  void return_value(T v) noexcept { value = std::move(v); }
  T result() noexcept { return std::move(value); }
};

template <>
struct task_promise<void> : task_promise_base {
  task<void> get_return_object() noexcept;
  static task<void> get_return_object_on_allocation_failure() noexcept;
  void return_void() noexcept {}
  void result() noexcept {}
};

}  // namespace detail

// A piece of work that can wait. It starts when it is awaited or given to lv::spawn.
template <class T>
class [[nodiscard]] task {
 public:
  using promise_type = detail::task_promise<T>;

  task() = default;
  explicit task(std::coroutine_handle<promise_type> frame) : frame_(frame) {}
  task(task&& other) noexcept : frame_(std::exchange(other.frame_, {})) {}
  task& operator=(task&& other) noexcept {
    if (this != &other) {
      destroy();
      frame_ = std::exchange(other.frame_, {});
    }
    return *this;
  }
  ~task() { destroy(); }

  // False when LVGL had no memory for the coroutine frame.
  explicit operator bool() const noexcept { return static_cast<bool>(frame_); }

  // co_await a task: run it, and continue with its result when it is done.
  auto operator co_await() const noexcept {
    struct awaiter {
      std::coroutine_handle<promise_type> frame;
      bool await_ready() const noexcept { return !frame || frame.done(); }
      std::coroutine_handle<> await_suspend(std::coroutine_handle<> waiting) const noexcept {
        frame.promise().continuation = waiting;
        return frame;  // start the awaited task now
      }
      T await_resume() const noexcept {
        if constexpr (std::is_void_v<T>) {
          return;
        } else {
          contract_assert(frame && "the awaited task could not be allocated");
          return frame.promise().result();
        }
      }
    };
    return awaiter{frame_};
  }

 private:
  std::coroutine_handle<promise_type> frame_{};

  void destroy() {
    if (frame_) frame_.destroy();
    frame_ = {};
  }

  friend bool spawn(task<void> work) noexcept;
};

namespace detail {

template <class T>
task<T> task_promise<T>::get_return_object() noexcept {
  return task<T>{std::coroutine_handle<task_promise<T>>::from_promise(*this)};
}
template <class T>
task<T> task_promise<T>::get_return_object_on_allocation_failure() noexcept {
  return {};
}
inline task<void> task_promise<void>::get_return_object() noexcept {
  return task<void>{std::coroutine_handle<task_promise<void>>::from_promise(*this)};
}
inline task<void> task_promise<void>::get_return_object_on_allocation_failure() noexcept {
  return {};
}

}  // namespace detail

// Starts a task and lets it run on its own; its memory is freed when it finishes.
// Returns false if the task is empty (no memory for its frame).
inline bool spawn(task<void> work) noexcept {
  if (!work) return false;
  const auto frame = std::exchange(work.frame_, {});
  frame.promise().detached = true;
  frame.resume();
  return true;
}

namespace detail {

// Waits for one event on a widget, or for the widget to be deleted.
struct event_awaiter {
  lv_obj_t* widget;
  lv_event_code_t code;
  std::coroutine_handle<> waiting{};
  lv_event_dsc_t* on_code = nullptr;
  lv_event_dsc_t* on_gone = nullptr;
  bool happened = false;

  bool await_ready() const noexcept { return widget == nullptr; }

  void await_suspend(std::coroutine_handle<> h) noexcept {
    waiting = h;
    on_code = lv_obj_add_event_cb(widget, &fired, code, this);
    on_gone = lv_obj_add_event_cb(widget, &gone, LV_EVENT_DELETE, this);
  }

  bool await_resume() const noexcept { return happened; }

  static void fired(lv_event_t* e) { static_cast<event_awaiter*>(lv_event_get_user_data(e))->finish(true); }
  static void gone(lv_event_t* e) { static_cast<event_awaiter*>(lv_event_get_user_data(e))->finish(false); }

  void finish(bool event_happened) {
    happened = event_happened;
    lv_obj_remove_event_dsc(widget, on_code);
    lv_obj_remove_event_dsc(widget, on_gone);
    const auto h = waiting;
    h.resume();  // this awaiter lives in the coroutine frame: do not touch it after this
  }
};

// Waits for a time span, with a one-shot LVGL timer.
struct sleep_awaiter {
  std::uint32_t ms;
  std::coroutine_handle<> waiting{};

  bool await_ready() const noexcept { return false; }

  bool await_suspend(std::coroutine_handle<> h) noexcept {
    waiting = h;
    lv_timer_t* timer = lv_timer_create(&elapsed, ms, this);
    if (timer == nullptr) return false;  // no memory for a timer: go on without waiting
    lv_timer_set_repeat_count(timer, 1);  // LVGL deletes the timer after it has run once
    return true;
  }

  void await_resume() const noexcept {}

  static void elapsed(lv_timer_t* timer) {
    const auto h = static_cast<sleep_awaiter*>(lv_timer_get_user_data(timer))->waiting;
    h.resume();
  }
};

// Waits for the next change of a state.
template <class T, class O>
struct change_awaiter {
  const state<T, O>* source;
  std::coroutine_handle<> waiting{};
  lv_observer_t* observer = nullptr;
  bool armed = false;

  bool await_ready() const noexcept { return source->raw() == nullptr; }

  void await_suspend(std::coroutine_handle<> h) noexcept {
    waiting = h;
    // LVGL calls a new observer once at once; only later calls are changes.
    observer = lv_subject_add_observer(source->raw(), &changed, this);
    armed = true;
  }

  T await_resume() const noexcept { return source->get(); }

  static void changed(lv_observer_t* o, lv_subject_t*) {
    auto* self = static_cast<change_awaiter*>(lv_observer_get_user_data(o));
    if (!self->armed) return;
    const auto h = self->waiting;
    lv_observer_delete(o);
    h.resume();
  }
};

}  // namespace detail

// co_await lv::next<LV_EVENT_CLICKED>(button): true when the event came, false when
// the widget was deleted first.
template <lv_event_code_t Code, detail::fixed W>
detail::event_awaiter next(view<W> widget) noexcept {
  return {widget.raw(), Code};
}

// co_await lv::sleep(300 * lv::ms)
inline detail::sleep_awaiter sleep(duration time) noexcept { return {time.ms}; }

// T now = co_await lv::changed(speed): the value after the state's next change.
template <class T, class O>
detail::change_awaiter<T, O> changed(const state<T, O>& source) noexcept {
  return {&source};
}

}  // namespace lv
