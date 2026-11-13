#pragma once

// Coroutines on LVGL's loop: write a sequence of UI steps as straight code.
//
//   lv::task<> count_clicks(lv::view<"button"> button) {
//     while (co_await lv::next<LV_EVENT_CLICKED>(button)) {   // until the button is deleted
//       co_await lv::sleep(100 * lv::ms);
//       clicks.update([](lv::counter c) { return ++c; });
//     }
//   }
//
//   lv::spawn(count_clicks(button));
//
// What a wait ended with is an outcome, matched by name: see "outcomes" below.
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

// ---- outcomes ---------------------------------------------------------------
//
// What a wait ended with. An outcome is one of a fixed set of cases, and match()
// takes one handler per case, by name:
//
//   auto outcome = co_await lv::first(
//       lv::next<LV_EVENT_CLICKED>(ok)     | lv::as<"accepted">,
//       lv::next<LV_EVENT_CLICKED>(cancel) | lv::as<"rejected">,
//       lv::sleep(5 * lv::sec));
//
//   co_return outcome.match<bool>({
//       .accepted = [] { return true; },
//       .elapsed  = [] { return false; },   // the time ran out
//       .gone     = [] { return false; },   // a button was deleted
//       .rejected = [] { return false; }});
//
// The handler struct is generated by reflection from the cases, sorted by name.
// Every case needs a handler: leaving one out does not compile.

// The event happened. Its case is named after the event: clicked, value_changed, ...
template <lv_event_code_t Code>
struct fired {};

// The widget was deleted while something waited on it.
struct gone {};

// The time of an lv::sleep ran out.
struct elapsed {};

// A state changed; `value` is its new value.
template <class T>
struct changed_to {
  T value{};
};

// A case with a name of your choice: see lv::as.
template <detail::fixed Name>
struct named {};

namespace detail {

// "LV_EVENT_VALUE_CHANGED" -> "value_changed".
consteval std::string_view event_case_name(lv_event_code_t code) {
  for (auto e : std::meta::enumerators_of(std::meta::dealias(^^lv_event_code_t)))
    if (std::meta::extract<lv_event_code_t>(e) == code) {
      std::string out;
      for (char c : std::meta::identifier_of(e).substr(std::string_view("LV_EVENT_").size()))
        out.push_back((c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c);
      return std::define_static_string(out);
    }
  throw unknown_name("lv: this event code has no name");
}

// The member name of a case in match(): the type's name, or what was given to lv::as.
template <class T>
inline constexpr std::string_view case_name = std::meta::identifier_of(
    std::meta::has_template_arguments(^^T) ? std::meta::template_of(^^T) : ^^T);
template <fixed Name>
inline constexpr std::string_view case_name<named<Name>> = Name.sv();
template <lv_event_code_t Code>
inline constexpr std::string_view case_name<fired<Code>> = event_case_name(Code);

// One handler of a match. It refers to a callable that takes the case, or nothing.
template <class R, class Case>
class case_handler {
  void* callable_;
  R (*call_)(void*, const Case&);

 public:
  template <class F>
    requires std::invocable<F&, const Case&> || std::invocable<F&>
  constexpr case_handler(F&& f)
      : callable_(const_cast<void*>(static_cast<const void*>(&f))),
        call_([](void* callable, const Case& c) -> R {
          auto& g = *static_cast<std::remove_reference_t<F>*>(callable);
          if constexpr (std::invocable<F&, const Case&>)
            return g(c);
          else
            return g();
        }) {}

  constexpr R operator()(const Case& c) const { return call_(callable_, c); }
};

// The handlers of a match: one member per case, named after it, sorted by name.
template <class R, class... Cases>
struct handlers_of {
  struct type;
  consteval {
    struct item {
      std::string_view name;
      std::meta::info type;
    };
    std::vector<item> items{{case_name<Cases>, ^^case_handler<R, Cases>}...};
    for (std::size_t i = 1; i < items.size(); ++i)
      for (std::size_t j = i; j > 0 && items[j].name < items[j - 1].name; --j)
        std::swap(items[j], items[j - 1]);
    std::vector<std::meta::info> specs;
    for (std::size_t i = 0; i < items.size(); ++i) {
      if (i > 0 && items[i].name == items[i - 1].name)
        throw unknown_name("lv: two cases of one outcome are both called '" + str(items[i].name) +
                           "'. Give one of them another name with lv::as.");
      specs.push_back(std::meta::data_member_spec(items[i].type, {.name = str(items[i].name)}));
    }
    std::meta::define_aggregate(^^type, specs);
  }
};

}  // namespace detail

// One of a fixed set of cases. See the top of this section.
template <class... Cases>
class outcome {
  std::size_t index_ = 0;
  std::tuple<Cases...> values_{};

