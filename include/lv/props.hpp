#pragma once

// props<"arc"> is a struct that reflection generates from LVGL's C API: one member
// per lv_arc_set_* and lv_obj_set_* function, and one per event (on_clicked, ...).
//
//   lv::props<"arc"> p{.id = "speedo", .range = {0, 240}, .rotation = 90_deg, .value = 120};
//
// Members are in alphabetical order, which is the order designated initializers
// must follow. Every member is typed: a wrong name, a wrong unit, a bool where an
// integer is expected and a narrowing conversion are all compile errors.

#include <lv/detail/api.hpp>
#include <lv/detail/fixed_string.hpp>
#include <lv/event.hpp>
#include <lv/units.hpp>

#include <array>
#include <concepts>
#include <cstdarg>
#include <span>
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

consteval bool is_keyword(std::string_view s) {
  return is_one_of(s, {"class", "default", "delete", "enum", "new", "operator", "static",
                       "switch", "template", "this", "union", "namespace", "bool", "int",
                       "char", "long", "short", "float", "double", "void", "auto", "const"});
}

// "LV_EVENT_CLICKED" -> "on_clicked". Empty for the entries that are not events.
consteval std::string event_member(std::string_view enumerator) {
  constexpr std::string_view prefix = "LV_EVENT_";
  if (!enumerator.starts_with(prefix)) return {};
  auto rest = enumerator.substr(prefix.size());
  if (is_one_of(rest, {"ALL", "LAST", "LAST_CUSTOM", "PREPROCESS", "MARKED_DELETING"})) return {};
  std::string out = "on_";
  for (char c : rest) out += (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
  return out;
}

// The usable lv_<...>_set_* functions with this prefix.
consteval std::vector<std::meta::info> setters_with_prefix(std::string_view prefix) {
  std::vector<std::meta::info> out;
  for (auto f : api) {
    const auto id = std::meta::identifier_of(f);
    if (!id.starts_with(prefix)) continue;
    const auto prop = id.substr(prefix.size());
    // Styles get their own typed API; they are not plain properties.
    if (prop.starts_with("style_") || prop.starts_with("local_style_")) continue;
    if (is_keyword(prop) || std::meta::is_vararg_function(f)) continue;

    const auto params = std::meta::parameters_of(f);
    if (params.size() < 2) continue;
    bool usable = true;
    for (std::size_t i = 1; i < params.size(); ++i)
      if (std::meta::dealias(std::meta::type_of(params[i])) == std::meta::dealias(^^va_list))
        usable = false;
    if (usable) out.push_back(f);
  }
  return out;
}

// One member of a props struct. Names are views of static strings: std::string
// here costs too many constexpr steps.
struct member_item {
  std::string_view name;
  std::meta::info type;    // arg<...>
  std::meta::info target;  // the setter, the LV_EVENT_* enumerator, or null for `id`
};

consteval void sort_by_name(std::vector<member_item>& items) {
  for (std::size_t i = 1; i < items.size(); ++i)
    for (std::size_t j = i; j > 0 && items[j].name < items[j - 1].name; --j)
      std::swap(items[j], items[j - 1]);
}

consteval member_item setter_item(std::meta::info f, std::size_t prefix_size) {
  const auto prop = std::meta::identifier_of(f).substr(prefix_size);
  const auto params = std::meta::parameters_of(f);
  std::vector<std::meta::info> types;
  for (std::size_t i = 1; i < params.size(); ++i) types.push_back(value_type(prop, params[i]));
  return {prop, std::meta::substitute(^^arg, types), f};
}

// What every widget has: `id`, the lv_obj_set_* properties and the events.
consteval std::vector<member_item> scan_base() {
  std::vector<member_item> items;
  items.push_back({"id", ^^arg<std::string_view>, std::meta::info{}});
  for (auto f : setters_with_prefix("lv_obj_set_"))
    items.push_back(setter_item(f, std::string_view("lv_obj_set_").size()));
  for (auto e : std::meta::enumerators_of(std::meta::dealias(^^lv_event_code_t)))
    if (auto name = event_member(std::meta::identifier_of(e)); !name.empty())
      items.push_back({std::define_static_string(name), ^^arg<handler>, e});
  sort_by_name(items);
  return items;
}

// The base is found and sorted once, on first use, and stored as one array of
// triples: the name (a reflected string), the type and the target.
// A template only so that nothing here runs until a props struct is needed.
template <class D>
consteval std::vector<std::meta::info> pack_base() {
  std::vector<std::meta::info> out;
  for (const auto& it : scan_base()) {
    out.push_back(std::meta::reflect_constant_string(it.name));
    out.push_back(it.type);
    out.push_back(it.target);
  }
  return out;
}

template <class D = void>
inline constexpr std::span<const std::meta::info> base =
    std::define_static_array(pack_base<D>());

// The members of props<widget>, sorted by name: the widget's own setters merged
// into the base. A widget's own setter wins over one of obj with the same name.
// D only delays the base scan until a props struct is really needed.
template <class D = void>
consteval std::vector<member_item> members(std::string_view widget) {
  resolve_create(widget);  // a wrong widget name fails here, with the list of valid ones

  std::vector<member_item> own;
  if (widget != "obj") {
    const std::string prefix = "lv_" + std::string(widget) + "_set_";
    for (auto f : setters_with_prefix(prefix)) own.push_back(setter_item(f, prefix.size()));
    sort_by_name(own);
  }

  std::vector<member_item> items;
  std::size_t o = 0;
  for (std::size_t b = 0; b < base<D>.size(); b += 3) {
    const std::string_view name = std::meta::extract<const char*>(base<D>[b]);
    while (o < own.size() && own[o].name < name) items.push_back(own[o++]);
    if (o < own.size() && own[o].name == name)
      items.push_back(own[o++]);
    else
      items.push_back({name, base<D>[b + 1], base<D>[b + 2]});
  }
  while (o < own.size()) items.push_back(own[o++]);
  return items;
}

template <class D = void>
consteval std::vector<std::meta::info> member_specs(std::string_view widget) {
  std::vector<std::meta::info> specs;
  for (const auto& it : members<D>(widget))
    specs.push_back(std::meta::data_member_spec(it.type, {.name = it.name}));
  return specs;
}

template <class D = void>
consteval std::vector<std::meta::info> member_targets(std::string_view widget) {
  std::vector<std::meta::info> out;
  for (const auto& it : members<D>(widget)) out.push_back(it.target);
  return out;
}

// The second parameter lets a signature name props lazily: see detail::make.
template <fixed W, class = void>
struct props_of {
  struct type;
  consteval { std::meta::define_aggregate(^^type, member_specs<>(W.sv())); }
};

// What each member of props<W> stands for, in member order.
template <fixed W>
inline constexpr std::span<const std::meta::info> targets =
    std::define_static_array(member_targets<>(W.sv()));

// Unit types and strict<> hand LVGL their plain value; everything else passes as is.
template <class T>
constexpr decltype(auto) lower(const T& x) {
  if constexpr (requires { x.raw(); })
    return x.raw();
  else
    return (x);
}

inline void add_handler(lv_obj_t* o, lv_event_code_t code, const handler& h) {
  // A function pointer travels in user_data, so nothing is allocated.
  if (h.plain)
    lv_obj_add_event_cb(
        o, [](lv_event_t* e) { reinterpret_cast<void (*)()>(lv_event_get_user_data(e))(); },
        code, reinterpret_cast<void*>(h.plain));
  else if (h.with_event)
    lv_obj_add_event_cb(
        o,
        [](lv_event_t* e) {
          reinterpret_cast<void (*)(event)>(lv_event_get_user_data(e))(event{e});
        },
        code, reinterpret_cast<void*>(h.with_event));
}

// Applies member number I of props<W>, whose value is `a`, to a widget.
template <fixed W, std::size_t I, class A>
void apply_member(lv_obj_t* o, const A& a) {
  constexpr auto target = targets<W>[I];
  const auto& [... x] = a.v;
  if constexpr (target == std::meta::info{}) {
    // `id` names the node for lv::mount; there is nothing to apply.
  } else if constexpr (std::meta::is_enumerator(target)) {
    add_handler(o, [:target:], x...);
  } else {
    [:target:](o, lower(x)...);
  }
}

}  // namespace detail

// The typed properties and events of a widget. See the top of this file.
template <detail::fixed W>
using props = typename detail::props_of<W>::type;

namespace detail {

template <fixed W>
inline constexpr auto members_of_props = std::define_static_array(
    std::meta::nonstatic_data_members_of(^^props<W>, std::meta::access_context::unchecked()));

template <std::size_t N>
inline constexpr auto indices = [] {
  std::array<std::size_t, N> a{};
  for (std::size_t i = 0; i < N; ++i) a[i] = i;
  return a;
}();

// Applies every set member of `p`, read at run time.
template <fixed W>
void apply_props(lv_obj_t* o, const props<W>& p) {
  template for (constexpr auto i : indices<members_of_props<W>.size()>) {
    constexpr auto m = members_of_props<W>[i];
    if (p.[:m:].set) apply_member<W, i>(o, p.[:m:]);
  }
}

}  // namespace detail

}  // namespace lv
