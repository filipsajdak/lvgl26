#pragma once

// props<"arc"> is a struct that reflection generates from LVGL's C API: one member
// per lv_arc_set_* and lv_obj_set_* function, one per event (on_clicked, ...), an
// `id` and a `style`.
//
//   lv::props<"arc"> p{.id = "speedo", .range = {0, 240}, .rotation = 90 * lv::deg, .value = 120};
//
// Members are in four groups: the id, the properties, the styles and the handlers, each in
// alphabetical order. That is the order designated initializers must follow. Every member is typed: a wrong name, a wrong unit, a bool where an
// integer is expected and a narrowing conversion are all compile errors.
//
// A props struct is made with braces, also an empty one: `lv::props<"arc"> p{};`.
// `lv::props<"arc"> p;` does not compile: see detail::made_with_braces.

#include <lv/detail/api.hpp>
#include <lv/detail/fixed_string.hpp>
#include <lv/detail/setter_order.hpp>
#include <lv/detail/source.hpp>
#include <lv/event.hpp>
#include <lv/style.hpp>
#include <lv/units.hpp>
#include <lv/values.hpp>

#include <array>
#include <concepts>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <new>
#include <type_traits>
#include <span>
#include <string_view>
#include <tuple>
#include <utility>

namespace lv {

namespace detail {

// A literal compared with a string_view is written "..."sv: a view made from a pointer alone
// counts its characters in a loop that costs the compiler about 25,000 instructions each.
using namespace std::string_view_literals;

// "LV_EVENT_CLICKED" -> "on_clicked". Empty for the entries that are not events.
consteval std::string event_member(std::string_view enumerator) {
  constexpr std::string_view prefix = "LV_EVENT_";
  if (!enumerator.starts_with(prefix)) return {};
  auto rest = enumerator.substr(prefix.size());
  if (is_one_of(rest, {"ALL"sv, "LAST"sv, "LAST_CUSTOM"sv, "PREPROCESS"sv, "MARKED_DELETING"sv})) return {};
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
  return is_one_of(name, {"lv_file_explorer_set_quick_access_path"sv, "lv_file_explorer_set_sort"sv,
                          "lv_gltf_set_env_brightness"sv, "lv_image_buf_set_palette"sv,
                          "lv_list_set_button_text"sv, "lv_list_set_button_translation_tag"sv,
                          "lv_menu_set_load_page_event"sv, "lv_menu_set_mode_header"sv,
                          "lv_menu_set_mode_root_back_button"sv, "lv_menu_set_page"sv,
                          "lv_menu_set_page_title"sv, "lv_menu_set_page_title_static"sv,
                          "lv_menu_set_sidebar_page"sv, "lv_obj_set_flag"sv,
                          "lv_obj_style_set_disabled"sv, "lv_rlottie_set_current_frame"sv,
                          "lv_rlottie_set_play_mode"sv, "lv_scale_section_set_style"sv,
                          "lv_spangroup_set_align"sv, "lv_spangroup_set_mode"sv,
                          "lv_textarea_set_align"sv});
}

// "LV_STATE_PRESSED" -> "style_pressed". Empty for the entries that select nothing
// on their own (the default state, the main part, the ANY and user values).
consteval std::string style_member(std::string_view enumerator, std::string_view prefix) {
  if (!enumerator.starts_with(prefix)) return {};
  const auto rest = enumerator.substr(prefix.size());
  if (is_one_of(rest, {"DEFAULT"sv, "MAIN"sv, "ANY"sv, "CUSTOM_FIRST"sv}) || rest.starts_with("USER_"sv))
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
  if (prop.starts_with("style_"sv) || prop.starts_with("local_style_"sv)) return false;
  if (is_keyword(prop) || is_deprecated_setter(std::meta::identifier_of(f))) return false;
  if (std::meta::is_vararg_function(f) || takes_va_list(f)) return false;
  return std::meta::parameters_of(f).size() >= 2;  // the widget and at least one value
}

// The usable lv_<...>_set_* functions with this prefix.
consteval std::vector<std::meta::info> setters_with_prefix(std::string_view prefix) {
  std::vector<std::meta::info> out;
  for (auto f : api(prefix)) {
    const auto id = std::meta::identifier_of(f);
    if (has_prefix(id, prefix) && is_usable_setter(f, id.substr(prefix.size()))) out.push_back(f);
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
  int group = 1;           // where it stands: member_group::*, set where the item is made
};

// The groups of a props struct, in order: the id, the properties (the setters), the styles
// (`style`, `style_pressed`, ...) and the handlers (`on_clicked`, ...). Each member is put in its
// group where it is made, so that no name is looked at to find it.
namespace member_group {
inline constexpr int id = 0, property = 1, style = 2, handler = 3;
}  // namespace member_group

template <class D>
consteval member_item setter_item(std::meta::info f, std::size_t prefix_size) {
  const auto prop = std::meta::identifier_of(f).substr(prefix_size);
  const auto params = std::meta::parameters_of(f);
  std::vector<std::meta::info> types;
  // A setter called options may take a list: one comparison for each setter of every widget,
  // which every file pays.
  const bool may_list = prop == "options"sv;
  for (std::size_t i = 1; i < params.size(); ++i) {
    auto type = value_type<D>(f, prop, params[i]);
    if (may_list && type == (^^const char*) && takes_option_list(f, parameter_name(params[i])))
      type = ^^option_list;
    types.push_back(type);
  }
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
  items.push_back({"id", ^^arg<std::string_view>, std::meta::info{}, member_group::id});
  for (const auto& slot : named_style_slots()) items.push_back({slot.name, ^^style_t<D>, slot.target, member_group::style});
  for (auto f : setters_with_prefix(prefix)) items.push_back(setter_item<D>(f, prefix.size()));
  for (auto e : std::meta::enumerators_of(std::meta::dealias(^^lv_event_code_t)))
    if (auto name = event_member(std::meta::identifier_of(e)); !name.empty())
      items.push_back({std::define_static_string(name), ^^arg<handler>, e, member_group::handler});
  sort_by_group_and_name(items);
  return items;
}

// The base is found and sorted once, on first use, and stored as one array: a triple for each
// member (the name as a reflected string, the type and the target), and then, for each of the four
// groups, the number of members that come before the end of it. The members are sorted by group,
// so a group is a range, and no member needs its group stored.
consteval std::vector<std::meta::info> pack_base(const std::vector<member_item>& items) {
  std::vector<std::meta::info> out;
  std::size_t ends[4] = {};
  for (const auto& it : items) {
    out.push_back(std::meta::reflect_constant_string(it.name));
    out.push_back(it.type);
    out.push_back(it.target);
    for (int group = it.group; group < 4; ++group) ++ends[group];
  }
  for (const auto end : ends) out.push_back(std::meta::reflect_constant(end));
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
  if (widget == "obj"sv) return own;
  const std::string prefix = "lv_" + str(widget) + "_set_";
  for (auto f : setters_with_prefix(prefix)) own.push_back(setter_item<D>(f, prefix.size()));
  sort_by_group_and_name(own);
  return own;
}

// Whether an own setter comes before the base member `name` of `group`.
consteval bool comes_before(const member_item& own, int group, std::string_view name) {
  return own.group < group || (own.group == group && own.name < name);
}

// Whether own setter `o` is the base member `name` of `group`, which it replaces.
consteval bool replaces(const std::vector<member_item>& own, std::size_t o, int group, std::string_view name) {
  return o < own.size() && own[o].group == group && own[o].name == name;
}

// The end of each group in the base, as the pack of the base lists them after its members.
template <class D>
consteval std::array<std::size_t, 4> base_group_ends() {
  std::array<std::size_t, 4> ends{};
  const std::size_t members_in_base = (base<D>.size() - 4) / 3;
  for (std::size_t g = 0; g < 4; ++g) ends[g] = std::meta::extract<std::size_t>(base<D>[members_in_base * 3 + g]);
  return ends;
}

// The members of props<widget>, by group and then by name: the widget's own setters merged
// into the base. A widget's own setter wins over one of obj with the same name.
// D only delays the base scan until a props struct is really needed. The loop stays here, in the
// one function: taking it into a function template of its own cost 3.6 percent of a small file.
template <class D = void>
consteval std::vector<member_item> members(std::string_view widget) {
  resolve_create(widget);  // a wrong widget name fails here, with the list of valid ones
  const auto own = own_members<D>(widget);
  const auto ends = base_group_ends<D>();
  const std::size_t members_in_base = (base<D>.size() - 4) / 3;
  std::vector<member_item> items;
  std::size_t o = 0;
  int group = 0;
  for (std::size_t b = 0; b < members_in_base * 3; b += 3) {
    while (b / 3 >= ends[static_cast<std::size_t>(group)]) ++group;
    const std::string_view name = std::meta::extract<const char*>(base<D>[b]);
    while (o < own.size() && comes_before(own[o], group, name)) items.push_back(own[o++]);
    items.push_back(replaces(own, o, group, name) ? own[o++] : member_item{name, base<D>[b + 1], base<D>[b + 2], group});
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
  for (auto f : api(widget)) {
    const auto id = std::meta::identifier_of(f);
    if (!has_prefix(id, prefix) || std::meta::parameters_of(f).size() != 2) continue;
    if (find(str(widget) + "set_" + str(id.substr(prefix.size()))) == std::meta::info{}) return f;
  }
  return {};
}

// The only setter of `widget` that sets and gets a plain integer, or a null reflection
// when it has none or several.
consteval std::meta::info only_integer_setter(std::string_view widget) {
  const std::string prefix = str(widget) + "set_";
  std::meta::info found{};
  for (auto f : api(widget)) {
    if (!has_prefix(std::meta::identifier_of(f), prefix) || !sets_and_gets_integer(f)) continue;
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

// lv_<widget>_get_<prop> for the setter lv_<widget>_set_<prop>, if it takes the widget
// alone and returns an integer. A widget the user changes is read back with it.
consteval std::meta::info number_getter_of(std::meta::info setter) {
  // A setter of a text or a pointer has no number to read back: skip the search by name.
  const auto value = std::meta::remove_cv(std::meta::dealias(std::meta::type_of(std::meta::parameters_of(setter)[1])));
  if (!std::meta::is_integral_type(value)) return {};
  const auto name = std::meta::identifier_of(setter);
  const auto at = name.find("_set_");
  const auto g = find(str(name.substr(0, at)) + "_get_" + str(name.substr(at + 5)));
  if (g == std::meta::info{} || std::meta::parameters_of(g).size() != 1) return {};
  const auto r = std::meta::remove_cv(std::meta::dealias(std::meta::return_type_of(g)));
  return std::meta::is_integral_type(r) ? g : std::meta::info{};
}

// Whether a property follows a state through LVGL's own bind function, and so through an
// LVGL subject. Every other property follows it through a watcher of the state.
//
// LVGL's bind function does what the public API cannot in two cases:
//   - The setter takes more than the value (the animation of a bar, a slider or a roller).
//     LVGL animates the change only once the widget has been drawn, and tells that by a
//     field that is not public.
//   - The setter takes a bool and has no getter: a widget state such as `checked`, which
//     LVGL writes back to the subject when the user changes it. lvgl26 finds the getter
//     by name and has none to find.
consteval bool keeps_lvgl_binder(std::meta::info setter, std::meta::info native) {
  if (native == std::meta::info{}) return false;
  const auto params = std::meta::parameters_of(setter);
  if (params.size() > 2) return true;
  const auto value = std::meta::remove_cv(std::meta::dealias(std::meta::type_of(params[1])));
  const bool is_bool = value == ^^bool;
  return is_bool && number_getter_of(setter) == std::meta::info{};
}

// keeps_lvgl_binder(Setter) and number_getter_of(Setter), found once for each setter: both
// search LVGL's whole API by name.
template <std::meta::info Setter>
inline constexpr bool keeps_binder = keeps_lvgl_binder(Setter, binder<Setter>);

template <std::meta::info Setter>
inline constexpr std::meta::info number_getter = number_getter_of(Setter);

// A property that follows a state: the widget, and what to ask the state.
struct bound_node : watch_node {
  lv_obj_t* widget = nullptr;
  const void* source = nullptr;
  source_ops_fn ops = nullptr;
};

// from_source(node, type): the value of the state that a node follows, as the type a setter
// takes. A number is the usual case and costs one call that returns in a register.
inline lv_color_t from_source(const bound_node& n, as<lv_color_t>) { return n.ops().read(n.source).color; }

template <class T>
  requires std::is_pointer_v<T>
T from_source(const bound_node& n, as<T>) {
  return static_cast<T>(const_cast<void*>(n.ops().read(n.source).pointer));
}

template <class T>
T from_source(const bound_node& n, as<T>) {
  return from_number(n.ops().number(n.source), as<T>{});
}

// A bound text that is formatted by LVGL's printf into room behind the node, so that the
// label shows the buffer and a change allocates nothing.
struct text_node : bound_node {
  const char* format = nullptr;
  std::size_t room = 0;

  char* text() { return reinterpret_cast<char*>(this + 1); }
};

// The widget goes: the watcher goes with it.
inline void drop_node(lv_event_t* e) {
  auto* n = static_cast<bound_node*>(lv_event_get_user_data(e));
  watch_list::remove(*n);
  lv_free(n);
}

// Shows the state's value through the setter of member I of props<W>.
template <fixed W, std::size_t I, class V>
void show_value(watch_node& n) {
  auto& b = static_cast<bound_node&>(n);
  [:targets<W>[I]:](b.widget, lower(from_source(b, as<V>{})));
}

template <fixed W, std::meta::info Target, bool Constant, class... V>
void set_property(lv_obj_t* o, V... v);

// Shows the state's number in a text, through the setter of member I of props<W>.
template <fixed W, std::size_t I>
void show_text(watch_node& n) {
  auto& t = static_cast<text_node&>(n);
  lv_snprintf(t.text(), t.room, t.format, t.ops().number(t.source));
  set_property<W, targets<W>[I], true>(t.widget, static_cast<const char*>(t.text()));
}

// A widget that the user changes writes its value back to the state.
template <std::meta::info Getter>
void write_back(lv_event_t* e) {
  auto* n = static_cast<bound_node*>(lv_event_get_user_data(e));
  if (!n->linked()) return;  // the state has gone
  n->ops().write(n->source, static_cast<std::int32_t>([:Getter:](lv_event_get_current_target_obj(e))));
}

// The widest number a format is given: a 32-bit integer.
inline constexpr std::size_t widest_number = 11;

// Makes the node of a binding, with `extra` bytes behind it. It is freed with the widget.
// Null if LVGL has no memory.
template <class Node>
Node* make_node(lv_obj_t* o, const void* source, source_ops_fn ops, void (*changed)(watch_node&),
                std::size_t extra = 0) {
  void* memory = lv_malloc(sizeof(Node) + extra);
  if (memory == nullptr) return nullptr;
  Node* n = ::new (memory) Node{};
  n->widget = o;
  n->source = source;
  n->ops = ops;
  n->changed = changed;
  if (lv_obj_add_event_cb(o, &drop_node, LV_EVENT_DELETE, n) == nullptr) {
    lv_free(n);
    return nullptr;
  }
  return n;
}

// What does not depend on the property, so that one copy of it serves every property:
// makes the node of a binding, puts it in the state's list and shows the value at once.
// `show` is the function of the property that sets its value. Null if LVGL has no memory.
[[gnu::noinline]] inline bound_node* watch(lv_obj_t* o, const void* source, source_ops_fn ops,
                                           void (*show)(watch_node&)) {
  bound_node* node = make_node<bound_node>(o, source, ops, show);
  if (node == nullptr) return nullptr;
  ops().watchers(source).append(*node);
  show(*node);
  return node;
}

// The same for a text that is shown through a format.
[[gnu::noinline]] inline bound_node* watch_text(lv_obj_t* o, const void* source, source_ops_fn ops,
                                                void (*show)(watch_node&), const char* format) {
  const std::size_t room = std::strlen(format) + widest_number + 1;
  auto* node = make_node<text_node>(o, source, ops, show, room);
  if (node == nullptr) return nullptr;
  node->format = format;
  node->room = room;
  ops().watchers(source).append(*node);
  show(*node);
  return node;
}

// Adds the handler that writes a widget's value back to the state it follows.
[[gnu::noinline]] inline void write_back_with(lv_obj_t* o, bound_node& node, lv_event_cb_t handler) {
  if (node.ops().write != nullptr) lv_obj_add_event_cb(o, handler, LV_EVENT_VALUE_CHANGED, &node);
}

// Makes member number I of props<W> follow the state it is bound to, through a watcher.
template <fixed W, std::size_t I, class A>
void watch_member(lv_obj_t* o, const A& a) {
  constexpr auto setter = targets<W>[I];
  using value_type = std::tuple_element_t<0, typename A::types>;
  bound_node* node = nullptr;
  if constexpr (std::same_as<value_type, const char*>) {
    if (a.bound_format != nullptr) node = watch_text(o, a.bound, a.bound_ops, &show_text<W, I>, a.bound_format);
  }
  if constexpr (std::same_as<value_type, const char*>) {
    if (a.bound_format == nullptr) node = watch(o, a.bound, a.bound_ops, &show_value<W, I, value_type>);
  } else {
    node = watch(o, a.bound, a.bound_ops, &show_value<W, I, value_type>);
  }
  // Nested, not `&&`: the getter of a property that LVGL has no bind function for is not looked up.
  if constexpr (binder<setter> != std::meta::info{}) {
    if constexpr (number_getter<setter> != std::meta::info{}) {
      if (node != nullptr) write_back_with(o, *node, &write_back<number_getter<setter>>);
    }
  }
}

// Makes member number I of props<W> follow the state it is bound to: through LVGL's bind
// function where there is no other way (keeps_lvgl_binder), through a watcher otherwise.
template <fixed W, std::size_t I, class A>
void bind_member(lv_obj_t* o, const A& a) {
  constexpr auto setter = targets<W>[I];
  contract_assert(a.follows != follows_choice &&
                  "a state of an enum follows the choice of options in a constexpr tree only");
  constexpr auto native = binder<setter>;
  if constexpr (keeps_binder<setter>) {
    lv_subject_t* subject = a.bound_ops().subject(a.bound);
    if (subject == nullptr) return;  // LVGL had no memory for the subject
    if constexpr (std::meta::parameters_of(native).size() == 3)
      [:native:](o, subject, a.bound_format);
    else
      [:native:](o, subject);
  } else if constexpr (std::tuple_size_v<typename A::types> != 1) {
    // lv::mount rejects this at compile time; set() at run time ends up here.
    contract_assert(false && "this property takes several values and cannot follow a state");
  } else {
    watch_member<W, I, A>(o, a);
  }
}

// lv_<widget>_set_text_static for lv_<widget>_set_text, or a null reflection. The static
// one keeps the pointer and copies nothing.
consteval std::meta::info static_text_of(std::meta::info setter) {
  if (setter == std::meta::info{} || !std::meta::is_function(setter)) return {};
  const auto name = std::meta::identifier_of(setter);
  if (!name.ends_with("_set_text"sv)) return {};
  const auto f = find(str(name) + "_static");
  if (f == std::meta::info{} || std::meta::parameters_of(f).size() != 2) return {};
  return f;
}

// Calls the setter `Target`, or its static twin when the value is a constant text.
template <fixed W, std::meta::info Target, bool Constant, class... V>
void set_property(lv_obj_t* o, V... v) {
  constexpr auto kept_text = static_text_of(Target);
  if constexpr (Constant && kept_text != std::meta::info{}) {
    // LVGL shows static text uncut when the label cuts with dots, so that one is copied.
    bool cut = false;
    if constexpr (W.sv() == "label"sv) cut = lv_label_get_long_mode(o) == LV_LABEL_LONG_MODE_DOTS;
    if (cut)
      [:Target:](o, v...);
    else
      [:kept_text:](o, v...);
  } else {
    [:Target:](o, v...);
  }
}

// lv_<widget>_set_options_static for lv_<widget>_set_options, or a null reflection.
consteval std::meta::info static_options_of(std::meta::info setter) {
  if (setter == std::meta::info{} || !std::meta::is_function(setter)) return {};
  const auto name = std::meta::identifier_of(setter);
  if (!name.ends_with("_set_options"sv)) return {};
  const auto f = find(str(name) + "_static");
  if (f == std::meta::info{} || std::meta::parameters_of(f).size() != 2) return {};
  return f;
}

// The list of options of a property that takes only that: the dropdown keeps the pointer of a
// list that the compiler joined, or of any list in a constexpr tree, and copies nothing.
template <fixed W, std::meta::info Target, bool Constant>
void set_property(lv_obj_t* o, const option_list& list) {
  constexpr auto kept = static_options_of(Target);
  if constexpr (kept != std::meta::info{}) {
    if (Constant || list.kept)
      [:kept:](o, list.text);
    else
      [:Target:](o, list.text);
  } else {
    [:Target:](o, list.text);
  }
}

// ---- an enum state that follows the choice of options ---------------------------------

consteval bool same_text(const char* a, const char* b) {
  while (*a != '\0' && *a == *b) ++a, ++b;
  return *a == *b;
}

// Stops the compilation when a state of the enum named `state_enum` cannot follow the choice
// that goes with the list `options`, which `is_set`.
consteval void check_enum_choice(bool is_set, const option_list& options, const char* state_enum) {
  if (!is_set || is_null(options.enum_name))
    fail(join({"lv: .selected follows a state of ", state_enum,
               ", and the options of this widget do not come from that enum. Give the widget "
               ".options = lv::options_of<", state_enum, ">, or bind a state that holds the index."}));
  if (!same_text(options.enum_name, state_enum))
    fail(join({"lv: .selected follows a state of ", state_enum, ", and the options come from ", options.enum_name,
               ". A state follows the choice of the options of its own enum: use .options = lv::options_of<",
               state_enum, ">."}));
}

// What the observer and the event of an enum state need, in read-only memory.
struct enum_link {
  const void* source;                       // the state
  source_ops_fn ops;                        // finds its subject
  const option_list* options;
};

// The type of parameter K of a setter.
template <std::meta::info Setter, std::size_t K>
using parameter_type = typename[:std::meta::type_of(std::meta::parameters_of(Setter)[K]):];

// Shows option `index`: calls the setter of the choice with it, and a default value for
// every parameter after it (a roller's animation is off).
template <fixed W, std::size_t I>
void show_choice(lv_obj_t* o, std::uint32_t index) {
  constexpr auto setter = targets<W>[I];
  [&]<std::size_t... K>(std::index_sequence<K...>) {
    [:setter:](o, index, parameter_type<setter, K + 2>{}...);
  }(std::make_index_sequence<std::meta::parameters_of(setter).size() - 2>{});
}

// The place of the chosen option: the getter that goes with the setter of the choice.
template <fixed W, std::size_t I>
std::uint32_t chosen_index(const lv_obj_t* o) {
  constexpr auto name = std::meta::identifier_of(targets<W>[I]);
  constexpr auto getter = find(str(name.substr(0, name.find("_set_"))) + "_get_" + str(name.substr(name.find("_set_") + 5)));
  return [:getter:](o);
}

// The state changed: show the option of its enumerator. An enumerator that is not in the
// list (left out by lv::but) changes nothing.
template <fixed W, std::size_t I>
void follow_choice(lv_observer_t* observer, lv_subject_t* subject) {
  auto* target = static_cast<lv_obj_t*>(lv_observer_get_target(observer));
  const auto& link = *static_cast<const enum_link*>(lv_observer_get_user_data(observer));
  const auto index = link.options->index_of(lv_subject_get_int(subject));
  if (index < link.options->count) show_choice<W, I>(target, index);
}

// The user chose an option: the state gets its enumerator.
template <fixed W, std::size_t I>
void report_choice(lv_event_t* e) {
  auto* target = lv_event_get_current_target_obj(e);
  const auto& link = *static_cast<const enum_link*>(lv_event_get_user_data(e));
  lv_subject_t* subject = link.ops().subject(link.source);
  if (subject == nullptr) return;
  const auto index = chosen_index<W, I>(target);
  if (index < link.options->count) lv_subject_set_int(subject, link.options->values[index]);
}

// Makes the choice of member I of props<W> follow the state of the link, both ways.
template <fixed W, std::size_t I>
void bind_choice(lv_obj_t* o, const enum_link& link) {
  lv_subject_t* subject = link.ops().subject(link.source);
  if (subject == nullptr) return;  // LVGL had no memory for the subject
  auto* data = const_cast<enum_link*>(&link);
  lv_obj_add_event_cb(o, &report_choice<W, I>, LV_EVENT_VALUE_CHANGED, data);
  lv_subject_add_observer_obj(subject, &follow_choice<W, I>, o, data);
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

// The number of the member of props<W> that takes a list of options, or the number of
// members when the widget has none.
template <fixed W>
consteval std::size_t option_member_of() {
  const auto members = members_of_props<W>;
  for (std::size_t i = 0; i < members.size(); ++i) {
    const auto type = std::meta::type_of(members[i]);
    if (std::meta::has_template_arguments(type) && std::meta::template_of(type) == ^^arg &&
        std::meta::template_arguments_of(type)[0] == ^^option_list)
      return i;
  }
  return members.size();
}

template <fixed W>
inline constexpr std::size_t option_member = option_member_of<W>();

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

// ---- the properties of a run-time set ----------------------------------------

// A style given to view.set(): it points at a style that lives until the end of the call, which
// set() reads before then. A whole style in each of the 16 style slots of props<W> made the
// argument of one set about 45 KB, more stack than a microcontroller has.
template <class S>
class style_ref {
  const S* style_;  // no default member initializer: an unset slot is a null pointer, as lv::arg is zeros

 public:
  style_ref() = default;
  constexpr style_ref(const S& style) : style_(&style) {}
  constexpr bool has() const { return style_ != nullptr; }
  constexpr const S& get() const { return *style_; }
};

// The type of a style slot of the properties that set() takes: the one place that says it.
template <class D>
using style_in_set = style_ref<style_t<D>>;

template <class S>
constexpr bool is_set(const style_ref<S>& s) {
  return s.has();
}

// The members of set_props<W>: those of props<W>, with each style slot a style_in_set.
template <fixed W, class D>
consteval std::vector<std::meta::info> set_props_specs() {
  std::vector<std::meta::info> specs;
  for (auto m : members_of_props<W>) {
    const auto type = std::meta::type_of(m);
    const bool style = std::meta::dealias(type) == std::meta::dealias(^^style_t<D>);
    specs.push_back(std::meta::data_member_spec(style ? ^^style_in_set<D> : type, {.name = std::meta::identifier_of(m)}));
  }
  return specs;
}

// What view.set({...}) takes: props<W> with its styles by reference. The second parameter delays
// it like that of props_of.
template <fixed W, class D = void>
struct set_props_of {
  struct type;
  consteval { std::meta::define_aggregate(^^type, set_props_specs<W, D>()); }
};

template <fixed W, class D = void>
using set_props = typename set_props_of<W, D>::type;

template <fixed W, class D = void>
inline constexpr auto members_of_set_props = std::define_static_array(
    std::meta::nonstatic_data_members_of(^^set_props<W, D>, std::meta::access_context::unchecked()));

template <fixed W, std::size_t I, class S>
void apply_set_member(lv_obj_t* o, const style_ref<S>& s) {
  apply_member<W, I>(o, s.get());
}
template <fixed W, std::size_t I, class A>
void apply_set_member(lv_obj_t* o, const A& a) {
  apply_member<W, I>(o, a);
}

// The numbers of the members of set_props<W, D> in the order of their setters (setter_order.hpp).
template <fixed W, class D>
consteval std::vector<std::size_t> set_apply_order_of() {
  std::vector<std::size_t> numbers;
  for (std::size_t i = 0; i < members_of_set_props<W, D>.size(); ++i) numbers.push_back(i);
  return in_setter_order<W>(numbers, members_of_set_props<W, D>);
}

template <fixed W, class D>
inline constexpr std::span<const std::size_t> set_apply_order = std::define_static_array(set_apply_order_of<W, D>());

// Applies every set member of `p`, read at run time. Its members are those of props<W>, in the
// same order, so member I is applied as member I of props<W>; a widget with a rule in
// setter_order.hpp applies them in the order of its setters, as a tree does.
template <fixed W, class D>
void apply_set_props(lv_obj_t* o, const set_props<W, D>& p) {
  if constexpr (has_setter_rule<W>) {
    template for (constexpr auto i : set_apply_order<W, D>) {
      constexpr auto m = members_of_set_props<W, D>[i];
      if (is_set(p.[:m:])) apply_set_member<W, i>(o, p.[:m:]);
    }
  } else {
    template for (constexpr auto i : indices<members_of_set_props<W, D>.size()>) {
      constexpr auto m = members_of_set_props<W, D>[i];
      if (is_set(p.[:m:])) apply_set_member<W, i>(o, p.[:m:]);
    }
  }
}

}  // namespace detail

}  // namespace lv
