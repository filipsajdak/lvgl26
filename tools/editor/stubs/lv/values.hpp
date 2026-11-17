#pragma once

// Editor stub of lv/values.hpp: the types that the members of a props struct and of a
// style have. They accept what the real ones accept, and hold nothing.

#include <lv/config.hpp>
#include <lv/detail/fixed_string.hpp>
#include <lv/event.hpp>
#include <lv/token_types.hpp>
#include <lv/units.hpp>

#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <string_view>
#include <type_traits>
#include <utility>

namespace lv {

template <class T, class O>
class state;

namespace detail {

template <class T>
concept integer = std::integral<T> && !std::same_as<T, bool> && !std::same_as<T, char> &&
                  !std::same_as<T, wchar_t> && !std::same_as<T, char8_t> &&
                  !std::same_as<T, char16_t> && !std::same_as<T, char32_t>;

constexpr bool is_one_of(std::string_view s, std::initializer_list<std::string_view> names) {
  for (auto n : names)
    if (s == n) return true;
  return false;
}

}  // namespace detail

/// An integer or bool that only accepts its own kind: `.value = true` and
/// `.value = 1.5` do not compile.
template <class T>
class strict {
  T v_{};

 public:
  constexpr strict() = default;

  template <detail::integer U>
    requires detail::integer<T>
  constexpr strict(U u) : v_(static_cast<T>(u)) {}

  template <std::same_as<bool> U>
    requires std::same_as<T, bool>
  constexpr strict(U u) : v_(u) {}

  constexpr T raw() const { return v_; }
};

/// A unit value for a setter that takes it unsigned, such as a spinner's sweep:
/// `.arc_sweep = 270 * lv::deg`. A negative value does not compile.
template <class U>
class nonnegative {
  U v_{};

 public:
  constexpr nonnegative() = default;
  constexpr nonnegative(U u) : v_(u) {}
  constexpr U get() const { return v_; }
};

/// An event handler in a props struct: a function, or a lambda without captures.
/// It takes nothing, or an lv::event.
struct handler {
  constexpr handler() = default;

  template <class F>
    requires std::convertible_to<F, void (*)()>
  constexpr handler(F) {}

  template <class F>
    requires(!std::convertible_to<F, void (*)()>) && std::convertible_to<F, void (*)(event)>
  constexpr handler(F) {}
};

/// A property bound to a state: lv::bind(speed).
template <class T>
struct binding {};

/// The format of a text binding: text with exactly one {} where the value goes.
struct text_format {
  template <std::size_t N>
  consteval text_format(const char (&)[N]) {}
};

/// A font property bound to an lv::scaled_font: lv::bind(title).
struct font_binding {};

/// A text property bound to a state through a format: lv::bind(speed, "{} km/h").
template <class T>
struct formatted_binding {};

namespace detail {

template <class T>
inline constexpr bool is_binding = false;
template <class T>
inline constexpr bool is_binding<binding<T>> = true;
template <class T>
inline constexpr bool is_binding<formatted_binding<T>> = true;
template <>
inline constexpr bool is_binding<font_binding> = true;

struct from_design_tokens {};

}  // namespace detail

/// A value that was read from a design token file. It converts to the plain value.
template <class T>
class token {
  T v_{};

 public:
  constexpr token() = default;
  constexpr token(T v, detail::from_design_tokens) : v_(v) {}
  constexpr operator const T&() const { return v_; }
  constexpr const T& operator*() const { return v_; }
  constexpr const T* operator->() const { return &v_; }
};

/// A style value that is deliberately not a design token. See lv::raw.
template <class T>
struct raw_value {
  T value;
};

/// Marks a style value as a deliberate exception to "styles use design tokens":
/// lv::raw(1 * lv::px). Easy to search for in a review.
template <class T>
constexpr raw_value<T> raw(T value) {
  return {value};
}

/// A design value in a style: a color, length, opacity, time, number or font.
/// It takes a design token (tok.color.card) or lv::raw(...), not a plain value.
template <class T>
class styled {
 public:
  constexpr styled() = default;

  template <class U>
    requires std::convertible_to<const U&, T>
  constexpr styled(const token<U>&) {}

  // A number token: an opacity from 0 to 1, a whole number, or degrees.
  constexpr styled(const token<double>&)
    requires(!std::convertible_to<const double&, T>)
  {}

