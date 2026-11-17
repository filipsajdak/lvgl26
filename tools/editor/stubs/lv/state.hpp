#pragma once

// Editor stub of lv/state.hpp: typed application state that widgets follow.
//
//   constinit lv::state speed{0 * km / h};
//   lv::arc({.value = lv::bind(speed)})
//   speed = 120 * km / h;

#include <lv/values.hpp>
#include <lv/view.hpp>

#include <concepts>
#include <type_traits>

namespace lv {

/// What a state can hold: a bool, an enum, an integer quantity, a named value, an lv_color_t
/// or a pointer. Where a part takes a value that it keeps in a state. (The editor accepts any
/// class type here; the compiler checks the members.)
template <class T>
concept state_value = std::same_as<T, bool> || std::is_enum_v<T> || std::is_integral_v<T> || std::is_class_v<T> ||
                      std::is_pointer_v<T>;

/// A value that widgets can follow. It holds a quantity (`0 * km / h`, with mp-units),
/// an lv::counter, a struct with one member, a bool, an enum, a color or a pointer.
/// O is the settings tag; leave it out.
template <class T, class O = override>
class state {
  T value_{};

 public:
  constexpr state() = default;
  constexpr explicit state(T initial) : value_(initial) {}
  state(const state&) = delete;
  state& operator=(const state&) = delete;
  ~state() {}

  /// The current value.
  T get() const { __builtin_unreachable(); }
  /// Changes the value and updates everything bound to it.
  void set(T v) {}
  /// The same as set().
  state& operator=(T v) { __builtin_unreachable(); }

  state& operator++()
    requires requires(T v) { ++v; } { __builtin_unreachable(); }
  state& operator--()
    requires requires(T v) { --v; } { __builtin_unreachable(); }
  template <class U>
    requires requires(T v, const U& u) { v += u; }
  state& operator+=(const U& u) { __builtin_unreachable(); }
  template <class U>
    requires requires(T v, const U& u) { v -= u; }
  state& operator-=(const U& u) { __builtin_unreachable(); }

  /// The plain number: of a quantity, in the state's own unit; of a named value, its member.
  stub::anything number() const { __builtin_unreachable(); }

  /// Changes the value with a function of the current one:
  ///   clicks.update([](lv::counter c) { return ++c; });
  template <class F>
    requires std::invocable<F&, T> && std::convertible_to<std::invoke_result_t<F&, T>, T>
  void update(F) {}

  /// The LVGL subject, created on first use: C code can share it.
  lv_subject_t* raw() const { __builtin_unreachable(); }

  /// Calls `f(value)` now and after every change. Returns LVGL's observer.
  template <class F>
    requires std::invocable<F&, T>
  lv_observer_t* observe(F) const { return nullptr; }

  /// Deletes the subject. Widgets bound to it stop following.
  void reset() {}
};

template <class T>
state(T) -> state<T>;

/// Binds a property to a state: .value = lv::bind(speed). The widget follows, and
/// writes back where LVGL has a bind function.
template <class T, class O>
constexpr binding<T> bind(const state<T, O>&) {
  return {};
}

/// Binds a text property to a state through a format: lv::bind(speed, "{} km/h").
template <class T, class O>
constexpr formatted_binding<T> bind(const state<T, O>&, text_format) {
  return {};
}

}  // namespace lv
