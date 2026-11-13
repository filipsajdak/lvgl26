#pragma once

// The value types that property structs are made of: strict<T>, handler and arg<...>,
// and the rule that gives each LVGL setter parameter its C++ type.

#include <lv/config.hpp>
#include <lv/detail/api.hpp>
#include <lv/detail/fixed_string.hpp>
#include <lv/event.hpp>
#include <lv/units.hpp>

#include <array>
#include <concepts>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace lv {

template <class T, class O>
class state;
template <class T>
class token;
template <class T>
struct raw_value;
template <class T>
class styled;

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

// How to write a value of type `t`, for the error message.
consteval std::string how_to_write(std::meta::info t) {
  if (t == ^^length) return "a length (120 * lv::px, 50 * lv::pct or lv::size_content)";
  if (t == std::meta::dealias(^^horizontal))
    return "a width or x (120 * lv::px, 50 * lv::pct or lv::size_content, not a vertical length)";
  if (t == std::meta::dealias(^^vertical))
    return "a height or y (120 * lv::px, 50 * lv::pct or lv::size_content, not a horizontal length)";
  if (t == ^^angle) return "an angle (90 * lv::deg)";
  if (t == ^^duration) return "a time (300 * lv::ms)";
  if (t == ^^opa) return "an opacity (50 * lv::pct)";
  if (t == ^^strict<bool>) return "true or false";
  if (std::meta::has_template_arguments(t) && std::meta::template_of(t) == ^^strict)
    return "a whole number (not bool, not floating point)";
  if (std::meta::has_template_arguments(t) && std::meta::template_of(t) == ^^styled)
    return "a design token (for example tok.space.md) or lv::raw(...), holding " +
           how_to_write(std::meta::template_arguments_of(t)[0]);
  return str(std::meta::display_string_of(t));
}

template <class... Ts>
struct wrong_value {
  template <class... U>
  static consteval std::string_view message() {
    std::string m = "lv: wrong value for this property. It takes ";
    std::string sep;
    ((m += sep + how_to_write(^^Ts), sep = ", then "), ...);
    m += ". You gave ";
    sep.clear();
    ((m += sep + str(std::meta::display_string_of(std::meta::dealias(^^std::remove_cvref_t<U>))), sep = ", "),
     ...);
    if (sizeof...(U) == 0) m += "nothing";
    return std::define_static_string(m + ".");
  }
};

}  // namespace detail

namespace detail {

template <class T>
concept plain_state_value = integer<T> || std::same_as<T, bool> || std::is_enum_v<T>;

// The single data member of a class that wraps one plain value, or a null reflection.
template <class T>
consteval std::meta::info strong_member() {
  if (!std::meta::is_class_type(^^T)) return {};
  const auto members =
      std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked());
  if (members.size() != 1 || !std::meta::is_public(members[0])) return {};
  const auto t = std::meta::remove_cv(std::meta::dealias(std::meta::type_of(members[0])));
  // Parentheses: `^^char && x` would be read as a reflection of the type char&&.
  const bool character = t == (^^char) || t == (^^wchar_t) || t == (^^char8_t) ||
                         t == (^^char16_t) || t == (^^char32_t);
  const bool plain = (std::meta::is_integral_type(t) || std::meta::is_enum_type(t)) && !character;
  return plain ? members[0] : std::meta::info{};
}

// A named value: a struct with exactly one public integer, bool or enum member,
// such as `struct kmh { int value; };`. lv::angle and lv::duration are of this kind.
template <class T>
concept strong_value = strong_member<T>() != std::meta::info{};

// What a widget receives from a state of type T: T itself, or the member of a named value.
template <class T>
struct state_payload {
  using type = T;
};
template <strong_value T>
struct state_payload<T> {
  using type = typename[:std::meta::type_of(strong_member<T>()):];
};

}  // namespace detail

// A property bound to a state: lv::bind(speed). See lv/state.hpp.
template <class T>
struct binding {
  const void* source;
  lv_subject_t* (*subject)(const void*);
};

// The format of a text binding: text with exactly one {} where the value goes,
// as in std::format. It is checked and turned into LVGL's printf form at compile time.
struct text_format {
  const char* printf;

  template <std::size_t N>
  consteval text_format(const char (&text)[N]) : printf(nullptr) {
    std::string out;
    int places = 0;
    for (std::size_t i = 0; i + 1 < N; ++i) {
      if (text[i] == '{' && text[i + 1] == '}') {
        out += "%d";
        ++places;
        ++i;
      } else if (text[i] == '%') {
        out += "%%";
      } else {
        out.push_back(text[i]);
      }
    }
    if (places != 1)
      throw detail::unknown_name("lv: a text binding format needs exactly one {} for the value, "
                                 "for example \"{} km/h\".");
    printf = std::define_static_string(out);
  }
};

// A text property bound to a state through a format: lv::bind(speed, "{} km/h").
template <class T>
struct formatted_binding {
  const void* source;
  lv_subject_t* (*subject)(const void*);
  const char* format;
};

namespace detail {

template <class T>
inline constexpr bool is_binding = false;
template <class T>
inline constexpr bool is_binding<binding<T>> = true;
template <class T>
inline constexpr bool is_binding<formatted_binding<T>> = true;

}  // namespace detail