 public:
  constexpr outcome() = default;

  template <class C>
    requires(std::same_as<C, Cases> || ...)
  constexpr outcome(C c) {
    std::size_t i = 0;
    ((std::same_as<C, Cases> ? (index_ = i, std::get<C>(values_) = std::move(c), 0) : 0, ++i), ...);
  }

  // Calls the handler of the case that happened and returns its result.
  template <class R = void>
  constexpr R match(const typename detail::handlers_of<R, Cases...>::type& handlers) const {
    template for (constexpr auto m :
                  std::define_static_array(std::meta::nonstatic_data_members_of(
                      ^^typename detail::handlers_of<R, Cases...>::type,
                      std::meta::access_context::unchecked()))) {
      template for (constexpr auto k : detail::indices<sizeof...(Cases)>) {
        if constexpr (detail::case_name<Cases...[k]> == std::meta::identifier_of(m))
          if (index_ == k) return handlers.[:m:](std::get<k>(values_));
      }
    }
    std::unreachable();
  }

  // Whether the case C is the one that happened.
  template <class C>
    requires(std::same_as<C, Cases> || ...)
  constexpr bool is() const {
    std::size_t i = 0, at = 0;
    ((std::same_as<C, Cases> ? (at = i, 0) : 0, ++i), ...);
    return index_ == at;
  }

  // For an outcome that can be `gone`: true when the widget still exists, so that
  //   while (co_await lv::next<LV_EVENT_CLICKED>(button)) { ... }
  // loops until the button is deleted.
  constexpr explicit operator bool() const
    requires(std::same_as<gone, Cases> || ...)
  {
    return !is<gone>();
  }

  // The same outcome as a wider one that has all of these cases.
  template <class Wider>
  constexpr Wider widen() const {
    template for (constexpr auto k : detail::indices<sizeof...(Cases)>) {
      if (index_ == k) return Wider{std::get<k>(values_)};
    }
    std::unreachable();
  }
};

namespace detail {

// ---- things to wait for ------------------------------------------------------
//
// A source can be armed with a function to call when it is done, disarmed before
// that, and asked for its outcome afterwards. arm() returns false when the source
// is done already and will not call back.

template <class S>
concept wait_source = requires(S s, void (*done)(void*), void* context) {
  typename S::result_type;
  { s.arm(done, context) } -> std::same_as<bool>;
  s.disarm();
  { s.result() } -> std::same_as<typename S::result_type>;
};

// One event on a widget, or the widget's deletion.
template <lv_event_code_t Code>
struct event_source {
  using result_type = outcome<fired<Code>, gone>;

  lv_obj_t* widget;
  void (*done)(void*) = nullptr;
  void* context = nullptr;
  lv_event_dsc_t* on_code = nullptr;
  lv_event_dsc_t* on_gone = nullptr;
  bool happened = false;

  bool arm(void (*done_fn)(void*), void* done_context) noexcept {
    if (widget == nullptr) return false;
    done = done_fn;
    context = done_context;
    on_code = lv_obj_add_event_cb(widget, &fired_cb, Code, this);
    on_gone = lv_obj_add_event_cb(widget, &gone_cb, LV_EVENT_DELETE, this);
    return true;
  }

  void disarm() noexcept {
    if (widget != nullptr && on_code != nullptr) {
      lv_obj_remove_event_dsc(widget, on_code);
      lv_obj_remove_event_dsc(widget, on_gone);
    }
    on_code = on_gone = nullptr;
  }

