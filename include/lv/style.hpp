#pragma once

// A style is a struct that reflection generates from LVGL's lv_obj_set_style_*
// functions: one typed member per style property, in alphabetical order.
//
//   constexpr auto card = lv::style({.bg_color = tok.color.card, .radius = tok.radius.card});
//   lv::box({.style = card})
//
// In a tree built by lv::mount, a style becomes a const lv_style_t in read-only
// memory: no heap is used, and LVGL only stores a pointer to it.
//
// A style for one part in one state is a selector, merged in with |:
//
//   .style = card | lv::on("knob", "pressed", {.bg_color = tok.color.active})
//
// lv::on makes a constant lv_style_t too, and the style keeps it in its last member,
// `layers`: a pointer and a count, null for a style without selectors.
//
// Design values must be design tokens (see lv/tokens.hpp) or lv::raw(...).
// Set lv::config::raw_style_values (see lv/config.hpp) to accept plain values such as 8 * lv::px.

#include <lv/token_types.hpp>
#include <lv/values.hpp>

#include <array>
#include <initializer_list>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace lv {

namespace detail {

inline constexpr std::string_view style_prefix = "lv_obj_set_style_";

// Scale, skew and rotation are ratios and angles, not lengths.
consteval bool is_ratio_prop(std::string_view prop) {
  return has(prop, "scale") || has(prop, "skew") || has(prop, "rotation");
}

consteval bool has_size_suffix(std::string_view prop) {
  for (std::string_view end : {"width", "height", "_x", "_y", "radius", "_space", "spread", "_pad"})
    if (prop.ends_with(end)) return true;
  return false;
}

// Whether a style property is a length, judged by its name.
consteval bool is_sized_prop(std::string_view prop) {
  const bool sized = has_size_suffix(prop) || prop.starts_with("pad_") ||
                     prop.starts_with("margin_") || is_one_of(prop, {"x", "y", "length"});
  return sized && !is_ratio_prop(prop);
}

// The unit type of a numeric style property, or a null reflection if it has none.
consteval std::meta::info style_unit(std::string_view prop) {
  if (has(prop, "opa")) return ^^opa;
  if (has(prop, "duration")) return ^^duration;
  if (is_sized_prop(prop)) return ^^length;
  return {};
}

// The C++ type of a style value. All style setters call their parameter `value`,
// so the unit comes from the property name alone. Design values (colors, lengths,
// opacities, times, numbers and fonts) are wrapped in styled<>, which asks for a design
// token. Enums, flags and other pointers are structure, not design, and stay plain.
// An angle is decidegrees when the setter's doc comment says tenths of a degree
// (see lv/detail/angle_docs.hpp); D only delays reading the comments.
template <class D = void>
consteval std::meta::info style_value_type(std::meta::info setter, std::string_view prop,
                                           std::meta::info param) {
  const auto raw = std::meta::type_of(param);
  const auto t = std::meta::remove_cv(std::meta::dealias(raw));
  const auto design = [](std::meta::info inner) { return std::meta::substitute(^^styled, {inner}); };

  if (t == std::meta::dealias(^^lv_color_t)) return design(^^lv_color_t);
  if (t == ^^bool) return ^^strict<bool>;
  if (t == std::meta::dealias(^^const lv_font_t*)) return design(^^const lv_font_t*);  // see lv/font.hpp
  if (!std::meta::is_arithmetic_type(t)) return raw;
  if (is_angle_name(prop, {})) return design(angle_type_of<D>(setter));
  if (const auto unit = style_unit(prop); unit != std::meta::info{}) return design(unit);
  if (std::meta::is_integral_type(t)) return design(std::meta::substitute(^^strict, {t}));
  return raw;
}

// True when `id` is LV_STYLE_ followed by `prop` in upper case.
consteval bool is_style_id(std::string_view id, std::string_view prop) {
  constexpr std::string_view prefix = "LV_STYLE_";
  if (id.size() != prefix.size() + prop.size() || !id.starts_with(prefix)) return false;
  for (std::size_t i = 0; i < prop.size(); ++i) {
    const char c = prop[i];
    if (id[prefix.size() + i] != ((c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c))
      return false;
  }
  return true;
}

struct style_item {
  std::string_view name;
  std::meta::info type;    // arg<...>
  std::meta::info setter;  // lv_obj_set_style_<name>
  std::meta::info id;      // LV_STYLE_<NAME>, or null when LVGL has no such id
};

// An LV_STYLE_* enumerator with its name.
struct style_id {
  std::string_view name;
  std::meta::info id;
};

// Every LV_STYLE_* enumerator. The names are read once for a scan: the compiler
// counts the characters of a name each time it is asked for one, and asking for
// all 150 of them for each of the 139 style properties was most of the scan.
consteval std::vector<style_id> lvgl_style_ids() {
  std::vector<style_id> ids;
  for (auto e : std::meta::enumerators_of(^^_lv_style_id_t))
    ids.push_back({std::meta::identifier_of(e), e});
  return ids;
}

// The LV_STYLE_* enumerator of the style property `prop`, or a null reflection.
consteval std::meta::info style_id_of(std::string_view prop, const std::vector<style_id>& ids) {
  for (const auto& id : ids)
    if (is_style_id(id.name, prop)) return id.id;
  return {};
}

// The member of lv::style for the setter `f`, called lv_obj_set_style_<prop>.
template <class D>
consteval style_item style_item_of(std::meta::info f, std::string_view prop,
                                   const std::vector<style_id>& ids) {
  const auto params = std::meta::parameters_of(f);  // obj, value(s), selector
  std::vector<std::meta::info> types;
  for (std::size_t i = 1; i + 1 < params.size(); ++i)
    types.push_back(style_value_type<D>(f, prop, params[i]));
  // Only a setter with one value can have a style id.
  const auto id = params.size() == 3 ? style_id_of(prop, ids) : std::meta::info{};
  return {prop, std::meta::substitute(^^arg, types), f, id};
}

// The functions below are templates on D only so that nothing runs until a style
// is really used: the scan costs about a second of compile time.
template <class D>
consteval std::vector<style_item> scan_style(const std::vector<style_id>& ids) {
  std::vector<style_item> items;
  for (auto f : api) {
    const auto name = std::meta::identifier_of(f);
    if (!name.starts_with(style_prefix) || std::meta::is_vararg_function(f)) continue;
    if (std::meta::parameters_of(f).size() < 3) continue;
    items.push_back(style_item_of<D>(f, name.substr(style_prefix.size()), ids));
  }
  sort_by_name(items);
  return items;
}

// The scan runs once. Its result is stored as triples: type, setter, id. It comes in
// as an argument: see scan_base in props.hpp for why.
consteval std::vector<std::meta::info> pack_style(const std::vector<style_item>& items) {
  std::vector<std::meta::info> out;
  for (const auto& it : items) {
    out.push_back(it.type);
    out.push_back(it.setter);
    out.push_back(it.id);
  }
  return out;
}

template <class D = void>
inline constexpr std::span<const std::meta::info> style_table =
    std::define_static_array(pack_style(scan_style<D>(lvgl_style_ids())));

// One style for one part in one state, made by lv::on: a constant lv_style_t and the
// selector to add it with.
struct style_layer {
  lv_style_selector_t selector;
  const lv_style_t* style;
};

// The last member of a style: the layers that lv::on added to it, in read-only memory.
// It takes the place of made_with_braces and has no default constructor either, so a
// style is made with braces. A style without layers holds a null pointer and a zero.
struct style_layers {
  const style_layer* first;
  std::size_t count;  // a number, not a look at the pointer: see theme_styles

  constexpr style_layers(std::initializer_list<int>) : first(nullptr), count(0) {}
  constexpr explicit style_layers(std::span<const style_layer> list) : first(list.data()), count(list.size()) {}

  constexpr std::span<const style_layer> list() const { return {first, count}; }
};

template <class D>
consteval std::vector<std::meta::info> style_specs() {
  std::vector<std::meta::info> out;
  for (std::size_t i = 0; i < style_table<D>.size(); i += 3)
    out.push_back(std::meta::data_member_spec(
        style_table<D>[i],
        {.name = str(std::meta::identifier_of(style_table<D>[i + 1]).substr(style_prefix.size()))}));
  out.push_back(std::meta::data_member_spec(^^style_layers, {.name = "layers"}));
  return out;
}

template <class D>
consteval std::vector<std::meta::info> style_column(std::size_t column) {
  std::vector<std::meta::info> out;
  for (std::size_t i = column; i < style_table<D>.size(); i += 3)
    out.push_back(style_table<D>[i]);
  return out;
}

template <class D = void>
struct style_of {
  struct type;
  consteval { std::meta::define_aggregate(^^type, style_specs<D>()); }
};

// lv::style, named so that it is generated on first use.
template <class D = void>
using style_t = typename style_of<D>::type;

template <class S, class D = void>
concept a_style = std::same_as<S, style_t<D>>;

template <class D = void>
inline constexpr auto style_members = std::define_static_array(members_before_braces(^^style_t<D>));

// For each member of lv::style, in member order: its setter and its LV_STYLE_* id.
template <class D = void>
inline constexpr std::span<const std::meta::info> style_setters =
    std::define_static_array(style_column<D>(1));
template <class D = void>
inline constexpr std::span<const std::meta::info> style_ids =
    std::define_static_array(style_column<D>(2));

// The font of a font property that follows an lv::scaled_font. Defined in
// lv/scaled_font.hpp, which a program that binds a font has included.
const lv_font_t* bound_font(const void* source);

// Calls a style setter with the value of one member: what it holds, or the font it follows.
template <class A, class Setter>
void set_style_value(lv_obj_t* o, const A& a, Setter setter, lv_style_selector_t selector) {
  if constexpr (std::same_as<A, arg<styled<const lv_font_t*>>>) {
    if (a.bound != nullptr) return setter(o, bound_font(a.bound), selector);
  }
  const auto& [... x] = a.v;
  setter(o, lower(x)..., selector);
}

// True when at least one property of the style is set, not counting its layers.
template <class S, class D = void>
  requires a_style<S, D>
constexpr bool sets_a_value(const S& s) {
  bool any = false;
  template for (constexpr auto m : style_members<D>) any = any || s.[:m:].set;
  return any;
}

// True when the style sets a property or has a layer.
template <class S, class D = void>
  requires a_style<S, D>
constexpr bool is_set(const S& s) {
  return s.layers.count != 0 || sets_a_value<S, D>(s);
}

// Adds the layers of a style. A layer is already on the widget when the same style is
// applied again, so it is taken off first: the widget's list of styles does not grow.
inline void add_layers(lv_obj_t* o, style_layers layers) {
  for (const style_layer& l : layers.list()) {
    lv_obj_remove_style(o, l.style, l.selector);
    lv_obj_add_style(o, l.style, l.selector);
  }
}

// Applies a style with LVGL's local style setters. Used at run time. A layer names its
// own part and state, so only the style of the main part in the default state has any.
template <class S, class D = void>
  requires a_style<S, D>
void apply_style_local(lv_obj_t* o, const S& s, lv_style_selector_t selector)
  pre(s.layers.count == 0 || selector == LV_PART_MAIN)
{
  template for (constexpr auto i : indices<style_members<D>.size()>) {
    constexpr auto m = style_members<D>[i];
    if (s.[:m:].set) set_style_value(o, s.[:m:], [:style_setters<D>[i]:], selector);
  }
  add_layers(o, s.layers);
}

// The constant style that `Where::get()` returns is described by a type, not by a
// reference template argument. GCC gives every function made for a sub-object of a
// variable the same symbol name in every file (`ui.kids.head`), so two files with a
// tree of the same name would share code. A type built from the root variable and an
// index path is unique.

// The numbers of the members of lv::style that `s` sets.
template <class S, class D = void>
  requires a_style<S, D>
consteval std::vector<std::size_t> set_style_members(const S& s) {
  std::vector<std::size_t> out;
  template for (constexpr auto i : indices<style_members<D>.size()>) {
    if (s.[:style_members<D>[i]:].set) out.push_back(i);
  }
  return out;
}

// The same for the constant style `Where::get()`. They are found in one constant
// evaluation and the loops below visit only these: a question to the compiler for
// each of the 139 members of each style of each node is what made a large tree slow.
template <class Where, class D = void>
inline constexpr std::span<const std::size_t> set_in =
    std::define_static_array(set_style_members<std::remove_cvref_t<decltype(Where::get())>, D>(Where::get()));

// Whether member i of the constant style can live in a const lv_style_t: LVGL has an
// id for it, and it holds a value. A font that is drawn on the device is not a constant.
template <class Where, std::size_t I, class D = void>
inline constexpr bool is_const_member =
    style_ids<D>[I] != std::meta::info{} && Where::get().[:style_members<D>[I]:].bound == nullptr;

// How many set properties of a constant style can live in a const lv_style_t.
template <class Where, class D = void>
inline constexpr std::size_t const_count = [] {
  std::size_t n = 0;
  template for (constexpr auto i : set_in<Where, D>) {
    if constexpr (is_const_member<Where, i, D>) ++n;
  }
  return n;
}();

constexpr lv_style_value_t style_value(lv_color_t c) {
  lv_style_value_t v{};
  v.color = c;
  return v;
}
template <class T>
  requires std::is_pointer_v<T>
constexpr lv_style_value_t style_value(T p) {
  lv_style_value_t v{};
  v.ptr = p;
  return v;
}
template <class T>
  requires(!std::is_pointer_v<T>)
constexpr lv_style_value_t style_value(T n) {
  lv_style_value_t v{};
  v.num = static_cast<std::int32_t>(n);
  return v;
}

// The property array of a const lv_style_t, ended by LV_STYLE_PROP_INV.
template <class Where, class D = void>
consteval std::array<lv_style_const_prop_t, const_count<Where, D> + 1> const_props() {
  std::array<lv_style_const_prop_t, const_count<Where, D> + 1> out{};
  std::size_t k = 0;
  template for (constexpr auto i : set_in<Where, D>) {
    if constexpr (is_const_member<Where, i, D>) {
      out[k].prop = static_cast<lv_style_prop_t>([:style_ids<D>[i]:]);
      out[k].value = style_value(Where::get().[:style_members<D>[i]:].v.v0);
      ++k;
    }
  }
  out[k].prop = LV_STYLE_PROP_INV;
  return out;
}

// What LVGL's LV_STYLE_CONST_INIT macro expands to, without its C cast.
constexpr lv_style_t const_style(const lv_style_const_prop_t* props) {
  lv_style_t s{};
  // lv_style_t has a sentinel member only when LVGL is built with style assertions.
  [](auto& style) {
    if constexpr (requires { style.sentinel; }) style.sentinel = LV_STYLE_SENTINEL_VALUE;
  }(s);
  s.values_and_props = const_cast<void*>(static_cast<const void*>(props));
  s.has_group = 0xFFFFFFFF;
  s.prop_cnt = 255;
  return s;
}

// The constant style of a node: one object for every node whose style has these values.
template <class Where, class D = void>
consteval const lv_style_t* shared_const_style() {
  // Equal values give one object, so two nodes with the same style share their style.
  const auto* props = std::define_static_array(const_props<Where, D>()).data();
  return std::define_static_object(const_style(props));
}

// Adds a constant style to a widget. What LVGL can keep in a const style goes to
// read-only memory; the few shorthands without a style id (pad_all, size, ...) and a
// font that follows an lv::scaled_font are set as local styles. The layers that
// lv::on made follow, each with its own selector.
template <class Where, lv_style_selector_t Selector, class D = void>
void add_const_style(lv_obj_t* o) {
  if constexpr (const_count<Where, D> > 0) {
    constexpr const lv_style_t* st = shared_const_style<Where, D>();
    lv_obj_add_style(o, st, Selector);
  }
  template for (constexpr auto i : set_in<Where, D>) {
    if constexpr (!is_const_member<Where, i, D>) {
      constexpr auto a = Where::get().[:style_members<D>[i]:];
      set_style_value(o, a, [:style_setters<D>[i]:], Selector);
    }
  }
  if constexpr (Where::get().layers.count != 0) {
    static_assert(Selector == LV_PART_MAIN,
                  "lv: lv::on goes in .style, because it names its own part and state. Write "
                  ".style = base | lv::on(\"knob\", \"pressed\", {...}) instead of .style_knob = lv::on(...).");
    for (const style_layer& l : Where::get().layers.list()) lv_obj_add_style(o, l.style, l.selector);
  }
}

// ---- constant styles worked out whole: for a theme and for lv::on ----------------

// A style property that sets several others. A constant style holds only the others.
struct shorthand {
  std::string_view name;
  std::array<std::string_view, 4> sets;  // an empty name ends the list
};

inline constexpr shorthand shorthands[] = {
    {"margin_all", {"margin_top", "margin_bottom", "margin_left", "margin_right"}},
    {"margin_hor", {"margin_left", "margin_right"}},
    {"margin_ver", {"margin_top", "margin_bottom"}},
    {"pad_all", {"pad_top", "pad_bottom", "pad_left", "pad_right"}},
    {"pad_gap", {"pad_row", "pad_column"}},
    {"pad_hor", {"pad_left", "pad_right"}},
    {"pad_ver", {"pad_top", "pad_bottom"}},
    {"size", {"width", "height"}},
    {"transform_scale", {"transform_scale_x", "transform_scale_y"}},
};

// The name of member `i` of lv::style.
template <class D>
consteval std::string_view style_name(std::size_t i) {
  return std::meta::identifier_of(style_setters<D>[i]).substr(style_prefix.size());
}

// The style id of the property called `name`.
template <class D>
consteval lv_style_prop_t style_prop_named(std::string_view name) {
  for (std::size_t i = 0; i < style_ids<D>.size(); ++i)
    if (style_ids<D>[i] != std::meta::info{} && style_name<D>(i) == name)
      return static_cast<lv_style_prop_t>(std::meta::extract<_lv_style_id_t>(style_ids<D>[i]));
  fail("lv: LVGL has no style property '" + str(name) + "'.");
}

// The style ids that the shorthand `name` sets. A member of lv::style that is neither
// a value nor a shorthand (`enabled`, which turns another style on and off) is refused.
template <class D>
consteval std::vector<lv_style_prop_t> long_forms(std::string_view name) {
  std::vector<lv_style_prop_t> out;
  for (const auto& s : shorthands) {
    if (s.name != name) continue;
    for (auto target : s.sets)
      if (!target.empty()) out.push_back(style_prop_named<D>(target));
    return out;
  }
  fail("lv: a theme or an lv::on cannot hold the style property '" + str(name) +
       "': LVGL keeps no value for it in a style. Leave it out, or set it in the node's own style.");
}

consteval bool holds(const std::vector<lv_style_const_prop_t>& props, lv_style_prop_t prop) {
  for (const auto& p : props)
    if (p.prop == prop) return true;
  return false;
}

// Adds what a shorthand stands for. A long form that the style sets itself wins.
consteval void add_long_forms(std::vector<lv_style_const_prop_t>& props, const std::vector<lv_style_prop_t>& ids,
                              std::span<const lv_style_value_t> values) {
  for (std::size_t k = 0; k < ids.size(); ++k) {
    if (holds(props, ids[k])) continue;
    lv_style_const_prop_t p{};
    p.prop = ids[k];
    p.value = values[k < values.size() ? k : 0];  // `size` has a value for each; the others have one
    props.push_back(p);
  }
}

// The properties of `s` that have a style id.
template <class S, class D>
consteval std::vector<lv_style_const_prop_t> plain_props(const S& s) {
  std::vector<lv_style_const_prop_t> props;
  template for (constexpr auto i : indices<style_members<D>.size()>) {
    if constexpr (style_ids<D>[i] != std::meta::info{}) {
      if (s.[:style_members<D>[i]:].set) {
        if (s.[:style_members<D>[i]:].bound != nullptr)
          fail("lv: a theme and an lv::on hold constants, and a font bound with lv::bind is drawn on the device. "
               "Give them a font made by the compiler (lv::typeface), or bind the font in the widget's own style.");
        lv_style_const_prop_t p{};
        p.prop = static_cast<lv_style_prop_t>([:style_ids<D>[i]:]);
        p.value = style_value(s.[:style_members<D>[i]:].v.v0);
        props.push_back(p);
      }
    }
  }
  return props;
}

// Adds the properties of `s` that have no style id, as the ones they stand for.
template <class S, class D>
consteval void add_shorthands(std::vector<lv_style_const_prop_t>& props, const S& s) {
  template for (constexpr auto i : indices<style_members<D>.size()>) {
    if constexpr (style_ids<D>[i] == std::meta::info{}) {
      if (s.[:style_members<D>[i]:].set) {
        const auto& [... x] = s.[:style_members<D>[i]:].v;
        const std::array values{style_value(lower(x))...};
        add_long_forms(props, long_forms<D>(style_name<D>(i)), values);
      }
    }
  }
}

// `s` as a constant lv_style_t with static storage: what LV_STYLE_CONST_INIT makes.
// Equal values give one object.
template <class S, class D>
consteval const lv_style_t* stored_style(const S& s) {
  auto props = plain_props<S, D>(s);
  add_shorthands<S, D>(props, s);
  lv_style_const_prop_t end{};
  end.prop = LV_STYLE_PROP_INV;
  props.push_back(end);
  const std::vector<lv_style_t> one{const_style(std::define_static_array(props).data())};
  return std::define_static_array(one).data();
}

// ---- parts and states by name, for lv::on ------------------------------------

// The value of the enumerator of E that user code names `text`, or nothing. `any`
// selects nothing a style could be added to, so it is not a name here.
template <class E>
consteval std::optional<lv_style_selector_t> selector_named(std::string_view text) {
  if (text == "any") return std::nullopt;
  for (const enum_name& e : enum_names<E>)
    if (short_enum_name<E>(e) == text) return static_cast<lv_style_selector_t>(e.value);
  return std::nullopt;
}

// "main, scrollbar, indicator, ...": the names of E, without `any`.
template <class E>
consteval std::string selector_names() {
  std::string out;
  for (const enum_name& e : enum_names<E>) {
    const std::string name = short_enum_name<E>(e);
    if (name == "any") continue;
    if (!out.empty()) out += ", ";
    out += name;
  }
  return out;
}

template <class E>
consteval std::string quoted(std::string_view text) { return "'" + str(text) + "'"; }

// The enums are template parameters only so that nothing is scanned until a name is read.
template <class Part = lv_part_t, class State = lv_state_t>
consteval lv_style_selector_t part_named(std::string_view text) {
  if (const auto part = selector_named<Part>(text)) return *part;
  if (selector_named<State>(text))
    fail("lv: " + quoted<Part>(text) + " is a state. lv::on takes the part first, then the state: lv::on(\"knob\", \"" +
         str(text) + "\", {...}).");
  fail("lv: widgets have no part " + quoted<Part>(text) + ". The parts: " + selector_names<Part>() + ".");
}

template <class Part = lv_part_t, class State = lv_state_t>
consteval lv_style_selector_t state_named(std::string_view text) {
  if (const auto state = selector_named<State>(text)) return *state;
  if (selector_named<Part>(text))
    fail("lv: " + quoted<Part>(text) + " is a part. lv::on takes the part first, then the state: lv::on(\"" + str(text) +
         "\", \"pressed\", {...}).");
  fail("lv: widgets have no state " + quoted<Part>(text) + ". The states: " + selector_names<State>() + ".");
}

template <class Part = lv_part_t, class State = lv_state_t>
consteval lv_style_selector_t part_or_state_named(std::string_view text) {
  if (const auto part = selector_named<Part>(text)) return *part;
  if (const auto state = selector_named<State>(text)) return *state;
  fail("lv: " + quoted<Part>(text) + " is neither a part nor a state. The parts: " + selector_names<Part>() +
       ". The states: " + selector_names<State>() + ".");
}

// The first name of lv::on("knob", "pressed", ...): a part of a widget, as LVGL names it.
struct part_name {
  lv_style_selector_t value;
  template <std::size_t N>
  consteval part_name(const char (&text)[N]) : value(part_named<>(text)) {}
};

// The second name: a state.
struct state_name {
  lv_style_selector_t value;
  template <std::size_t N>
  consteval state_name(const char (&text)[N]) : value(state_named<>(text)) {}
};

// The one name of lv::on("knob", ...) or lv::on("pressed", ...).
struct part_or_state {
  lv_style_selector_t value;
  template <std::size_t N>
  consteval part_or_state(const char (&text)[N]) : value(part_or_state_named<>(text)) {}
};

// A style that holds nothing but one layer: `s` for `selector`.
template <class D>
consteval style_t<D> layer_of(lv_style_selector_t selector, const style_t<D>& s) {
  if (s.layers.count != 0)
    fail("lv: an lv::on cannot hold another lv::on. Put them side by side: "
         "base | lv::on(\"knob\", {...}) | lv::on(\"knob\", \"pressed\", {...}).");
  const std::array one{style_layer{selector, stored_style<style_t<D>, D>(s)}};
  style_t<D> out{};
  out.layers = style_layers(std::define_static_array(one));
  return out;
}

// The layers of `a`, then those of `b`, made by the compiler.
consteval style_layers joined_while_compiling(style_layers a, style_layers b) {
  std::vector<style_layer> all(a.list().begin(), a.list().end());
  all.insert(all.end(), b.list().begin(), b.list().end());
  return style_layers(std::define_static_array(all));
}

// The layers of `a`, then those of `b`. Layers are made by the compiler, so two lists
// are joined while compiling. At run time one of them must be empty: two styles with
// lv::on layers are merged with | while compiling only.
template <class D = void>
constexpr style_layers joined(style_layers a, style_layers b) {
  if (a.count == 0) return b;
  if (b.count == 0) return a;
  if consteval {
    return joined_while_compiling(a, b);
  }
  contract_assert(false);
  return b;
}

}  // namespace detail

namespace detail {

template <class T>
constexpr token<T> part(T v) {
  return {v, from_design_tokens{}};
}

}  // namespace detail

// Composite tokens are merged into a style with |:
//   lv::style({.bg_color = tok.color.card}) | tok.elevation.card | tok.border.card

template <class S, class D = void>
  requires detail::a_style<S, D>
constexpr S operator|(S s, const token<shadow_token>& t) {
  s.shadow_color = detail::part(t->color);
  s.shadow_offset_x = detail::part(t->offset_x);
  s.shadow_offset_y = detail::part(t->offset_y);
  s.shadow_width = detail::part(t->blur);
  s.shadow_spread = detail::part(t->spread);
  return s;
}

template <class S, class D = void>
  requires detail::a_style<S, D>
constexpr S operator|(S s, const token<border_token>& t) {
  s.border_color = detail::part(t->color);
  s.border_width = detail::part(t->width);
  return s;
}

// Only the letter spacing: choosing an lv_font_t from a family, size and weight
// needs the font assets, which are not part of the framework yet.
template <class S, class D = void>
  requires detail::a_style<S, D>
constexpr S operator|(S s, const token<typography_token>& t) {
  s.text_letter_space = detail::part(t->letter_spacing);
  return s;
}

template <class S, class D = void>
  requires detail::a_style<S, D>
constexpr S operator|(S s, const token<transition_token>& t) {
  s.anim_duration = detail::part(t->duration);
  return s;
}

namespace detail {

// Two styles merge with |: what the right one sets wins.
//   .style = button | danger
// It lives in this namespace because the generated style type does, and that is
// where the compiler looks for operators on it.
// The layers of both are kept, those of the right one last.
template <class S, class D = void>
  requires a_style<S, D>
constexpr S operator|(S base, const S& over) {
  template for (constexpr auto m : style_members<D>) {
    if (over.[:m:].set) base.[:m:] = over.[:m:];
  }
  base.layers = joined<D>(base.layers, over.layers);
  return base;
}

}  // namespace detail

// A style value: lv::style({.bg_color = tok.color.card, .radius = tok.radius.card}).
// Keep it in a constexpr variable to give it a name and reuse it.
template <class D = void>
constexpr detail::style_t<D> style(detail::style_t<D> s) {
  return s;
}

// A style for one part in one state, merged into a style with |:
//   .style = base | lv::on("knob", "pressed", {.bg_color = tok.color.active})
// The names are LVGL's parts (main, indicator, knob, items, cursor, ...) and states
// (pressed, checked, focused, disabled, ...). With one name it is a part in the default
// state, or a state of the main part: lv::on("pressed", {...}) is what .style_pressed does.
// The compiler makes a constant lv_style_t of it, so it costs no heap; the widget adds
// it with LVGL's selector for the part and the state.
template <class D = void>
consteval detail::style_t<D> on(detail::part_name part, detail::state_name state, const detail::style_t<D>& s) {
  return detail::layer_of<D>(part.value | state.value, s);
}

template <class D = void>
consteval detail::style_t<D> on(detail::part_or_state name, const detail::style_t<D>& s) {
  return detail::layer_of<D>(name.value, s);
}

}  // namespace lv
