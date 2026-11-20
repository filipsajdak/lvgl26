#pragma once

// Typed application state over LVGL's observer.
//
//   #include <mp-units/systems/si.h>
//   using namespace mp_units::si::unit_symbols;
//
//   constinit lv::state speed{0 * km / h};        // a constant-initialized global
//
//   lv::arc({.value = lv::bind(speed)})           // the arc follows speed, and writes back
//   lv::label({.text = lv::bind(speed)})          // shows "0 km/h"
//
//   speed = 120 * km / h;                         // every bound widget updates
//   speed = 2 * km / min;                         // converted, by mp-units' rules
//
// Measures are mp-units quantities: lvgl26 has no unit system of its own for them.
// A count is an lv::counter. A state can also hold any struct with one member
// (an index, an id), a bool, an enum, a color or a pointer.
//
// A state keeps its own list of watchers: the widgets bound to it, every lv::computed and
// lv::effect that depends on it, and lv::changed. Following a state costs no LVGL subject
// and no LVGL observer.
//
// raw() gives the state's lv_subject_t, created the first time it is asked for. From then
// on the two stay one value: what lvgl26 sets reaches the subject's C observers, and what C
// code writes with lv_subject_set_*() reaches every watcher. C observers are told after the
// watchers of lvgl26, whatever the order in which they were added. observe() is a C
// observer, so it makes the subject too.
//
// A state tells its watchers when its value changes: setting the value it has tells nobody,
// except a pointer, which cannot be compared. detail/watch.hpp says what a walk over the
// watchers promises, for a watcher that changes the state or its neighbours.
// Like all of LVGL, a state must only be touched from the UI thread.

#include <lv/detail/settings.hpp>
#include <lv/detail/source.hpp>
#include <lv/detail/watch.hpp>
#include <lv/values.hpp>
#include <lv/view.hpp>

#include <concepts>
#include <cstdint>
#include <cstring>
#include <functional>
#include <type_traits>

