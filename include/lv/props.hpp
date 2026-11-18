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
//
// A props struct is made with braces, also an empty one: `lv::props<"arc"> p{};`.
// `lv::props<"arc"> p;` does not compile: see detail::made_with_braces.

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

// Setters that LVGL v9.6 has deprecated (v10 has removed them). Reflection cannot
// see [[deprecated]], so they are listed here and get no property. The list is
// every setter declared after an LV_DEPRECATED marker in include/lvgl:
//   grep -rh -A2 "LV_DEPRECATED(" include/lvgl | grep -v '"' | grep -o "lv_[a-z0-9_]*_set_[a-z0-9_]*("
// A setter that LVGL deprecates later shows up as a warning in our own build.
consteval bool is_deprecated_setter(std::string_view name) {
  return is_one_of(name, {"lv_file_explorer_set_quick_access_path", "lv_file_explorer_set_sort",
                          "lv_gltf_set_env_brightness", "lv_image_buf_set_palette",
                          "lv_list_set_button_text", "lv_list_set_button_translation_tag",
                          "lv_menu_set_load_page_event", "lv_menu_set_mode_header",
                          "lv_menu_set_mode_root_back_button", "lv_menu_set_page",
                          "lv_menu_set_page_title", "lv_menu_set_page_title_static",
                          "lv_menu_set_sidebar_page", "lv_obj_set_flag",
                          "lv_obj_style_set_disabled", "lv_rlottie_set_current_frame",
                          "lv_rlottie_set_play_mode", "lv_scale_section_set_style",
                          "lv_spangroup_set_align", "lv_spangroup_set_mode",
                          "lv_textarea_set_align"});
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

// Where va_list is an array (x86-64 Linux), a parameter of it is a pointer to its
// element: the type an array has as a parameter.
consteval bool takes_va_list(std::meta::info f) {
  const auto list = std::meta::dealias(^^va_list), as_parameter = std::meta::decay(list);
  for (auto p : std::meta::parameters_of(f)) {
    const auto type = std::meta::dealias(std::meta::type_of(p));
    if (type == list || type == as_parameter) return true;
  }
  return false;
}

// Whether the setter `f`, called <prefix><prop>, becomes a property.
consteval bool is_usable_setter(std::meta::info f, std::string_view prop) {
  // Styles get their own typed API; they are not plain properties.
  if (prop.starts_with("style_") || prop.starts_with("local_style_")) return false;
  if (is_keyword(prop) || is_deprecated_setter(std::meta::identifier_of(f))) return false;
  if (std::meta::is_vararg_function(f) || takes_va_list(f)) return false;
  return std::meta::parameters_of(f).size() >= 2;  // the widget and at least one value
}

// The usable lv_<...>_set_* functions with this prefix.
consteval std::vector<std::meta::info> setters_with_prefix(std::string_view prefix) {
  std::vector<std::meta::info> out;
  for (auto f : api) {
    const auto id = std::meta::identifier_of(f);
    if (id.starts_with(prefix) && is_usable_setter(f, id.substr(prefix.size()))) out.push_back(f);
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

template <class D>
consteval member_item setter_item(std::meta::info f, std::size_t prefix_size) {
  const auto prop = std::meta::identifier_of(f).substr(prefix_size);
  const auto params = std::meta::parameters_of(f);
  std::vector<std::meta::info> types;
  for (std::size_t i = 1; i < params.size(); ++i)
    types.push_back(value_type<D>(f, prop, params[i]));
  return {prop, std::meta::substitute(^^arg, types), f};
}

// The style members of a widget, and of a theme's widget: `style`, then style_pressed,
// style_checked, ... and style_indicator, style_knob, ...: the same style struct for
// one state or one part. The target is lv_obj_add_style for `style`, and otherwise
// the LV_STATE_* or LV_PART_* enumerator, which is the selector.
struct named_style_slot {
  std::string_view name;
  std::meta::info target;
};

consteval std::vector<named_style_slot> named_style_slots() {
  std::vector<named_style_slot> slots{{"style", ^^lv_obj_add_style}};
  for (auto e : std::meta::enumerators_of(std::meta::dealias(^^lv_state_t)))
    if (auto name = style_member(std::meta::identifier_of(e), "LV_STATE_"); !name.empty())
      slots.push_back({std::define_static_string(name), e});
  for (auto e : std::meta::enumerators_of(std::meta::dealias(^^lv_part_t)))
    if (auto name = style_member(std::meta::identifier_of(e), "LV_PART_"); !name.empty())
      slots.push_back({std::define_static_string(name), e});
  return slots;
}

// What every widget has: `id`, `style`, the lv_obj_set_* properties and the events.
// `prefix` is "lv_obj_set_". It is a parameter, and the scans are handed on as
// arguments, because of GCC 16.2: a call whose arguments are all constants, written
// inside a function template, is run when the template is instantiated and again
// when the function runs. That doubled the cost of every scan.
template <class D>
consteval std::vector<member_item> scan_base(std::string_view prefix) {
  std::vector<member_item> items;
  items.push_back({"id", ^^arg<std::string_view>, std::meta::info{}});
  for (const auto& slot : named_style_slots()) items.push_back({slot.name, ^^style_t<D>, slot.target});
  for (auto f : setters_with_prefix(prefix)) items.push_back(setter_item<D>(f, prefix.size()));
  for (auto e : std::meta::enumerators_of(std::meta::dealias(^^lv_event_code_t)))
    if (auto name = event_member(std::meta::identifier_of(e)); !name.empty())
      items.push_back({std::define_static_string(name), ^^arg<handler>, e});
  sort_by_name(items);
  return items;
}

// The base is found and sorted once, on first use, and stored as one array of
// triples: the name (a reflected string), the type and the target.
consteval std::vector<std::meta::info> pack_base(const std::vector<member_item>& items) {
  std::vector<std::meta::info> out;
  for (const auto& it : items) {
    out.push_back(std::meta::reflect_constant_string(it.name));
    out.push_back(it.type);
    out.push_back(it.target);
  }
  return out;
}

// A template only so that nothing here runs until a props struct is needed.
template <class D = void>
inline constexpr std::span<const std::meta::info> base =
    std::define_static_array(pack_base(scan_base<D>("lv_obj_set_")));

// The widget's own setters as members, sorted by name. None for obj itself.
template <class D>
consteval std::vector<member_item> own_members(std::string_view widget) {
  std::vector<member_item> own;
  if (widget == "obj") return own;
  const std::string prefix = "lv_" + str(widget) + "_set_";
  for (auto f : setters_with_prefix(prefix)) own.push_back(setter_item<D>(f, prefix.size()));
  sort_by_name(own);
  return own;
}

// The members of props<widget>, sorted by name: the widget's own setters merged
// into the base. A widget's own setter wins over one of obj with the same name.
// D only delays the base scan until a props struct is really needed.
template <class D = void>
consteval std::vector<member_item> members(std::string_view widget) {
  resolve_create(widget);  // a wrong widget name fails here, with the list of valid ones
  const auto own = own_members<D>(widget);

  std::vector<member_item> items;
  std::size_t o = 0;
  for (std::size_t b = 0; b < base<D>.size(); b += 3) {
    const std::string_view name = std::meta::extract<const char*>(base<D>[b]);
    while (o < own.size() && own[o].name < name) items.push_back(own[o++]);
    const bool overridden = o < own.size() && own[o].name == name;
    items.push_back(overridden ? own[o++] : member_item{name, base<D>[b + 1], base<D>[b + 2]});
  }
  items.insert(items.end(), own.begin() + static_cast<std::ptrdiff_t>(o), own.end());
  return items;
}

// Members as pairs: what std::meta::define_aggregate needs for the member, then its
// target.
consteval std::vector<std::meta::info> member_pairs(const std::vector<member_item>& items) {
  std::vector<std::meta::info> out;
  for (const auto& it : items) {
    out.push_back(std::meta::data_member_spec(it.type, {.name = str(it.name)}));
    out.push_back(it.target);
  }
  return out;
}

// The members of props<W> as such pairs. The struct and its targets both come from
// this table, so the setters of a widget are looked up once.
template <fixed W>
inline constexpr std::span<const std::meta::info> member_table =
    std::define_static_array(member_pairs(members<>(W.sv())));

// Every second entry of member_table<W> from `first` on: 0 for the members, 1 for
// their targets.
template <fixed W>
consteval std::vector<std::meta::info> member_column(std::size_t first) {
  std::vector<std::meta::info> out;
  for (std::size_t i = first; i < member_table<W>.size(); i += 2) out.push_back(member_table<W>[i]);
  return out;
}

template <fixed W>
consteval std::vector<std::meta::info> member_specs() {
  auto specs = member_column<W>(0);
  specs.push_back(braces_member());
  return specs;
}

// The second parameter lets a signature name props lazily: see detail::make.
template <fixed W, class = void>
struct props_of {
  struct type;
  consteval { std::meta::define_aggregate(^^type, member_specs<W>()); }
};

// What each member of props<W> stands for, in member order. The second parameter
// lets a template name the targets of a widget it knows without generating them
// where it is only read: targets<"obj", D>.
template <fixed W, class = void>
inline constexpr std::span<const std::meta::info> targets =
    std::define_static_array(member_column<W>(1));

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

// from_number(n, type): a whole number, as the type a setter takes. A state carries one
// in its subject, a field of a row (lv/list.hpp) in its own value. One overload per kind
// of type.
template <class T>
using as = std::type_identity<T>;

inline strict<bool> from_number(std::int32_t n, as<strict<bool>>) { return n != 0; }

template <class T>
  requires std::is_enum_v<T>
T from_number(std::int32_t n, as<T>) {
  return static_cast<T>(n);
}

inline length from_number(std::int32_t n, as<length>) { return length::px(n); }

template <class Axis>
along<Axis> from_number(std::int32_t n, as<along<Axis>>) {
  return length::px(n);
}

inline opa from_number(std::int32_t n, as<opa>) { return opa::from_raw(static_cast<lv_opa_t>(n)); }

inline angle from_number(std::int32_t n, as<angle>) { return angle{n}; }

inline decidegrees from_number(std::int32_t n, as<decidegrees>) { return decidegrees{n}; }

inline duration from_number(std::int32_t n, as<duration>) {
  return duration{static_cast<std::uint32_t>(n)};
}

// A number for a setter that takes a unit unsigned. A negative number breaks the contract
// of nonnegative.
template <class U>
nonnegative<U> from_number(std::int32_t n, as<nonnegative<U>>) {
  return from_number(n, as<U>{});
}

// Anything else: built from the integer if it can be, such as strict<int>.
template <class T>
T from_number(std::int32_t n, as<T>) {
  if constexpr (std::constructible_from<T, std::int32_t>)
    return T{n};
  else
    return T{};  // no state type converts to T, so no binding to it can exist
}

// from_subject(subject, type): the value of a subject as the type a setter takes.
inline lv_color_t from_subject(lv_subject_t* s, as<lv_color_t>) { return lv_subject_get_color(s); }

template <class T>
  requires std::is_pointer_v<T>
T from_subject(lv_subject_t* s, as<T>) {
  return static_cast<T>(const_cast<void*>(lv_subject_get_pointer(s)));
}

template <class T>
T from_subject(lv_subject_t* s, as<T>) {
  return from_number(lv_subject_get_int(s), as<T>{});
}

// Whether `f` is one of LVGL's bind functions of a property: it takes the widget and
// a subject, and optionally a format.
consteval bool is_binder_shape(std::meta::info f) {
  const auto params = std::meta::parameters_of(f);
  const bool plain = params.size() == 2;
  const bool with_format = params.size() == 3 && std::meta::type_of(params[2]) == ^^const char*;
  return plain || with_format;
}

// Whether `setter`, lv_<widget>_set_<prop>, sets a plain integer (not a bool, not an
// enum) that lv_<widget>_get_<prop> reads back.
consteval bool sets_and_gets_integer(std::meta::info setter) {
  const auto params = std::meta::parameters_of(setter);
  if (params.size() < 2) return false;
  const auto t = std::meta::remove_cv(std::meta::dealias(std::meta::type_of(params[1])));
  if (!std::meta::is_integral_type(t) || t == ^^bool) return false;
  const auto name = std::meta::identifier_of(setter);
  const auto at = name.find("_set_");
  const auto getter = find(str(name.substr(0, at)) + "_get_" + str(name.substr(at + 5)));
  return getter != std::meta::info{} &&
         std::meta::remove_cv(std::meta::dealias(std::meta::return_type_of(getter))) == t;
}

// The bind function of `widget` ("lv_roller_") that takes the widget and a subject and
// whose name matches no setter, such as lv_roller_bind_value; or a null reflection.
consteval std::meta::info unmatched_binder(std::string_view widget) {
  const std::string prefix = str(widget) + "bind_";
  for (auto f : api) {
    const auto id = std::meta::identifier_of(f);
    if (!id.starts_with(prefix) || std::meta::parameters_of(f).size() != 2) continue;
    if (find(str(widget) + "set_" + str(id.substr(prefix.size()))) == std::meta::info{}) return f;
  }
  return {};
}

// The only setter of `widget` that sets and gets a plain integer, or a null reflection
// when it has none or several.
consteval std::meta::info only_integer_setter(std::string_view widget) {
  const std::string prefix = str(widget) + "set_";
  std::meta::info found{};
  for (auto f : api) {
    if (!std::meta::identifier_of(f).starts_with(prefix) || !sets_and_gets_integer(f)) continue;
    if (found != std::meta::info{}) return {};
    found = f;
  }
  return found;
}

// The setter that an unmatched bind function belongs to, found once for each of them.
template <std::meta::info Binder>
inline constexpr std::meta::info setter_of_binder =
    only_integer_setter(std::meta::identifier_of(Binder).substr(
        0, std::meta::identifier_of(Binder).find("bind_")));

// The bind function of the setter lv_<widget>_set_<prop>, or a null reflection.
// LVGL's own bind functions are used when they exist: they also write the widget's
// value back to the subject (an arc, a slider). It is lv_<widget>_bind_<prop> when
// LVGL names the two alike. LVGL names the one of a roller and of a dropdown
// lv_<widget>_bind_value, for the setter lv_<widget>_set_selected: a bind function that
// matches no setter belongs to the only property the widget sets and gets as a plain
// integer, the value it binds.
consteval std::meta::info binder_of(std::meta::info setter) {
  const auto name = std::meta::identifier_of(setter);
  const auto at = name.find("_set_");
  const auto widget = name.substr(0, at + 1);
  if (const auto f = find(str(name.substr(0, at)) + "_bind_" + str(name.substr(at + 5)));
      f != std::meta::info{})
    return is_binder_shape(f) ? f : std::meta::info{};
  if (!sets_and_gets_integer(setter)) return {};
  const auto f = unmatched_binder(widget);
  if (f == std::meta::info{}) return f;
  return std::meta::extract<std::meta::info>(
             std::meta::substitute(^^setter_of_binder, {std::meta::reflect_constant(f)})) == setter
             ? f
             : std::meta::info{};
}

// binder_of(Setter), found once for each setter.
template <std::meta::info Setter>
inline constexpr std::meta::info binder = binder_of(Setter);

// Whether member number I of props<W> can follow a state: LVGL has a bind function
// for it, or it is a setter that takes one value.
template <fixed W, std::size_t I>
consteval bool bindable() {
  constexpr auto t = targets<W>[I];
  if constexpr (t == std::meta::info{} || !std::meta::is_function(t))
    return false;
  else
    return binder<t> != std::meta::info{} || std::meta::parameters_of(t).size() == 2;
}

// The observer of a generic binding: calls the setter with the subject's value.
// A named function, not a lambda: GCC's analyzer reports a null `this` for the
// function pointer made from a lambda here.
template <fixed W, std::size_t I, class V>
void follow_subject(lv_observer_t* observer, lv_subject_t* s) {
  auto* target = static_cast<lv_obj_t*>(lv_observer_get_target(observer));
  [:targets<W>[I]:](target, lower(from_subject(s, as<V>{})));
}

// Makes member number I of props<W> follow the state it is bound to.
template <fixed W, std::size_t I, class A>
void bind_member(lv_obj_t* o, const A& a) {
  constexpr auto setter = targets<W>[I];
  lv_subject_t* subject = a.bound_subject(a.bound);
  if (subject == nullptr) return;  // LVGL had no memory for the subject

  constexpr auto native = binder<setter>;
  if constexpr (native != std::meta::info{} && std::meta::parameters_of(native).size() == 3) {
    [:native:](o, subject, a.bound_format);
  } else if constexpr (native != std::meta::info{}) {
    [:native:](o, subject);
  } else if constexpr (std::tuple_size_v<typename A::types> != 1) {
    // lv::mount rejects this at compile time; set() at run time ends up here.
    contract_assert(false && "this property takes several values and cannot follow a state");
  } else {
    using value_type = std::tuple_element_t<0, typename A::types>;
    lv_subject_add_observer_obj(subject, &follow_subject<W, I, value_type>, o, nullptr);
  }
}

// lv_<widget>_set_text_static for lv_<widget>_set_text, lv_<widget>_set_options_static for
// lv_<widget>_set_options, or a null reflection. The static one keeps the pointer and
// copies nothing.
consteval std::meta::info static_text_of(std::meta::info setter) {
  if (setter == std::meta::info{} || !std::meta::is_function(setter)) return {};
  const auto name = std::meta::identifier_of(setter);
  if (!name.ends_with("_set_text") && !name.ends_with("_set_options")) return {};
  const auto f = find(str(name) + "_static");
  if (f == std::meta::info{} || std::meta::parameters_of(f).size() != std::meta::parameters_of(setter).size())
    return {};
  return f;
}

// Whether a value is a text that lives for the whole program: a list of options that the
// compiler joined.
template <class T>
constexpr bool lives_on(const T&) {
  return false;
}

constexpr bool lives_on(const option_list& list) { return list.kept; }

// Calls the setter `Target`, or its static twin when the value is a constant text.
template <fixed W, std::meta::info Target, bool Constant, class... V>
void set_property(lv_obj_t* o, V... v) {
  constexpr auto kept_text = static_text_of(Target);
  if constexpr (kept_text != std::meta::info{}) {
    // LVGL shows static text uncut when the label cuts with dots, so that one is copied.
    bool keep = Constant || (lives_on(v) || ...);
    if constexpr (W.sv() == "label") keep = keep && lv_label_get_long_mode(o) != LV_LABEL_LONG_MODE_DOTS;
    if (keep)
      [:kept_text:](o, v...);
    else
      [:Target:](o, v...);
  } else {
    [:Target:](o, v...);
  }
}

// Applies member number I of props<W>, whose value is `a`, to a widget. `Constant` says
// the value lives for the whole program, as in a constexpr tree: a text then needs no copy.
template <fixed W, std::size_t I, class A, bool Constant = false>
void apply_member(lv_obj_t* o, const A& a) {
  constexpr auto target = targets<W>[I];
  if constexpr (target == std::meta::info{}) {
    // `id` names the node for lv::mount; there is nothing to apply.
  } else if constexpr (style_value<A>) {
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
    set_property<W, target, Constant>(o, lower(x)...);
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

// The second parameter delays them as that of targets does.
template <fixed W, class = void>
inline constexpr auto members_of_props = std::define_static_array(members_before_braces(^^props<W>));

// A style of a node of a tree: none, or one style. An empty one is two zeros to the
// compiler, where a style with nothing set is 139 unset members.
template <class S>
struct maybe {
  bool has;
  union {
    char nothing;
    S value;
  };
};

template <class T>
inline constexpr bool is_maybe = false;
template <class S>
inline constexpr bool is_maybe<maybe<S>> = true;

template <class S>
constexpr bool is_set(const maybe<S>& m) {
  return m.has;
}

// The members of kept<W>: those of props<W>, with every style in a maybe<>.
template <fixed W, class D>
consteval std::vector<std::meta::info> kept_specs() {
  std::vector<std::meta::info> specs;
  for (auto m : members_of_props<W>) {
    const auto type = std::meta::type_of(m);
    const bool style = std::meta::dealias(type) == std::meta::dealias(^^style_t<D>);
    specs.push_back(std::meta::data_member_spec(style ? ^^maybe<style_t<D>> : type,
                                                {.name = std::meta::identifier_of(m)}));
  }
  return specs;
}

// The properties of a widget as a node of a tree keeps them. props<W> is what is
// written; it has a full style for each of the 25 states and parts, which makes it
// thousands of members that the compiler walks every time a node is copied. A node
// is copied at every level of the tree above it, so it keeps this smaller form.
// The second parameter delays it like that of props_of.
template <fixed W, class D = void>
struct kept_of {
  struct type;
  consteval { std::meta::define_aggregate(^^type, kept_specs<W, D>()); }
};

template <fixed W, class D = void>
using kept = typename kept_of<W, D>::type;

template <fixed W>
inline constexpr auto members_of_kept = std::define_static_array(
    std::meta::nonstatic_data_members_of(^^kept<W>, std::meta::access_context::unchecked()));

// `p` in the form a node keeps.
template <fixed W, class D>
constexpr kept<W, D> keep(const lazy_props<W, D>& p) {
  kept<W, D> out{};
  template for (constexpr auto i : indices<members_of_props<W>.size()>) {
    const auto& given = p.[:members_of_props<W>[i]:];
    auto& held = out.[:members_of_kept<W>[i]:];
    if (is_set(given)) {
      if constexpr (is_maybe<std::remove_cvref_t<decltype(held)>>)
        held = {.has = true, .value = given};
      else
        held = given;
    }
  }
  return out;
}

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
