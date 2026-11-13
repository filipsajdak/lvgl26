#pragma once

// Coroutines on LVGL's loop: write a sequence of UI steps as straight code.
//
//   lv::task<> count_clicks(lv::view<"button"> button) {
//     for (;;) {
//       co_await button.next().clicked;
//       co_await lv::sleep(100 * lv::ms);
//       ++clicks;
//     }
//   }
//
//   lv::spawn(count_clicks(button));
//
// What can be awaited:
//   view.next().clicked      one event of a widget (any LVGL event, by name)
//   lv::sleep(300 * lv::ms)  a time span
//   lv::changed(state)       the next change of a state; gives the new value
//   another lv::task<T>      gives its result
//   lv::first(...)           whichever of several comes first; see below
//
// A deleted widget cancels the task. Once a task has waited for an event of a
// widget, it is tied to that widget: when the widget is deleted, the task ends,
// whatever it is doing at that moment. Its frame is destroyed and the destructors
// of its local objects run. So the loop above needs no exit condition, and a task
// never wakes up to a widget that is gone.
// To react to the deletion instead, wait for it: view.next().gone.
//
// Everything runs on the UI thread, inside lv_timer_handler(): a coroutine resumes
// in the event, timer or observer callback it was waiting for. Nothing here is
// thread-safe, like LVGL itself.
//
// A coroutine frame is allocated with lv_malloc. If that fails, the task is empty
// and lv::spawn returns false; no exception is thrown.

#include <lv/events.hpp>
#include <lv/state.hpp>
#include <lv/units.hpp>
#include <lv/view.hpp>

#include <array>
#include <coroutine>
#include <exception>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace lv {

template <class T = void>
class task;

namespace detail {

struct task_promise_base;
inline void cancel(task_promise_base* waiting) noexcept;

// A widget that a spawned task depends on. Deleting the widget cancels the task.
struct widget_watch {
  lv_obj_t* widget;
  lv_event_dsc_t* on_delete;  // null while the task itself waits for the deletion
  task_promise_base* task;
  widget_watch* next;
};

// What a resume in progress needs to know afterwards, kept on the resumer's stack
// because the task's frame may be gone by then.
struct resume_state {
  bool frame_destroyed = false;  // the task finished and freed itself
  bool cancel_requested = false; // the task must end once it is suspended again
};

struct task_promise_base {
  resume_state* running = nullptr;         // set on the spawned task while its code runs
  std::coroutine_handle<> continuation{};  // who awaits this task, if anyone
  std::coroutine_handle<> self{};          // this task's own frame
  task_promise_base* parent = nullptr;     // the promise of who awaits this task
  widget_watch* watches = nullptr;         // the widgets this spawned task depends on
  bool detached = false;                   // started by lv::spawn: frees itself

  task_promise_base() = default;
  task_promise_base(const task_promise_base&) = delete;
  task_promise_base& operator=(const task_promise_base&) = delete;

