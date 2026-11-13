#pragma once

// The value types that property structs are made of: strict<T>, handler and arg<...>,
// and the rule that gives each LVGL setter parameter its C++ type.

#include <lv/detail/api.hpp>
#include <lv/detail/fixed_string.hpp>
#include <lv/event.hpp>
#include <lv/units.hpp>

#include <array>
#include <concepts>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace lv {

namespace detail {

template <class T>
concept integer = std::integral<T> && !std::same_as<T, bool> && !std::same_as<T, char> &&
                  !std::same_as<T, wchar_t> && !std::same_as<T, char8_t> &&
                  !std::same_as<T, char16_t> && !std::same_as<T, char32_t>;

}  // namespace detail

// An integer or bool that only accepts its own kind, and only values that fit.
// So `.value = true` and `.value = 1.5` do not compile.
template <class T>
class strict {
  T v_{};

 public:
  constexpr strict() = default;

  template <detail::integer U>
    requires detail::integer<T>
  constexpr strict(U u) : v_(static_cast<T>(u)) {
    contract_assert(std::in_range<T>(u));
  }

  template <std::same_as<bool> U>
    requires std::same_as<T, bool>
  constexpr strict(U u) : v_(u) {}

  constexpr T raw() const { return v_; }
};

// An event handler stored in a props struct: a function or a lambda without captures.
// For a handler with state, use view.on<LV_EVENT_...>() instead.
struct handler {
  void (*plain)() = nullptr;
  void (*with_event)(event) = nullptr;

  constexpr handler() = default;

  template <class F>
    requires std::convertible_to<F, void (*)()>
  constexpr handler(F f) : plain(f) {}

  template <class F>
    requires(!std::convertible_to<F, void (*)()>) && std::convertible_to<F, void (*)(event)>
  constexpr handler(F f) : with_event(f) {}
};

namespace detail {

// How to write a value of type T, for the error message.
template <class T>
consteval std::string how_to_write() {
  if (^^T == ^^length) return "a length (120_px, 50_pct or lv::size_content)";
  if (^^T == ^^angle) return "an angle (90_deg)";
  if (^^T == ^^duration) return "a time (300_ms)";
  if (^^T == ^^opa) return "an opacity (50_pct)";
  if (^^T == ^^strict<bool>) return "true or false";
  if (std::meta::has_template_arguments(^^T) && std::meta::template_of(^^T) == ^^strict)
    return "a whole number (not bool, not floating point)";
  return std::string(std::meta::display_string_of(^^T));
}

template <class... Ts>
struct wrong_value {
  template <class... U>
  static consteval std::string_view message() {
    std::string m = "lv: wrong value for this property. It takes ";
    std::string sep;
    ((m += sep + how_to_write<Ts>(), sep = ", then "), ...);
    m += ". You gave ";
    sep.clear();
    ((m += sep + std::string(std::meta::display_string_of(std::meta::dealias(^^std::remove_cvref_t<U>))), sep = ", "),
     ...);
    if (sizeof...(U) == 0) m += "nothing";
    return std::define_static_string(m + ".");
  }
};

}  // namespace detail

// One member of a props struct: unset, or the arguments of one setter.
template <class... Ts>
struct arg {
  bool set = false;
  std::tuple<Ts...> v{};

  constexpr arg() = default;

  // Accepts anything, so that a wrong value gets the message below instead of
  // the compiler's "could not convert".
  template <class... U>
    requires(sizeof...(U) > 0) &&
            (!(sizeof...(U) == 1 && (std::same_as<std::remove_cvref_t<U>, arg> && ...)))
  constexpr arg(U&&... u) : set(true), v(convert(std::forward<U>(u)...)) {}

 private:
  template <class... U>
  static constexpr bool fits = sizeof...(U) == sizeof...(Ts) && requires {
    requires(std::convertible_to<U, Ts> && ...);
  };

  template <class... U>
  static constexpr std::tuple<Ts...> convert(U&&... u) {
    static_assert(fits<U...>, detail::wrong_value<Ts...>::template message<U...>());
    if constexpr (fits<U...>)
      return std::tuple<Ts...>{static_cast<Ts>(std::forward<U>(u))...};
    else
      return {};  // unreachable: keeps the message above the only error
  }
};

namespace detail {

consteval bool has(std::string_view s, std::string_view part) {
  return s.find(part) != std::string_view::npos;
}

consteval bool is_one_of(std::string_view s, std::initializer_list<std::string_view> names) {
  for (auto n : names)
    if (s == n) return true;
  return false;
}

// The C++ type a setter parameter gets in the props struct.
// The unit is inferred from the names LVGL uses, because C only says int32_t.
// Style values follow other rules: see style_value_type in style.hpp.
consteval std::meta::info value_type(std::string_view prop, std::meta::info param) {
  const auto raw = std::meta::type_of(param);
  const auto t = std::meta::remove_cv(std::meta::dealias(raw));
  const std::string_view name =
      std::meta::has_identifier(param) ? std::meta::identifier_of(param) : std::string_view{};

  if (std::meta::is_arithmetic_type(t) && t != ^^bool) {
    if (has(prop, "opa") || has(name, "opa")) return ^^opa;
    if (has(prop, "angle") || has(prop, "rotation") || has(name, "angle") ||
        has(name, "rotation"))
      return ^^angle;
    if (has(prop, "duration") || has(prop, "time") || has(prop, "delay") ||
        has(prop, "period") || has(name, "duration") || has(name, "time") ||
        has(name, "delay") || has(name, "period"))
      return ^^duration;
    if (is_one_of(name, {"x", "y", "w", "h", "width", "height"})) return ^^length;
  }
  if (std::meta::is_integral_type(t)) return std::meta::substitute(^^strict, {t});
  return raw;
}


// Unit types and strict<> hand LVGL their plain value; everything else passes as is.
template <class T>
constexpr decltype(auto) lower(const T& x) {
  if constexpr (requires { x.raw(); })
    return x.raw();
  else
    return (x);
}


template <std::size_t N>
inline constexpr auto indices = [] {
  std::array<std::size_t, N> a{};
  for (std::size_t i = 0; i < N; ++i) a[i] = i;
  return a;
}();


template <class... Ts>
constexpr bool is_set(const arg<Ts...>& a) {
  return a.set;
}

}  // namespace detail

}  // namespace lv
