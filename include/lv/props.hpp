#pragma once

// props<"arc"> is a struct that reflection generates from LVGL's C API: one member
// per lv_arc_set_* and lv_obj_set_* function, one per event (on_clicked, ...), an
// `id` and a `style`.
//
//   lv::props<"arc"> p{.id = "speedo", .range = {0, 240}, .rotation = 90 * lv::deg, .value = 120};
//
// Members are in alphabetical order, which is the order designated initializers
// must follow. Every member is typed: a wrong name, a wrong unit, a bool where an
// integer is expected and a narrowing conversion are all compile errors.

#include <lv/detail/api.hpp>
#include <lv/detail/fixed_string.hpp>
#include <lv/event.hpp>
#include <lv/style.hpp>
#include <lv/units.hpp>
#include <lv/values.hpp>

#include <array>
#include <concepts>
#include <cstdarg>
#include <cstdint>
#include <type_traits>
#include <span>
#include <string_view>
#include <tuple>
#include <utility>

namespace lv {

namespace detail {

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

// Setters that LVGL has deprecated. Reflection cannot see [[deprecated]], so they
// are listed here and get no property. A newly deprecated setter shows up as a
// warning in our own build, which is the cue to add it.
consteval bool is_deprecated_setter(std::string_view name) {
  return is_one_of(name, {"lv_obj_set_flag"});
}

// "LV_STATE_PRESSED" -> "style_pressed". Empty for the entries that select nothing
// on their own (the default state, the main part, the ANY and user values).
consteval std::string style_member(std::string_view enumerator, std::string_view prefix) {
  if (!enumerator.starts_with(prefix)) return {};
  const auto rest = enumerator.substr(prefix.size());
  if (is_one_of(rest, {"DEFAULT", "MAIN", "ANY", "CUSTOM_FIRST"}) || rest.starts_with("USER_"))
    return {};
  std::string out = "style_";
  for (char c : rest) out.push_back((c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c);
  return out;
}

// The style selector of a style member of props: its part or state.
template <std::meta::info Target>
constexpr lv_style_selector_t selector_of() {
  if constexpr (Target == ^^lv_obj_add_style)
    return LV_PART_MAIN;
  else
    return static_cast<lv_style_selector_t>([:Target:]);
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
    if (is_deprecated_setter(id)) continue;

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
  std::meta::info target;  // the setter; the LV_EVENT_* enumerator; lv_obj_add_style for
                           // `style`; the LV_STATE_* or LV_PART_* enumerator for
                           // `style_pressed` and the like; or null for `id`
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

// What every widget has: `id`, `style`, the lv_obj_set_* properties and the events.
template <class D>
consteval std::vector<member_item> scan_base() {
  std::vector<member_item> items;
  items.push_back({"id", ^^arg<std::string_view>, std::meta::info{}});
  items.push_back({"style", ^^style_t<D>, ^^lv_obj_add_style});
  // style_pressed, style_checked, ... and style_indicator, style_knob, ...: the same
  // style struct for one state or one part. The target is the LV_STATE_* or LV_PART_*
  // enumerator, which is the selector.
  for (auto e : std::meta::enumerators_of(std::meta::dealias(^^lv_state_t)))
    if (auto name = style_member(std::meta::identifier_of(e), "LV_STATE_"); !name.empty())
      items.push_back({std::define_static_string(name), ^^style_t<D>, e});
  for (auto e : std::meta::enumerators_of(std::meta::dealias(^^lv_part_t)))
    if (auto name = style_member(std::meta::identifier_of(e), "LV_PART_"); !name.empty())
      items.push_back({std::define_static_string(name), ^^style_t<D>, e});
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
  for (const auto& it : scan_base<D>()) {
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
    const std::string prefix = "lv_" + str(widget) + "_set_";
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
    specs.push_back(std::meta::data_member_spec(it.type, {.name = str(it.name)}));
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

// Reads the value of a subject as the type T that a setter takes.
template <class T>
T from_subject(lv_subject_t* s) {
  if constexpr (std::same_as<T, lv_color_t>) {
    return lv_subject_get_color(s);
  } else if constexpr (std::is_pointer_v<T>) {
    return static_cast<T>(const_cast<void*>(lv_subject_get_pointer(s)));
  } else if constexpr (std::same_as<T, strict<bool>>) {
    return lv_subject_get_int(s) != 0;
  } else if constexpr (std::is_enum_v<T>) {
    return static_cast<T>(lv_subject_get_int(s));
  } else if constexpr (std::same_as<T, length> || std::same_as<T, horizontal> ||
                       std::same_as<T, vertical>) {
    return length::px(lv_subject_get_int(s));
  } else if constexpr (std::same_as<T, opa>) {
    return opa::from_raw(static_cast<lv_opa_t>(lv_subject_get_int(s)));
  } else if constexpr (std::same_as<T, angle>) {
    return angle{lv_subject_get_int(s)};
  } else if constexpr (std::same_as<T, duration>) {
    return duration{static_cast<std::uint32_t>(lv_subject_get_int(s))};
  } else if constexpr (std::constructible_from<T, std::int32_t>) {
    return T{lv_subject_get_int(s)};
  } else {
    return T{};  // no state type converts to T, so no binding to it can exist
  }
}

// lv_<widget>_bind_<prop> for the setter lv_<widget>_set_<prop>, or a null reflection.
// LVGL's own bind functions are used when they exist: they also write the widget's
// value back to the subject (an arc, a slider).
consteval std::meta::info binder_of(std::meta::info setter) {
  const auto name = std::meta::identifier_of(setter);
  const auto at = name.find("_set_");
  const auto f = find(str(name.substr(0, at)) + "_bind_" + str(name.substr(at + 5)));
  if (f == std::meta::info{}) return f;
  const auto params = std::meta::parameters_of(f);
  const bool plain = params.size() == 2;
  const bool with_format =
      params.size() == 3 && std::meta::type_of(params[2]) == ^^const char*;
  return (plain || with_format) ? f : std::meta::info{};
}

// Whether member number I of props<W> can follow a state: LVGL has a bind function
// for it, or it is a setter that takes one value.
template <fixed W, std::size_t I>
consteval bool bindable() {
  const auto t = targets<W>[I];
  if (t == std::meta::info{} || !std::meta::is_function(t)) return false;
  return binder_of(t) != std::meta::info{} || std::meta::parameters_of(t).size() == 2;
}

// The observer of a generic binding: calls the setter with the subject's value.
// A named function, not a lambda: GCC's analyzer reports a null `this` for the
// function pointer made from a lambda here.
template <fixed W, std::size_t I, class V>
void follow_subject(lv_observer_t* observer, lv_subject_t* s) {
  auto* target = static_cast<lv_obj_t*>(lv_observer_get_target(observer));
  [:targets<W>[I]:](target, lower(from_subject<V>(s)));
}

// Makes member number I of props<W> follow the state it is bound to.
template <fixed W, std::size_t I, class A>
void bind_member(lv_obj_t* o, const A& a) {
  constexpr auto setter = targets<W>[I];
  lv_subject_t* subject = a.bound_subject(a.bound);
  if (subject == nullptr) return;  // LVGL had no memory for the subject

  constexpr auto native = binder_of(setter);
  if constexpr (native != std::meta::info{} && std::meta::parameters_of(native).size() == 3) {
    [:native:](o, subject, a.bound_format);
  } else if constexpr (native != std::meta::info{}) {
    [:native:](o, subject);
  } else if constexpr (std::tuple_size_v<decltype(a.v)> != 1) {
    // lv::mount rejects this at compile time; set() at run time ends up here.
    contract_assert(false && "this property takes several values and cannot follow a state");
  } else {
    using value_type = std::tuple_element_t<0, decltype(a.v)>;
    lv_subject_add_observer_obj(subject, &follow_subject<W, I, value_type>, o, nullptr);
  }
}

// Applies member number I of props<W>, whose value is `a`, to a widget.
template <fixed W, std::size_t I, class A>
void apply_member(lv_obj_t* o, const A& a) {
  constexpr auto target = targets<W>[I];
  if constexpr (target == std::meta::info{}) {
    // `id` names the node for lv::mount; there is nothing to apply.
  } else if constexpr (a_style<A>) {
    apply_style_local(o, a, selector_of<target>());
  } else if constexpr (std::meta::is_enumerator(target)) {
    const auto& [... x] = a.v;
    add_handler(o, [:target:], x...);
  } else {
    if (a.bound != nullptr) {
      bind_member<W, I>(o, a);
      return;
    }
    const auto& [... x] = a.v;
    [:target:](o, lower(x)...);
  }
}

// props<W>, named so that it is generated on first use: see props_of.
template <fixed W, class D>
using lazy_props = typename props_of<W, D>::type;

}  // namespace detail

// The typed properties and events of a widget. See the top of this file.
template <detail::fixed W>
using props = typename detail::props_of<W>::type;

namespace detail {

template <fixed W>
inline constexpr auto members_of_props = std::define_static_array(
    std::meta::nonstatic_data_members_of(^^props<W>, std::meta::access_context::unchecked()));

// Applies every set member of `p`, read at run time.
template <fixed W>
void apply_props(lv_obj_t* o, const props<W>& p) {
  template for (constexpr auto i : indices<members_of_props<W>.size()>) {
    constexpr auto m = members_of_props<W>[i];
    if (is_set(p.[:m:])) apply_member<W, i>(o, p.[:m:]);
  }
}

}  // namespace detail

}  // namespace lv