  result_type result() const noexcept {
    if (happened) return fired<Code>{};
    return gone{};
  }

  static void fired_cb(lv_event_t* e) {
    auto* self = static_cast<event_source*>(lv_event_get_user_data(e));
    self->happened = true;
    self->disarm();
    self->done(self->context);  // may free this source: nothing after this line
  }

  static void gone_cb(lv_event_t* e) {
    auto* self = static_cast<event_source*>(lv_event_get_user_data(e));
    self->disarm();
    self->widget = nullptr;
    self->done(self->context);
  }
};

// A time span, with a one-shot LVGL timer.
struct sleep_source {
  using result_type = outcome<elapsed>;

  std::uint32_t ms;
  void (*done)(void*) = nullptr;
  void* context = nullptr;
  lv_timer_t* timer = nullptr;

  bool arm(void (*done_fn)(void*), void* done_context) noexcept {
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

  result_type result() const noexcept { return elapsed{}; }

  static void elapsed_cb(lv_timer_t* t) {
    auto* self = static_cast<sleep_source*>(lv_timer_get_user_data(t));
    self->timer = nullptr;  // LVGL deletes it when this callback returns
    self->done(self->context);
  }
};

// The next change of a state.
template <class T, class O>
struct change_source {
  using result_type = outcome<changed_to<T>>;

  const state<T, O>* source;
  void (*done)(void*) = nullptr;
  void* context = nullptr;
  lv_observer_t* observer = nullptr;
  bool armed = false;

  bool arm(void (*done_fn)(void*), void* done_context) noexcept {
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

  result_type result() const noexcept { return changed_to<T>{source->get()}; }

  static void changed_cb(lv_observer_t* o, lv_subject_t*) {
    auto* self = static_cast<change_source*>(lv_observer_get_user_data(o));
    if (!self->armed) return;
    self->disarm();
    self->done(self->context);
  }
};

// The cases of an outcome type, as a tuple type.
template <class T>
struct cases_of_impl;
template <class... Cases>
struct cases_of_impl<outcome<Cases...>> {
  using type = std::tuple<Cases...>;
};
template <class T>
using cases_of = typename cases_of_impl<T>::type;

// The outcome of S with its first case renamed: see lv::as.
template <fixed Name, class First, class... Rest>
outcome<named<Name>, Rest...> rename_first(std::type_identity<outcome<First, Rest...>>);

template <fixed Name, wait_source S>
struct renamed_source {
  using result_type =
      decltype(rename_first<Name>(std::type_identity<typename S::result_type>{}));

  S inner;

  bool arm(void (*done_fn)(void*), void* done_context) noexcept {
    return inner.arm(done_fn, done_context);
  }
  void disarm() noexcept { inner.disarm(); }

  result_type result() const noexcept {
    const auto r = inner.result();
    if (r.template is<std::tuple_element_t<0, cases_of<typename S::result_type>>>())
      return named<Name>{};
    return rest_of(r);
  }

 private:
  template <class First, class... Rest>
  static result_type rest_of(const outcome<First, Rest...>& r) {
    result_type out{};
    ((r.template is<Rest>() ? (out = result_type{Rest{}}, 0) : 0), ...);
    return out;
  }
};

// co_await of one source.
template <wait_source S>
struct wait {
  S source;
  std::coroutine_handle<> waiting{};

  bool await_ready() const noexcept { return false; }

  bool await_suspend(std::coroutine_handle<> h) noexcept {
    waiting = h;
    return source.arm(&resume, this);
  }

  // One case with a value gives the value; one empty case gives nothing; several
  // cases give the outcome.
  auto await_resume() const noexcept {
    using result = typename S::result_type;
    if constexpr (std::tuple_size_v<cases_of<result>> != 1) {
      return source.result();
    } else if constexpr (requires(std::tuple_element_t<0, cases_of<result>> c) { c.value; }) {
      return source.result().template match<decltype(std::tuple_element_t<0, cases_of<result>>{}.value)>(
          only_value_handlers<result>());
    } else {
      return;
    }
  }