namespace lv {

// What a state can hold: a bool, an enum, an integer quantity, a named value (a struct of
// one integer, bool or enum member), an lv_color_t or a pointer. Where a part takes a value
// that it keeps in a state.
template <class T>
concept state_value = detail::plain_state_value<T> || detail::strong_value<T> || detail::integer_quantity<T> ||
                      std::same_as<T, lv_color_t> || std::is_pointer_v<T>;

namespace detail {

// Which kind of subject carries a state's value.

template <class T>
constexpr lv_subject_type_t subject_type() {
  if constexpr (std::same_as<T, lv_color_t>)
    return LV_SUBJECT_TYPE_COLOR;
  else if constexpr (std::is_pointer_v<T>)
    return LV_SUBJECT_TYPE_POINTER;
  else
    return LV_SUBJECT_TYPE_INT;
}

template <class T>
void write_subject(lv_subject_t* s, T v) {
  if constexpr (std::same_as<T, lv_color_t>)
    lv_subject_set_color(s, v);
  else if constexpr (std::is_pointer_v<T>)
    lv_subject_set_pointer(s, const_cast<void*>(static_cast<const void*>(v)));
  else if constexpr (integer_quantity<T>)
    lv_subject_set_int(s, static_cast<std::int32_t>(v.numerical_value_in(T::unit)));
  else if constexpr (strong_value<T>)
    lv_subject_set_int(s, static_cast<std::int32_t>(v.[:strong_member<T>():]));
  else
    lv_subject_set_int(s, static_cast<std::int32_t>(v));
}

// The value of a state of type T made from a number: what an LVGL integer subject holds.
template <class T>
T value_of_number(std::int32_t n) {
  if constexpr (integer_quantity<T>) {
    return T{static_cast<typename T::rep>(n), T::reference};
  } else if constexpr (strong_value<T>) {
    return T{static_cast<typename state_payload<T>::type>(n)};
  } else if constexpr (std::same_as<T, bool>) {
    return n != 0;
  } else {
    return static_cast<T>(n);
  }
}

// The number a state carries in an integer subject: of a quantity, in its own unit; of a
// named value, its member.
template <class T>
std::int32_t number_of(const T& v) {
  if constexpr (integer_quantity<T>)
    return static_cast<std::int32_t>(v.numerical_value_in(T::unit));
  else if constexpr (strong_value<T>)
    return static_cast<std::int32_t>(v.[:strong_member<T>():]);
  else
    return static_cast<std::int32_t>(v);
}

template <class T>
T read_subject(lv_subject_t* s) {
  if constexpr (std::same_as<T, lv_color_t>)
    return lv_subject_get_color(s);
  else if constexpr (std::is_pointer_v<T>)
    return static_cast<T>(lv_subject_get_pointer(s));
  else
    return value_of_number<T>(lv_subject_get_int(s));
}

// The value in the form a subject holds it.
template <class T>
subject_value to_subject_value(const T& v) {
  if constexpr (std::same_as<T, lv_color_t>)
    return {.number = 0, .color = v, .pointer = nullptr};
  else if constexpr (std::is_pointer_v<T>)
    return {.number = 0, .color = {}, .pointer = static_cast<const void*>(v)};
  else
    return {.number = number_of(v), .color = {}, .pointer = nullptr};
}

// Whether setting `now` over `before` changes nothing: LVGL's rule for its subjects. A
// pointer always counts as a change.
template <class T>
bool same_value(const T& before, const T& now) {
  if constexpr (std::same_as<T, lv_color_t>)
    return lv_color_eq(before, now);
  else if constexpr (std::is_pointer_v<T>)
    return false;
  else
    return number_of(before) == number_of(now);
}

// What a state and a computed share: the current value, the list of watchers, and the LVGL
// subject that raw() makes on demand.
//
// Without a subject `value_` is the value. With one the subject is: C code may write it, and
// the observer `forward` (the first on the subject) tells the watchers about every change,
// whoever made it.
template <class T>
class watched {
 protected:
  mutable T value_{};
  mutable lv_subject_t* subject_ = nullptr;
  mutable watch_list watchers_;

  constexpr watched() = default;
  constexpr explicit watched(T initial) : value_(initial) {}
  watched(const watched&) = delete;
  watched& operator=(const watched&) = delete;

  // The value now.
  T current() const { return subject_ ? read_subject<T>(subject_) : value_; }

  // Makes `v` the value and tells the watchers if it is a change.
  void publish(T v) const {
    const bool same = same_value(value_, v);
    value_ = v;
    if (subject_)
      write_subject(subject_, v);  // `forward` tells the watchers if the subject changed
    else if (!same)
      watchers_.tell();
  }

  lv_subject_t* subject() const {
    if (subject_ == nullptr) subject_ = make_subject();
    return subject_;
  }

  // Ends the walks and forgets every watcher, then deletes the subject. What C code wrote
  // last stays the value.
  void release() const {
    watchers_.detach_all();
    if (subject_ != nullptr && lv_is_initialized()) {
      value_ = read_subject<T>(subject_);
      lv_subject_delete(subject_);
    }
    subject_ = nullptr;
  }

 private:
  lv_subject_t* make_subject() const {
    lv_subject_t* s = lv_subject_create(subject_type<T>());
    if (s == nullptr) return nullptr;
    write_subject(s, value_);
    // subject_ is still null here, so forward() ignores LVGL's first call.
    if (lv_subject_add_observer(s, &forward, const_cast<watched*>(this)) == nullptr) {
      lv_subject_delete(s);
      return nullptr;
    }
    return s;
  }

