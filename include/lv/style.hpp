#pragma once

// A style is a struct that reflection generates from LVGL's lv_obj_set_style_*
// functions: one typed member per style property, in alphabetical order.
//
//   constexpr auto card = lv::style({.bg_color = 0x1b2430_rgb, .pad_all = 12_px, .radius = 8_px});
//   lv::box({.style = card})
//
// In a tree built by lv::mount, a style becomes a const lv_style_t in read-only
// memory: no heap is used, and LVGL only stores a pointer to it.

#include <lv/values.hpp>

#include <array>
#include <span>
#include <string_view>
#include <vector>

namespace lv {

namespace detail {

inline constexpr std::string_view style_prefix = "lv_obj_set_style_";

// The C++ type of a style value. All style setters call their parameter `value`,
// so the unit comes from the property name alone.
consteval std::meta::info style_value_type(std::string_view prop, std::meta::info param) {
  const auto raw = std::meta::type_of(param);
  const auto t = std::meta::remove_cv(std::meta::dealias(raw));

  if (std::meta::is_arithmetic_type(t) && t != ^^bool) {
    if (has(prop, "opa")) return ^^opa;
    if (has(prop, "duration")) return ^^duration;
    // Scale and skew are ratios and tenths of a degree, not lengths.
    const bool ratio = has(prop, "scale") || has(prop, "skew") || has(prop, "rotation");
    const bool sized = prop.ends_with("width") || prop.ends_with("height") ||
                       prop.ends_with("_x") || prop.ends_with("_y") || prop.ends_with("radius") ||
                       prop.ends_with("_space") || prop.ends_with("spread") ||
                       prop.ends_with("_pad") || prop.starts_with("pad_") ||
                       prop.starts_with("margin_") || is_one_of(prop, {"x", "y", "length"});
    if (sized && !ratio) return ^^length;
  }
  if (std::meta::is_integral_type(t)) return std::meta::substitute(^^strict, {t});
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

// The functions below are templates on D only so that nothing runs until a style
// is really used: the scan costs about a second of compile time.
template <class D>
consteval std::vector<style_item> scan_style() {
  struct known_id {
    std::string_view name;
    std::meta::info e;
  };
  std::vector<known_id> ids;
  for (auto e : std::meta::enumerators_of(^^_lv_style_id_t))
    ids.push_back({std::meta::identifier_of(e), e});

  std::vector<style_item> items;
  for (auto f : api) {
    const auto name = std::meta::identifier_of(f);
    if (!name.starts_with(style_prefix) || std::meta::is_vararg_function(f)) continue;
    const auto prop = name.substr(style_prefix.size());
    const auto params = std::meta::parameters_of(f);
    if (params.size() < 3) continue;  // obj, value(s), selector

    std::vector<std::meta::info> types;
    for (std::size_t i = 1; i + 1 < params.size(); ++i)
      types.push_back(style_value_type(prop, params[i]));

    std::meta::info id{};
    if (params.size() == 3)
      for (const auto& k : ids)
        if (is_style_id(k.name, prop)) id = k.e;
    items.push_back({prop, std::meta::substitute(^^arg, types), f, id});
  }
  for (std::size_t i = 1; i < items.size(); ++i)
    for (std::size_t j = i; j > 0 && items[j].name < items[j - 1].name; --j)
      std::swap(items[j], items[j - 1]);
  return items;
}

// The scan runs once. Its result is stored as triples: type, setter, id.
template <class D>
consteval std::vector<std::meta::info> pack_style() {
  std::vector<std::meta::info> out;
  for (const auto& it : scan_style<D>()) {
    out.push_back(it.type);
    out.push_back(it.setter);
    out.push_back(it.id);
  }
  return out;
}

template <class D = void>
inline constexpr std::span<const std::meta::info> style_table =
    std::define_static_array(pack_style<D>());

template <class D>
consteval std::vector<std::meta::info> style_specs() {
  std::vector<std::meta::info> out;
  for (std::size_t i = 0; i < style_table<D>.size(); i += 3)
    out.push_back(std::meta::data_member_spec(
        style_table<D>[i],
        {.name = str(std::meta::identifier_of(style_table<D>[i + 1]).substr(style_prefix.size()))}));
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
inline constexpr auto style_members = std::define_static_array(
    std::meta::nonstatic_data_members_of(^^style_t<D>, std::meta::access_context::unchecked()));

// For each member of lv::style, in member order: its setter and its LV_STYLE_* id.
template <class D = void>
inline constexpr std::span<const std::meta::info> style_setters =
    std::define_static_array(style_column<D>(1));
template <class D = void>
inline constexpr std::span<const std::meta::info> style_ids =
    std::define_static_array(style_column<D>(2));

// True when at least one property of the style is set.
template <class S, class D = void>
  requires a_style<S, D>
constexpr bool is_set(const S& s) {
  bool any = false;
  template for (constexpr auto m : style_members<D>) any = any || s.[:m:].set;
  return any;
}

// Applies a style with LVGL's local style setters. Used at run time.
template <class S, class D = void>
  requires a_style<S, D>
void apply_style_local(lv_obj_t* o, const S& s, lv_style_selector_t selector) {
  template for (constexpr auto i : indices<style_members<D>.size()>) {
    constexpr auto m = style_members<D>[i];
    if (s.[:m:].set) {
      const auto& [... x] = s.[:m:].v;
      [:style_setters<D>[i]:](o, lower(x)..., selector);
    }
  }
}

// How many set properties of the constant style S can live in a const lv_style_t.
template <const auto& S, class D = void>
inline constexpr std::size_t const_count = [] {
  std::size_t n = 0;
  template for (constexpr auto i : indices<style_members<D>.size()>) {
    constexpr auto m = style_members<D>[i];
    if (S.[:m:].set && style_ids<D>[i] != std::meta::info{}) ++n;
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
template <const auto& S, class D = void>
constexpr std::array<lv_style_const_prop_t, const_count<S, D> + 1> const_props() {
  std::array<lv_style_const_prop_t, const_count<S, D> + 1> out{};
  std::size_t k = 0;
  template for (constexpr auto i : indices<style_members<D>.size()>) {
    constexpr auto m = style_members<D>[i];
    if constexpr (S.[:m:].set && style_ids<D>[i] != std::meta::info{}) {
      out[k].prop = static_cast<lv_style_prop_t>([:style_ids<D>[i]:]);
      out[k].value = style_value(lower(std::get<0>(S.[:m:].v)));
      ++k;
    }
  }
  out[k].prop = LV_STYLE_PROP_INV;
  return out;
}

// What LVGL's LV_STYLE_CONST_INIT macro expands to, without its C cast.
constexpr lv_style_t const_style(const lv_style_const_prop_t* props) {
  lv_style_t s{};
#if LV_USE_ASSERT_STYLE
  s.sentinel = LV_STYLE_SENTINEL_VALUE;
#endif
  s.values_and_props = const_cast<void*>(static_cast<const void*>(props));
  s.has_group = 0xFFFFFFFF;
  s.prop_cnt = 255;
  return s;
}

// Adds the constant style S to a widget. What LVGL can keep in a const style goes
// to read-only memory; the few shorthands without a style id (pad_all, size, ...)
// are set as local styles.
template <const auto& S, class D = void>
void add_const_style(lv_obj_t* o, lv_style_selector_t selector) {
  if constexpr (const_count<S, D> > 0) {
    static constexpr auto props = const_props<S, D>();
    static constexpr lv_style_t st = const_style(props.data());
    lv_obj_add_style(o, &st, selector);
  }
  template for (constexpr auto i : indices<style_members<D>.size()>) {
    constexpr auto m = style_members<D>[i];
    if constexpr (S.[:m:].set && style_ids<D>[i] == std::meta::info{}) {
      constexpr auto a = S.[:m:];
      const auto& [... x] = a.v;
      [:style_setters<D>[i]:](o, lower(x)..., selector);
    }
  }
}

}  // namespace detail

// A style value: lv::style({.bg_color = 0x1b2430_rgb, .radius = 8_px}).
// Keep it in a constexpr variable to give it a name and reuse it.
template <class D = void>
constexpr detail::style_t<D> style(detail::style_t<D> s) {
  return s;
}

}  // namespace lv