  static void resume(void* self) {
    const auto h = static_cast<wait*>(self)->waiting;
    h.resume();
  }

 private:
  // The outcome has one case, changed_to<T>: its only handler returns the value.
  template <class Result>
  static auto only_value_handlers() {
    using only = std::tuple_element_t<0, cases_of<Result>>;
    using value = decltype(only{}.value);
    return typename handlers_of<value, only>::type{[](const only& c) { return c.value; }};
  }
};

template <wait_source S>
wait<S> operator co_await(S source) noexcept {
  return {std::move(source)};
}

// The cases of several outcome types together, each once.
consteval std::meta::info merged_outcome(std::initializer_list<std::meta::info> outcomes) {
  std::vector<std::meta::info> cases;
  for (auto o : outcomes)
    for (auto c : std::meta::template_arguments_of(std::meta::dealias(o))) {
      bool known = false;
      for (auto k : cases) known = known || k == c;
      if (!known) cases.push_back(c);
    }
  return std::meta::substitute(^^outcome, cases);
}

// co_await lv::first(...): the first of several sources to be done.
template <wait_source... S>
struct first_of {
  using result_type = typename[:merged_outcome({^^typename S::result_type...}):];
  static constexpr std::size_t none = static_cast<std::size_t>(-1);

  struct slot {
    first_of* self;
    std::size_t index;
  };

  std::tuple<S...> sources;
  std::array<slot, sizeof...(S)> slots{};
  std::coroutine_handle<> waiting{};
  std::size_t winner = none;

  bool await_ready() const noexcept { return false; }

  bool await_suspend(std::coroutine_handle<> h) noexcept {
    waiting = h;
    template for (constexpr auto i : indices<sizeof...(S)>) {
      if (winner == none) {
        slots[i] = {this, i};
        if (!std::get<i>(sources).arm(&done, &slots[i])) {
          winner = i;        // done already: the ones armed before lose
          disarm_others();
        }
      }
    }
    return winner == none;
  }

  result_type await_resume() const noexcept {
    template for (constexpr auto i : indices<sizeof...(S)>) {
      if (winner == i) return std::get<i>(sources).result().template widen<result_type>();
    }
    std::unreachable();
  }

  static void done(void* context) {
    const auto* s = static_cast<slot*>(context);
    first_of* self = s->self;
    if (self->winner != none) return;
    self->winner = s->index;
    self->disarm_others();
    const auto h = self->waiting;
    h.resume();
  }

 private:
  void disarm_others() noexcept {
    template for (constexpr auto i : indices<sizeof...(S)>) {
      if (winner != i) std::get<i>(sources).disarm();
    }
  }
};

template <fixed Name>
struct as_name {};

// source | lv::as<"name">. In this namespace because the sources are, and that is
// where the compiler looks for the operator.
template <wait_source S, fixed Name>
renamed_source<Name, S> operator|(S source, as_name<Name>) noexcept {
  return {std::move(source)};
}

}  // namespace detail

// co_await lv::next<LV_EVENT_CLICKED>(button): the event came, or the widget was
// deleted. The outcome converts to bool: true while the widget exists.
template <lv_event_code_t Code, detail::fixed W>
detail::event_source<Code> next(view<W> widget) noexcept {
  return {widget.raw()};
}

// co_await lv::sleep(300 * lv::ms)
inline detail::sleep_source sleep(duration time) noexcept { return {time.ms}; }

// T now = co_await lv::changed(speed): the value after the state's next change.
template <class T, class O>
detail::change_source<T, O> changed(const state<T, O>& source) noexcept {
  return {&source};
}

// Names what a source means: lv::next<LV_EVENT_CLICKED>(ok) | lv::as<"accepted">.
// Its first case (the event, the time, the change) gets that name in match().
template <detail::fixed Name>
inline constexpr detail::as_name<Name> as{};

// co_await lv::first(a, b, c): whichever is done first. The others are cancelled.
template <detail::wait_source... S>
detail::first_of<S...> first(S... sources) noexcept {
  return {{std::move(sources)...}};
}

}  // namespace lv