  static void forward(lv_observer_t* observer, lv_subject_t*) {
    const auto* self = static_cast<const watched*>(lv_observer_get_user_data(observer));
    if (self->subject_ != nullptr) self->watchers_.tell();
  }
};

// For lv::computed, lv::effect and lv::changed (lv/computed.hpp, lv/task.hpp): the watchers
// of a state or a computed.
struct graph_access {
  template <class D>
  static watch_list& watchers(const D& d) {
    return d.follow();
  }
};

// A source a widget can write a number to: a state of a number, not of a color or a pointer.
template <class S>
inline constexpr bool writable_source =
    requires(S& s, typename S::value_type v) { s.set(v); } &&
    !std::same_as<typename S::value_type, lv_color_t> && !std::is_pointer_v<typename S::value_type>;

// The functions of source_ops for S, a state or a computed.
template <class S>
struct source_functions {
  static lv_subject_t* subject(const void* source) { return static_cast<const S*>(source)->raw(); }
  static watch_list& watchers(const void* source) {
    return graph_access::watchers(*static_cast<const S*>(source));
  }
  static subject_value read(const void* source) {
    return to_subject_value(static_cast<const S*>(source)->current_value());
  }
  static std::int32_t number(const void* source) {
    return to_subject_value(static_cast<const S*>(source)->current_value()).number;
  }
  static void write(const void* source, std::int32_t number) {
    const_cast<S*>(static_cast<const S*>(source))->set(value_of_number<typename S::value_type>(number));
  }
  static consteval auto write_function() {
    if constexpr (writable_source<S>)
      return &write;
    else
      return static_cast<void (*)(const void*, std::int32_t)>(nullptr);
  }
};

// The table of a state or a computed. A binding holds this function, not the table.
template <class S>
const source_ops& source_ops_of() {
  using fn = source_functions<S>;
  static constexpr source_ops ops{.subject = &fn::subject,
                                  .watchers = &fn::watchers,
                                  .read = &fn::read,
                                  .number = &fn::number,
                                  .write = fn::write_function()};
  return ops;
}

}  // namespace detail

// A value that widgets can follow. See the top of this file.
// O is the settings tag; leave it out.
namespace detail {

consteval std::string_view bad_state_type_text(std::meta::info type) {
  return std::define_static_string(
      "lv: a state holds a quantity (`0 * km / h`, with mp-units), a struct with one integer, bool or enum member, a "
      "bool, an enum, an lv_color_t or a pointer (for text: const char*). This state's type is " +
      str(std::meta::display_string_of(type)) + ".");
}

template <class T>
consteval std::string_view bad_state_type() {
  return bad_state_type_text(std::meta::dealias(^^T));
}

}  // namespace detail

template <class T, class O = override>
class state : detail::watched<T> {
  static_assert(!detail::unit_quantity<T> || detail::integer_quantity<T>,
                "lv: a quantity in a state needs an integer number for now, for example "
                "`0 * km / h` rather than `0.0 * km / h`.");
  static_assert(state_value<T>, detail::bad_state_type<T>());
  static_assert(!detail::integer<T> || detail::project<O>.rules.raw_integer_state,
                "lv: a state of a plain integer does not say what the number is. If it is a "
                "measure, give it a unit with mp-units: `lv::state speed{0 * km / h};`. If it "
                "counts things, use lv::counter: `lv::state clicks{lv::counter{}};`. For "
                "anything else, name it: `struct tab_index { int value; };`. To allow plain "
                "integers, set rules.raw_integer_state in the settings.");

  using base = detail::watched<T>;
  friend detail::graph_access;
  friend detail::source_functions<state>;

 public:
  using value_type = T;

  constexpr state() = default;
  constexpr explicit state(T initial) : base(initial) {}
  state(const state&) = delete;
  state& operator=(const state&) = delete;
  ~state() { reset(); }

  // The current value. Reads the subject, so a change made by C code is seen.
  T get() const { return base::current(); }

  // Changes the value and updates everything bound to it.
  void set(T v) { base::publish(v); }

  state& operator=(T v) {
    set(v);
    return *this;
  }

