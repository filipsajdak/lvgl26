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
#include <vector>

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
  constexpr strict(U u)
    pre(std::in_range<T>(u))
      : v_(static_cast<T>(u)) {}

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
consteval bool is_made_from(std::meta::info t, std::meta::info a_template) {
  return std::meta::has_template_arguments(t) && std::meta::template_of(t) == a_template;
}

// How to write a value of one of the unit types, or an empty string for another type.
consteval std::string_view how_to_write_unit(std::meta::info t) {
  struct entry {
    std::meta::info type;
    std::string_view text;
  };
  for (const entry& e :
       {entry{^^length, "a length (120 * lv::px, 50 * lv::pct or lv::size_content)"},
        entry{std::meta::dealias(^^horizontal),
              "a width or x (120 * lv::px, 50 * lv::pct or lv::size_content, not a vertical "
              "length)"},
        entry{std::meta::dealias(^^vertical),
              "a height or y (120 * lv::px, 50 * lv::pct or lv::size_content, not a horizontal "
              "length)"},
        entry{^^angle, "an angle (90 * lv::deg)"},
        entry{^^duration, "a time (300 * lv::ms)"},
        entry{^^opa, "an opacity (50 * lv::pct)"},
        entry{^^strict<bool>, "true or false"}})
    if (e.type == t) return e.text;
  return {};
}

consteval std::string how_to_write(std::meta::info t) {
  if (const auto unit = how_to_write_unit(t); !unit.empty()) return str(unit);
  if (is_made_from(t, ^^strict)) return "a whole number (not bool, not floating point)";
  if (is_made_from(t, ^^styled))
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

// Whether `t` is an integer, bool or enum type, and not a character type.
consteval bool is_plain_value_type(std::meta::info t) {
  // Parentheses: `^^char || x` would otherwise be parsed with the type.
  for (auto character : {(^^char), (^^wchar_t), (^^char8_t), (^^char16_t), (^^char32_t)})
    if (t == character) return false;
  return std::meta::is_integral_type(t) || std::meta::is_enum_type(t);
}

// The single data member of a class that wraps one plain value, or a null reflection.
template <class T>
consteval std::meta::info strong_member() {
  if (!std::meta::is_class_type(^^T)) return {};
  const auto members =
      std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked());
  if (members.size() != 1 || !std::meta::is_public(members[0])) return {};
  const auto t = std::meta::remove_cv(std::meta::dealias(std::meta::type_of(members[0])));
  return is_plain_value_type(t) ? members[0] : std::meta::info{};
}

// A quantity of a units library: a number with a unit, as mp_units::quantity is.
// The check is by shape, so this header does not include mp-units; a project that
// uses quantities includes it itself.
template <class T>
concept unit_quantity = requires(const T& q) {
  typename T::rep;
  T::unit;
  T::reference;
  { q.numerical_value_in(T::unit) } -> std::convertible_to<typename T::rep>;
  T{typename T::rep{}, T::reference};
  unit_symbol(T::unit);
};

// A quantity whose number is an integer: what an LVGL integer subject can carry.
template <class T>
concept integer_quantity = unit_quantity<T> && integer<typename T::rep>;

// A named value: a struct with exactly one public integer, bool or enum member,
// such as `struct tab_index { int value; };`. lv::angle and lv::duration are of this kind.
// A quantity is not one, even though mp-units happens to store it in one member.
template <class T>
concept strong_value = !unit_quantity<T> && strong_member<T>() != std::meta::info{};

// What a widget receives from a state of type T: T itself, or the member of a named value.
template <class T>
struct state_payload {
  using type = T;
};
template <strong_value T>
struct state_payload<T> {
  using type = typename[:std::meta::type_of(strong_member<T>()):];
};
template <integer_quantity T>
struct state_payload<T> {
  using type = typename T::rep;
};

// "%d km/h": how a quantity is shown by a text binding that has no format.
template <class Q>
inline constexpr const char* quantity_format = nullptr;
template <integer_quantity Q>
inline constexpr const char* quantity_format<Q> = [] {
  std::string f = "%d ";
  for (char c : unit_symbol(Q::unit)) {
    f.push_back(c);
    if (c == '%') f.push_back('%');
  }
  return std::define_static_string(f);
}();

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
  constexpr arg(binding<T> b)
      : set(true), bound(b.source), bound_subject(b.subject),
        bound_format(text_of_quantity<T> ? detail::quantity_format<T> : nullptr) {
    using first = std::tuple_element_t<0, std::tuple<Ts...>>;
    static_assert(text_of_quantity<T> || std::convertible_to<T, first> ||
                      std::convertible_to<typename detail::state_payload<T>::type, first>,
                  "lv: this property cannot be bound to a state of that type. The state's "
                  "type, or the number inside its quantity or named type, must be what the "
                  "property takes.");
  }

  template <class T>
  constexpr arg(formatted_binding<T> b)
      : set(true), bound(b.source), bound_subject(b.subject), bound_format(b.format) {
    static_assert(sizeof...(Ts) == 1 && (std::same_as<Ts, const char*> && ...),
                  "lv: lv::bind(state, format) is for text properties only.");
  }

 private:
  // A quantity bound to a text property is shown with its unit: "120 km/h".
  template <class T>
  static constexpr bool text_of_quantity = detail::integer_quantity<T> && sizeof...(Ts) == 1 &&
                                           (std::same_as<Ts, const char*> && ...);

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

// Sorts items that have a `name` by it. An insertion sort: std::sort costs far more
// constexpr steps for the few hundred items this is used on.
template <class Item>
consteval void sort_by_name(std::vector<Item>& items) {
  for (std::size_t i = 1; i < items.size(); ++i)
    for (std::size_t j = i; j > 0 && items[j].name < items[j - 1].name; --j)
      std::swap(items[j], items[j - 1]);
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

// Whether the property or its parameter is named after one of these words.
consteval bool named_after(std::string_view prop, std::string_view parameter,
                           std::initializer_list<std::string_view> words) {
  for (auto w : words)
    if (has(prop, w) || has(parameter, w)) return true;
  return false;
}

// The unit type of a numeric setter parameter, judged by the names LVGL uses, or
// a null reflection if they say nothing.
consteval std::meta::info unit_of(std::string_view prop, std::string_view parameter) {
  if (named_after(prop, parameter, {"opa"})) return ^^opa;
  if (named_after(prop, parameter, {"angle", "rotation"})) return ^^angle;
  if (named_after(prop, parameter, {"duration", "time", "delay", "period"})) return ^^duration;
  if (is_one_of(parameter, {"x", "w", "width"})) return ^^horizontal;
  if (is_one_of(parameter, {"y", "h", "height"})) return ^^vertical;
  return {};
}

// The C++ type a setter parameter gets in the props struct.
// The unit is inferred from the names LVGL uses, because C only says int32_t.
// Style values follow other rules: see style_value_type in style.hpp.
consteval std::meta::info value_type(std::string_view prop, std::meta::info param) {
  const auto raw = std::meta::type_of(param);
  const auto t = std::meta::remove_cv(std::meta::dealias(raw));
  const std::string_view name =
      std::meta::has_identifier(param) ? std::meta::identifier_of(param) : std::string_view{};

  if (std::meta::is_arithmetic_type(t) && t != ^^bool)
    if (const auto unit = unit_of(prop, name); unit != std::meta::info{}) return unit;
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