// One member of a props struct: unset, the arguments of one setter, or a binding.
template <class... Ts>
struct arg {
  bool set = false;
  std::tuple<Ts...> v{};

  // Set when the property follows a state instead of holding a value.
  const void* bound = nullptr;
  lv_subject_t* (*bound_subject)(const void*) = nullptr;
  const char* bound_format = nullptr;

  constexpr arg() = default;

  // Accepts anything, so that a wrong value gets the message below instead of
  // the compiler's "could not convert".
  template <class... U>
    requires(sizeof...(U) > 0) &&
            (!(sizeof...(U) == 1 && ((std::same_as<std::remove_cvref_t<U>, arg> ||
                                      detail::is_binding<std::remove_cvref_t<U>>) && ...)))
  constexpr arg(U&&... u) : set(true), v(convert(std::forward<U>(u)...)) {}

  template <class T>
  constexpr arg(binding<T> b) : set(true), bound(b.source), bound_subject(b.subject) {
    using first = std::tuple_element_t<0, std::tuple<Ts...>>;
    static_assert(std::convertible_to<T, first> ||
                      std::convertible_to<typename detail::state_payload<T>::type, first>,
                  "lv: this property cannot be bound to a state of that type. The state's "
                  "type, or the value inside its named type, must be what the property takes.");
  }

  template <class T>
  constexpr arg(formatted_binding<T> b)
      : set(true), bound(b.source), bound_subject(b.subject), bound_format(b.format) {
    static_assert(sizeof...(Ts) == 1 && (std::same_as<Ts, const char*> && ...),
                  "lv: lv::bind(state, format) is for text properties only.");
  }

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

// Names that cannot be used for a generated struct member.
consteval bool is_keyword(std::string_view s) {
  return is_one_of(s, {"auto",     "bool",   "break",  "case",     "char",      "class",  "const",
                       "continue", "default", "delete", "do",       "double",    "else",   "enum",
                       "export",   "false",  "float",  "for",      "if",        "inline", "int",
                       "long",     "namespace", "new", "operator", "private",   "public", "return",
                       "short",    "signed", "static", "struct",   "switch",    "template", "this",
                       "true",     "union",  "unsigned", "using",  "virtual",   "void",   "while"});
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
    if (is_one_of(name, {"x", "w", "width"})) return ^^horizontal;
    if (is_one_of(name, {"y", "h", "height"})) return ^^vertical;
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

// ---- design tokens and styles ----------------------------------------------

namespace detail {
struct from_design_tokens {};
}  // namespace detail

// A value that was read from a design token file. It converts to the plain value.
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

// A style value that is deliberately not a design token. See lv::raw.
template <class T>
struct raw_value {
  T value;
};

// Marks a style value as a deliberate exception to "styles use design tokens":
// lv::raw(1 * lv::px). Easy to search for in a review.
template <class T>
constexpr raw_value<T> raw(T value) {
  return {value};
}

namespace detail {

template <class T>
inline constexpr bool is_token_or_raw = false;
template <class T>
inline constexpr bool is_token_or_raw<token<T>> = true;
template <class T>
inline constexpr bool is_token_or_raw<raw_value<T>> = true;

}  // namespace detail

// The type of a design value in a style: a color, length, opacity, time or number.
// It only accepts a design token or lv::raw(...), so that styles cannot drift away
// from the design system. Set lv::config::raw_style_values to accept plain values.
template <class T>
class styled {
  T v_{};

 public:
  constexpr styled() = default;

  template <class U>
    requires std::convertible_to<const U&, T>
  constexpr styled(const token<U>& t) : v_(static_cast<T>(*t)) {}

  // A number token from 0 to 1 is an opacity.
  constexpr styled(const token<double>& t)
    requires std::same_as<T, opa>
      : v_(percent{static_cast<std::int32_t>(*t * 100 + 0.5)}) {}

  // A number token that is a whole number.
  template <std::same_as<double> N>
    requires requires { typename std::type_identity_t<decltype(T{}.raw())>; } &&
             detail::integer<decltype(T{}.raw())> && (!std::convertible_to<const N&, T>)
  constexpr styled(const token<N>& t) : v_(static_cast<decltype(T{}.raw())>(*t)) {
    contract_assert(static_cast<double>(static_cast<decltype(T{}.raw())>(*t)) == *t);
  }

  template <class U>
    requires std::convertible_to<const U&, T>
  constexpr styled(const raw_value<U>& r) : v_(static_cast<T>(r.value)) {}

  // A plain value, when the project allows it: see lv::config::raw_style_values.
  // O only delays reading the setting until this constructor is considered.
  template <class U, class O = override>
    requires(cfg<O>.raw_style_values) && (!detail::is_token_or_raw<std::remove_cvref_t<U>>) &&
            (!std::same_as<std::remove_cvref_t<U>, styled>) && std::convertible_to<U, T>
  constexpr styled(U&& u) : v_(static_cast<T>(std::forward<U>(u))) {}

  constexpr decltype(auto) raw() const { return detail::lower(v_); }
};

}  // namespace lv