  ~task_promise_base() {
    if (running != nullptr) running->frame_destroyed = true;
    while (watches != nullptr) {
      widget_watch* w = watches;
      watches = w->next;
      if (w->widget != nullptr && w->on_delete != nullptr)
        lv_obj_remove_event_dsc(w->widget, w->on_delete);
      lv_free(w);
    }
  }

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

// The spawned task that `waiting` belongs to.
inline task_promise_base* root_of(task_promise_base* waiting) noexcept {
  while (waiting->parent != nullptr) waiting = waiting->parent;
  return waiting;
}

// Ends the spawned task that `waiting` belongs to, with everything it awaits: its
// frame is destroyed, which destroys the frames of the tasks it holds.
//
// A frame must not be destroyed while its code runs. That happens when a task
// deletes a widget it has waited on. The request is then recorded, and
// resume_task() destroys the frame as soon as the task is suspended again.
//
// Whatever called this may live in one of those frames and must not touch itself
// afterwards.
inline void cancel(task_promise_base* waiting) noexcept {
  task_promise_base* root = root_of(waiting);
  if (!root->detached) return;
  if (root->running != nullptr)
    root->running->cancel_requested = true;
  else
    root->self.destroy();
}

// Whether the task that `waiting` belongs to is about to be ended. A task in this
// state must not start waiting for anything: it only needs to get suspended.
inline bool cancel_pending(task_promise_base* waiting) noexcept {
  const task_promise_base* root = root_of(waiting);
  return root->running != nullptr && root->running->cancel_requested;
}

// Resumes a coroutine of the spawned task that `owner` belongs to. Every resume
// goes through here, so that the task knows when its code runs.
inline void resume_task(task_promise_base* owner, std::coroutine_handle<> frame) noexcept {
  task_promise_base* root = root_of(owner);
  resume_state state{};
  root->running = &state;
  frame.resume();
  if (state.frame_destroyed) return;  // the task finished: `root` is gone
  root->running = nullptr;
  if (state.cancel_requested) root->self.destroy();
}

inline void widget_deleted(lv_event_t* e) {
  auto* w = static_cast<widget_watch*>(lv_event_get_user_data(e));
  w->widget = nullptr;  // LVGL frees the widget's event list itself
  w->on_delete = nullptr;
  cancel(w->task);
}

inline widget_watch* find_watch(task_promise_base* root, lv_obj_t* widget) noexcept {
  for (widget_watch* w = root->watches; w != nullptr; w = w->next)
    if (w->widget == widget) return w;
  return nullptr;
}

// Ties the spawned task to `widget`: from now on, deleting the widget cancels the
// task, whatever it is waiting for at that moment. Returns the record, or null if
// LVGL has no memory (the task is then not protected for this widget).
inline widget_watch* watch(task_promise_base* waiting, lv_obj_t* widget, bool cancel_on_delete) {
  task_promise_base* root = root_of(waiting);
  widget_watch* w = find_watch(root, widget);
  if (w == nullptr) {
    w = static_cast<widget_watch*>(lv_malloc(sizeof(widget_watch)));
    if (w == nullptr) return nullptr;
    *w = {widget, nullptr, root, root->watches};
    root->watches = w;
    if (cancel_on_delete) w->on_delete = lv_obj_add_event_cb(widget, &widget_deleted, LV_EVENT_DELETE, w);
  }
  return w;
}

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

// Only a coroutine of ours can wait for these things: cancelling needs its promise.
template <class P>
concept task_promise_type = std::derived_from<P, task_promise_base>;

// What co_await on a task does.
template <class T>
struct task_awaiter {
  std::coroutine_handle<task_promise<T>> frame;

  bool await_ready() const noexcept { return !frame || frame.done(); }

  template <task_promise_type P>
  std::coroutine_handle<> await_suspend(std::coroutine_handle<P> waiting) const noexcept {
    frame.promise().continuation = waiting;
    frame.promise().parent = &waiting.promise();
    if (cancel_pending(&waiting.promise())) return std::noop_coroutine();  // just get suspended
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
  detail::task_awaiter<T> operator co_await() const noexcept { return {frame_}; }

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
  const auto frame = std::coroutine_handle<task_promise<T>>::from_promise(*this);
  self = frame;
  return task<T>{frame};
}
template <class T>
task<T> task_promise<T>::get_return_object_on_allocation_failure() noexcept {
  return {};
}
inline task<void> task_promise<void>::get_return_object() noexcept {
  const auto frame = std::coroutine_handle<task_promise<void>>::from_promise(*this);
  self = frame;
  return task<void>{frame};
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
  detail::resume_task(&frame.promise(), frame);
  return true;
}

namespace detail {

// ---- things to wait for ------------------------------------------------------
//
// A source can be armed with a function to call when it is done and disarmed
// before that. arm() returns false when the source is done already and will not
// call back. A source disarms itself when it is destroyed, which is what makes
// cancelling a task safe.

template <class S>
concept wait_source =
    requires(S s, void (*done)(void*), void* context, task_promise_base* owner) {
      { s.arm(done, context, owner) } -> std::same_as<bool>;
      s.disarm();
    };

// One event of a widget. Waiting for it ties the task to the widget: see watch().
template <lv_event_code_t Code>
struct event_source {
  lv_obj_t* widget = nullptr;
  void (*done)(void*) = nullptr;
  void* context = nullptr;
  lv_event_dsc_t* on_code = nullptr;