  template <class U>
    requires std::convertible_to<const U&, T>
  constexpr styled(const raw_value<U>&) {}
};

/// One member of a props struct or of a style: the values of one LVGL setter, or a
/// binding to a state. Unset when it is left out.
template <class... Ts>
struct arg {
  constexpr arg() = default;

  template <class... U>
    requires(sizeof...(U) == sizeof...(Ts)) && (std::convertible_to<U, Ts> && ...) &&
            (!(sizeof...(U) == 1 && ((std::same_as<std::remove_cvref_t<U>, arg> ||
                                      detail::is_binding<std::remove_cvref_t<U>>) && ...)))
  constexpr arg(U&&...) {}

  /// Follows a state: .value = lv::bind(speed).
  template <class T>
  constexpr arg(binding<T>) {}

  /// Follows a font drawn on the device: .text_font = lv::bind(title).
  constexpr arg(font_binding)
    requires(sizeof...(Ts) == 1) && (std::same_as<Ts, styled<const lv_font_t*>> && ...)
  {}

  /// Follows a state as text: .text = lv::bind(speed, "{} km/h").
  template <class T>
  constexpr arg(formatted_binding<T>)
    requires(sizeof...(Ts) == 1) && (std::same_as<Ts, const char*> && ...)
  {}
};

namespace stub {

// Declared and never defined, and not constexpr: a consteval constructor that reaches
// it is not a constant expression, so clang marks the wrong name where it is written.
void this_name_is_not_one_of_the_names_of_the_enum();

/// A member that takes LVGL enumerators by name: `.align = "center"`. Each Names type
/// is written by the stub generator and knows the names of one enum.
template <class... Names>
struct enum_arg {
  constexpr enum_arg() = default;

  template <std::size_t... N>
    requires(sizeof...(N) == sizeof...(Names))
  consteval enum_arg(const char (&... name)[N]) {
    if (!(Names::has(name) && ...)) this_name_is_not_one_of_the_names_of_the_enum();
  }

  // LVGL's own enumerators, for code that still names the C API.
  template <class... U>
    requires(sizeof...(U) == sizeof...(Names)) && (std::convertible_to<U, typename Names::type> && ...) &&
            (!(sizeof...(U) == 1 && (std::same_as<std::remove_cvref_t<U>, enum_arg> && ...)))
  constexpr enum_arg(U&&...) {}

  /// Follows a state of the enum.
  template <class T>
  constexpr enum_arg(binding<T>) {}
};

/// One LVGL enumerator by name, next to values of other kinds in one property. The
/// editor does not check the name here; GCC does.
template <class Names>
struct name_of {
  constexpr name_of() = default;
  template <std::size_t N>
  constexpr name_of(const char (&)[N]) {}
  constexpr name_of(typename Names::type) {}
};

/// The id of a node: `.id = "speedo"`, then root->get<"speedo">().
struct id_name {
  static constexpr std::size_t capacity = 32;
  char text[capacity]{};

  constexpr id_name() = default;
  constexpr id_name(const char* name) : id_name(std::string_view{name}) {}
  /// A name that a function passes on: `.id = id` where `id` is a std::string_view.
  constexpr id_name(std::string_view name) {
    for (std::size_t i = 0; i + 1 < capacity && i < name.size(); ++i) text[i] = name[i];
  }
  constexpr std::string_view sv() const { return std::string_view{text}; }
};

}  // namespace stub

/// The two ends of an animation of one property: `.value = {0, 100}`,
/// `.width = {80 * lv::px, 120 * lv::px}`. See lv::animate.
template <class T>
class from_to {
  bool set_ = false;
  T from_{}, to_{};

 public:
  constexpr from_to() = default;

  template <class U, class V>
    requires std::convertible_to<U, T> && std::convertible_to<V, T>
  constexpr from_to(U&& from, V&& to)
      : set_(true), from_(static_cast<T>(std::forward<U>(from))), to_(static_cast<T>(std::forward<V>(to))) {}

  template <class U>
    requires(!std::same_as<std::remove_cvref_t<U>, from_to>)
  constexpr from_to(U&&) = delete("lv: an animation needs both ends: .property = {from, to}.");

  constexpr bool is_set() const { return set_; }
  constexpr T from() const { return from_; }
  constexpr T to() const { return to_; }
};

}  // namespace lv
