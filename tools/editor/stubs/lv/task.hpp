#pragma once

// Editor stub of lv/task.hpp: coroutines on LVGL's loop.
//
//   lv::task<> count_clicks(lv::view<"button"> button) {
//     for (;;) {
//       co_await button.next().clicked;
//       co_await lv::sleep(100 * lv::ms);
//       ++clicks;
//     }
//   }
//   lv::spawn(count_clicks(button));

#include <lv/events.hpp>
#include <lv/state.hpp>
#include <lv/units.hpp>
#include <lv/view.hpp>

#include <concepts>
#include <coroutine>
#include <cstddef>
#include <new>
#include <type_traits>

namespace lv {

namespace detail {

template <class T>
concept task_result = std::is_void_v<T> || (std::movable<T> && std::default_initializable<T>);

/// What co_await gives: a value of type T.
template <class T>
struct awaiter {
  bool await_ready() const noexcept { __builtin_unreachable(); }
  void await_suspend(std::coroutine_handle<>) const noexcept {}
  T await_resume() const noexcept { __builtin_unreachable(); }
};

/// Something to wait for, with what to do when it is the first in lv::first.
template <class Value, class F>
struct handled {
  F handler;
};

// What the handler of an item returns: it takes the item's value, or nothing.
template <class Value, class F>
struct handler_result : std::invoke_result<F&> {};
template <class Value, class F>
  requires(!std::is_void_v<Value>) && std::invocable<F&, Value>
struct handler_result<Value, F> : std::invoke_result<F&, Value> {};

/// Something a task can wait for: co_await it, or give it to lv::first.
template <class Value>
struct source {
  awaiter<Value> operator co_await() const noexcept { __builtin_unreachable(); }

  /// For lv::first: what to do when this is the first to be done.
  template <class F>
  handled<Value, F> then(F handler) const { return {handler}; }
};

template <class T>
inline constexpr bool is_handled = false;
template <class Value, class F>
inline constexpr bool is_handled<handled<Value, F>> = true;

template <class... R>
struct common_result : std::common_type<R...> {};
template <class... R>
  requires(std::is_void_v<R> && ...)
struct common_result<R...> {
  using type = void;
};

// What lv::first gives: what the winner's handler returns, or the winner's position.
template <class... Items>
struct first_result {
  using type = std::size_t;
};
template <class... Value, class... F>
struct first_result<handled<Value, F>...> : common_result<typename handler_result<Value, F>::type...> {};

template <class T>
struct task_promise;

template <class T>
struct task_promise_result {
  void return_value(T value) noexcept {}
};
template <>
struct task_promise_result<void> {
  void return_void() noexcept;
};

}  // namespace detail

template <detail::task_result T = void>
class task;

namespace detail {

template <class T>
struct task_promise : task_promise_result<T> {
  task<T> get_return_object() noexcept { __builtin_unreachable(); }
  static task<T> get_return_object_on_allocation_failure() noexcept { __builtin_unreachable(); }
  std::suspend_always initial_suspend() const noexcept { __builtin_unreachable(); }
  std::suspend_always final_suspend() const noexcept { __builtin_unreachable(); }
  void unhandled_exception() noexcept {}
};

}  // namespace detail

/// A piece of work that can wait. It starts when it is awaited or given to lv::spawn.
/// It may co_await: an event (view.next().clicked), lv::sleep, lv::changed, lv::animate,
/// lv::first, or another task. Deleting a widget it waited on ends it.
template <detail::task_result T>
class [[nodiscard]] task {
 public:
  using promise_type = detail::task_promise<T>;

  task() = default;
  task(task&&) noexcept {}
  task& operator=(task&&) noexcept { return *this; }
  ~task() {}

  /// False when LVGL had no memory for the coroutine frame.
  explicit operator bool() const noexcept { __builtin_unreachable(); }

  /// co_await a task: run it, and continue with its result when it is done.
  detail::awaiter<T> operator co_await() const noexcept { __builtin_unreachable(); }
};

/// Starts a task and lets it run on its own; its memory is freed when it finishes.
/// Returns false if the task is empty (no memory for its frame).
bool spawn(task<void> work) noexcept;

/// co_await button.next().clicked
template <lv_event_code_t Code>
detail::awaiter<void> operator co_await(event_ref<Code>) noexcept { __builtin_unreachable(); }

template <lv_event_code_t Code>
template <std::invocable F>
detail::handled<void, F> event_ref<Code>::then(F handler) const {
  return {handler};
}

/// co_await lv::sleep(300 * lv::ms): go on after that time.
detail::source<void> sleep(duration time) noexcept;

/// T now = co_await lv::changed(speed): the value after the state's next change.
template <class T, class O>
detail::source<T> changed(const state<T, O>& source) noexcept { __builtin_unreachable(); }

namespace detail {

template <class Item>
struct as_item {
  using type = Item;
};
template <lv_event_code_t Code>
struct as_item<event_ref<Code>> {
  using type = source<void>;
};

}  // namespace detail

/// co_await lv::first(a, b, c): whichever is done first; the others are cancelled.
/// With handlers (a.then([] { ... })) the result is what the winner's handler returns.
/// Without any it is the position of the winner, starting at 0.
template <class... Items>
detail::awaiter<typename detail::first_result<Items...>::type> first(Items... items) noexcept { __builtin_unreachable(); }

}  // namespace lv