  event_source() = default;
  explicit event_source(lv_obj_t* w) : widget(w) {}
  event_source(event_source&&) = default;  // only moved before it is armed
  ~event_source() { disarm(); }

  bool arm(void (*done_fn)(void*), void* done_context, task_promise_base* owner) noexcept {
    contract_assert(widget != nullptr && "waiting for an event of an empty view");
    done = done_fn;
    context = done_context;
    // Without the record that ties the task to the widget, a later deletion would
    // leave the task waiting on freed memory. So if LVGL has no memory for it, the
    // task ends here instead of waiting unprotected.
    if (watch(owner, widget, true) == nullptr) {
      cancel(owner);
      return true;  // stay suspended, with nothing armed, until the task is ended
    }
    on_code = lv_obj_add_event_cb(widget, &fired_cb, Code, this);
    return true;
  }

  void disarm() noexcept {
    if (on_code != nullptr) lv_obj_remove_event_dsc(widget, on_code);
    on_code = nullptr;
  }

  static void fired_cb(lv_event_t* e) {
    auto* self = static_cast<event_source*>(lv_event_get_user_data(e));
    self->disarm();
    self->done(self->context);  // may destroy this source: nothing after this line
  }
};

// The deletion of a widget: view.next().gone. While a task waits for it, deleting
// the widget does not cancel the task; the wait is done instead.
template <>
struct event_source<LV_EVENT_DELETE> {
  lv_obj_t* widget = nullptr;
  void (*done)(void*) = nullptr;
  void* context = nullptr;
  lv_event_dsc_t* on_code = nullptr;
  widget_watch* record = nullptr;

  event_source() = default;
  explicit event_source(lv_obj_t* w) : widget(w) {}
  event_source(event_source&&) = default;
  ~event_source() { disarm(); }

  bool arm(void (*done_fn)(void*), void* done_context, task_promise_base* owner) noexcept {
    contract_assert(widget != nullptr && "waiting for the deletion of an empty view");
    done = done_fn;
    context = done_context;
    // Take over from the task's own watch of this widget, if there is one.
    record = watch(owner, widget, false);
    if (record != nullptr && record->on_delete != nullptr) {
      lv_obj_remove_event_dsc(widget, record->on_delete);
      record->on_delete = nullptr;
    }
    on_code = lv_obj_add_event_cb(widget, &fired_cb, LV_EVENT_DELETE, this);
    return true;
  }

  // Stops waiting. If the widget still exists, the task is tied to it again.
  void disarm() noexcept {
    if (on_code == nullptr) return;
    lv_obj_remove_event_dsc(widget, on_code);
    on_code = nullptr;
    if (record != nullptr)
      record->on_delete = lv_obj_add_event_cb(widget, &widget_deleted, LV_EVENT_DELETE, record);
  }

  static void fired_cb(lv_event_t* e) {
    auto* self = static_cast<event_source*>(lv_event_get_user_data(e));
    self->on_code = nullptr;  // LVGL frees the widget's event list itself
    if (self->record != nullptr) self->record->widget = nullptr;
    self->done(self->context);
  }
};

// A time span, with a one-shot LVGL timer.
struct sleep_source {
  std::uint32_t ms = 0;
  void (*done)(void*) = nullptr;
  void* context = nullptr;
  lv_timer_t* timer = nullptr;

  explicit sleep_source(std::uint32_t time) : ms(time) {}
  sleep_source(sleep_source&&) = default;
  ~sleep_source() { disarm(); }

  bool arm(void (*done_fn)(void*), void* done_context, task_promise_base*) noexcept {
    done = done_fn;
    context = done_context;
    timer = lv_timer_create(&elapsed_cb, ms, this);
    if (timer == nullptr) return false;   // no memory for a timer: do not wait
    lv_timer_set_repeat_count(timer, 1);  // LVGL deletes the timer after it has run once
    return true;
  }