  // A state forwards the operators its value has:
  //   ++clicks;   speed += 10 * km / h;
  state& operator++()
    requires requires(T v) { ++v; }
  {
    T v = get();
    ++v;
    set(v);
    return *this;
  }
  state& operator--()
    requires requires(T v) { --v; }
  {
    T v = get();
    --v;
    set(v);
    return *this;
  }
  template <class U>
    requires requires(T v, const U& u) { v += u; }
  state& operator+=(const U& u) {
    T v = get();
    v += u;
    set(v);
    return *this;
  }
  template <class U>
    requires requires(T v, const U& u) { v -= u; }
  state& operator-=(const U& u) {
    T v = get();
    v -= u;
    set(v);
    return *this;
  }

  // The plain number: of a quantity, in the state's own unit; of a named value,
  // its member.
  auto number() const
    requires detail::integer_quantity<T> || detail::strong_value<T>
  {
    const T v = get();
    if constexpr (detail::integer_quantity<T>)
      return v.numerical_value_in(T::unit);
    else
      return v.[:detail::strong_member<T>():];
  }

  // Changes the value with a function of the current one:
  //   clicks.update([](lv::counter c) { return ++c; });
  template <class F>
    requires std::invocable<F&, T> && std::convertible_to<std::invoke_result_t<F&, T>, T>
  void update(F f) {
    set(f(get()));
  }

  // The LVGL subject, created on first use. Needs lv_init() to have run. From then on
  // it and the state are one value, in both directions. Null only if LVGL has no memory left.
  lv_subject_t* raw() const { return base::subject(); }

  // Calls `f(value)` now and after every change. The handler may capture nothing,
  // or one pointer-sized trivially copyable value. Returns LVGL's observer, which
  // lv_observer_delete() removes.
  template <class F>
    requires std::invocable<F&, T>
  lv_observer_t* observe(F f) const {
    lv_subject_t* s = raw();
    if (s == nullptr) return nullptr;
    if constexpr (detail::stateless<F>) {
      return lv_subject_add_observer(s, &call_stateless<F>, nullptr);
    } else if constexpr (detail::pointer_sized<F>) {
      void* stored = nullptr;
      std::memcpy(&stored, &f, sizeof(F));
      return lv_subject_add_observer(s, &call_stored<F>, stored);
    } else {
      static_assert(false,
                    "lv: an observer may capture nothing, or one pointer-sized trivially "
                    "copyable value. Capture a pointer to your state instead.");
    }
  }

  // Stops everything that follows this state and deletes its subject. Widgets bound to
  // it stop following, and so does every lv::computed and lv::effect that depends on it
  // (see lv/computed.hpp). The value stays. Called by the destructor.
  void reset() { base::release(); }

 private:
  detail::watch_list& follow() const { return this->watchers_; }
  T current_value() const { return get(); }

  template <class F>
  static void call_stateless(lv_observer_t*, lv_subject_t* subject) {
    F h{};
    h(detail::read_subject<T>(subject));
  }

  template <class F>
  static void call_stored(lv_observer_t* observer, lv_subject_t* subject) {
    void* kept = lv_observer_get_user_data(observer);
    alignas(F) unsigned char buf[sizeof(F)];
    std::memcpy(buf, &kept, sizeof(F));
    (*reinterpret_cast<F*>(buf))(detail::read_subject<T>(subject));
  }
};

// Binds a property to a state: .value = lv::bind(speed).
template <class T, class O>
constexpr binding<T> bind(const state<T, O>& s) {
  return {&s, &detail::source_ops_of<state<T, O>>};
}

// Binds a text property to a state through a format: lv::bind(speed, "{} km/h").
template <class T, class O>
constexpr formatted_binding<T> bind(const state<T, O>& s, text_format format) {
  return {&s, &detail::source_ops_of<state<T, O>>, format.printf};
}

}  // namespace lv