  void disarm() noexcept {
    if (timer != nullptr) lv_timer_delete(timer);
    timer = nullptr;
  }

  static void elapsed_cb(lv_timer_t* t) {
    auto* self = static_cast<sleep_source*>(lv_timer_get_user_data(t));
    self->timer = nullptr;  // LVGL deletes it when this callback returns
    self->done(self->context);
  }
};

// The next change of a state.
template <class T, class O>
struct change_source {
  const state<T, O>* source = nullptr;
  void (*done)(void*) = nullptr;
  void* context = nullptr;
  lv_observer_t* observer = nullptr;
  bool armed = false;

  explicit change_source(const state<T, O>* s) : source(s) {}
  change_source(change_source&&) = default;
  ~change_source() { disarm(); }

  bool arm(void (*done_fn)(void*), void* done_context, task_promise_base*) noexcept {
    if (source->raw() == nullptr) return false;
    done = done_fn;
    context = done_context;
    // LVGL calls a new observer once at once; only later calls are changes.
    observer = lv_subject_add_observer(source->raw(), &changed_cb, this);
    armed = observer != nullptr;
    return armed;
  }

  void disarm() noexcept {
    if (observer != nullptr) lv_observer_delete(observer);
    observer = nullptr;
    armed = false;
  }

  // What the wait gives: the state's new value.
  T value() const noexcept { return source->get(); }

  static void changed_cb(lv_observer_t* o, lv_subject_t*) {
    auto* self = static_cast<change_source*>(lv_observer_get_user_data(o));
    if (!self->armed) return;
    self->disarm();
    self->done(self->context);
  }
};

// co_await of one source.
template <wait_source S>
struct wait {
  S source;
  std::coroutine_handle<> waiting{};
  task_promise_base* owner = nullptr;

  bool await_ready() const noexcept { return false; }

  template <task_promise_type P>
  bool await_suspend(std::coroutine_handle<P> h) noexcept {
    waiting = h;
    owner = &h.promise();
    if (cancel_pending(owner)) return true;  // the task is ending: just get suspended
    return source.arm(&done, this, owner);
  }

  // Two plain functions instead of one with a computed return type: GCC's analyzer
  // reports an uninitialized result for the latter when the type is void.
  auto await_resume() noexcept
    requires requires(S& s) { s.value(); }
  {
    return source.value();
  }
  void await_resume() noexcept
    requires(!requires(S& s) { s.value(); })
  {}

  static void done(void* context) {
    auto* self = static_cast<wait*>(context);
    resume_task(self->owner, self->waiting);
  }
};

// A source with what to do when it wins: view.next().clicked >> [] { ... }.
template <wait_source S, class F>
struct handled {
  S source;
  F handler;
};

template <class T>
inline constexpr bool is_handled = false;
template <class S, class F>
inline constexpr bool is_handled<handled<S, F>> = true;

template <wait_source S>
S& source_of(S& s) {
  return s;
}
template <class S, class F>
S& source_of(handled<S, F>& h) {
  return h.source;
}

// Calls the handler of an item that won, with the source's value if it takes one.
template <class S, class F>
decltype(auto) run(handled<S, F>& h) {
  if constexpr (requires { h.handler(h.source.value()); })
    return h.handler(h.source.value());
  else
    return h.handler();
}

template <class S, class F>
using result_of = decltype(run(std::declval<handled<S, F>&>()));

template <class... R>
struct common_result : std::common_type<R...> {};
template <class... R>
  requires(std::is_void_v<R> && ...)
struct common_result<R...> {
  using type = void;
};

// co_await lv::first(...): the first of several to be done. The others are cancelled.
//
//   int answer = co_await lv::first(ok.next().clicked     >> [] { return 1; },
//                                   cancel.next().clicked >> [] { return 2; },
//                                   lv::sleep(5 * lv::sec) >> [] { return 3; });
//
// With handlers, the result is what the winner's handler returns. Without any,
// the result is the position of the winner, starting at 0.
template <class... Items>
struct first_of {
  static constexpr bool with_handlers = (is_handled<Items> && ...);
  static_assert(with_handlers || (!is_handled<Items> && ...),
                "lv: in lv::first, give a handler to every item (a >> [] { ... }) or to none.");

  static constexpr std::size_t none = static_cast<std::size_t>(-1);

  struct slot {
    first_of* self;
    std::size_t index;
  };

  std::tuple<Items...> items;
  std::array<slot, sizeof...(Items)> slots{};
  std::coroutine_handle<> waiting{};
  task_promise_base* owner = nullptr;
  std::size_t winner = none;

  explicit first_of(Items... i) : items(std::move(i)...) {}

  bool await_ready() const noexcept { return false; }

  template <task_promise_type P>
  bool await_suspend(std::coroutine_handle<P> h) noexcept {
    waiting = h;
    owner = &h.promise();
    template for (constexpr auto i : indices<sizeof...(Items)>) {
      // Stop arming as soon as one is done already, or the task is ending.
      if (winner == none && !cancel_pending(owner)) {
        slots[i] = {this, i};
        if (!source_of(std::get<i>(items)).arm(&done, &slots[i], owner)) winner = i;
      }
    }
    if (cancel_pending(owner)) return true;  // just get suspended
    if (winner == none) return true;
    disarm_others();
    return false;
  }

  auto await_resume() noexcept {
    if constexpr (with_handlers) {
      using result = typename common_result<decltype(run(std::declval<Items&>()))...>::type;
      template for (constexpr auto i : indices<sizeof...(Items)>) {
        if (winner == i) return static_cast<result>(run(std::get<i>(items)));
      }
      std::unreachable();
    } else {
      return winner;
    }
  }

  static void done(void* context) {
    const auto* s = static_cast<slot*>(context);
    first_of* self = s->self;
    if (self->winner != none) return;
    self->winner = s->index;
    self->disarm_others();
    resume_task(self->owner, self->waiting);
  }

 private:
  void disarm_others() noexcept {
    template for (constexpr auto i : indices<sizeof...(Items)>) {
      if (winner != i) source_of(std::get<i>(items)).disarm();
    }
  }
};

// A named event of a widget, as something to wait for.
template <lv_event_code_t Code>
event_source<Code> as_source(event_ref<Code> e) noexcept {
  return event_source<Code>{e.widget};
}
template <wait_source S>
S as_source(S s) noexcept {
  return s;
}
template <class S, class F>
handled<S, F> as_source(handled<S, F> h) noexcept {
  return h;
}

template <class T>
concept awaitable_item = requires(T t) { as_source(std::move(t)); };

template <wait_source S>
wait<S> operator co_await(S source) noexcept {
  return {std::move(source)};
}

// source >> handler, for lv::first. In this namespace because the sources are.
template <wait_source S, class F>
handled<S, F> operator>>(S source, F handler) noexcept {
  return {std::move(source), std::move(handler)};
}

}  // namespace detail

// co_await button.next().clicked
template <lv_event_code_t Code>
detail::wait<detail::event_source<Code>> operator co_await(event_ref<Code> e) noexcept {
  return {detail::event_source<Code>{e.widget}};
}

// button.next().clicked >> handler, for lv::first.
template <lv_event_code_t Code, class F>
detail::handled<detail::event_source<Code>, F> operator>>(event_ref<Code> e, F handler) noexcept {
  return {detail::event_source<Code>{e.widget}, std::move(handler)};
}

// co_await lv::sleep(300 * lv::ms)
inline detail::sleep_source sleep(duration time) noexcept { return detail::sleep_source{time.ms}; }

// T now = co_await lv::changed(speed): the value after the state's next change.
template <class T, class O>
detail::change_source<T, O> changed(const state<T, O>& source) noexcept {
  return detail::change_source<T, O>{&source};
}

// co_await lv::first(a, b, c): whichever is done first. See detail::first_of.
template <detail::awaitable_item... Items>
auto first(Items... items) noexcept {
  return detail::first_of<decltype(detail::as_source(std::move(items)))...>{
      detail::as_source(std::move(items))...};
}

}  // namespace lv
