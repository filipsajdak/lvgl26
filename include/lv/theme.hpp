#pragma once

// A theme: the styles that every widget of a kind starts with.
//
//   constexpr auto mine = lv::theme({
//       .screen = {.style = {.bg_color = tok.color.ground}},
//       .arc = {.style = {.arc_width = tok.stroke.track}, .style_indicator = {.arc_color = tok.color.active}},
//       .button = {.style = {.bg_color = tok.color.active}, .style_pressed = {.bg_color = tok.color.raised}},
//   });
//
//   lv::display display{app, frame, mine};
//
// There is one member for each LVGL widget, named as LVGL names it (`arc`, `button`,
// `obj` for a plain box), and `screen` for a widget without a parent. They are in
// alphabetical order, as the members of props are. LVGL's `switch`
// is `toggle`, as in lv::toggle: `switch` is a C++ keyword. A member holds what a
// node of a tree holds: `style`, `style_pressed`, `style_indicator` and so on.
//
// A theme is worked out by the compiler and lives in read-only memory: it costs no
// heap. It is applied after LVGL's own theme, so it only has to say what differs. A
// widget still overrides it with its own `.style`.
//
// A widget gets the styles of its own kind only: a slider does not get those of `bar`.

#include <lv/detail/theme_styles.hpp>
#include <lv/props.hpp>
#include <lv/style.hpp>

#include <array>
#include <span>
#include <string_view>
#include <vector>

namespace lv {

namespace detail {

// A style member of a theme's widget: its name and what selects it.
struct look_item {
  std::string_view name;
  std::meta::info target;  // as in member_item
};

consteval std::vector<look_item> look_items() {
  std::vector<look_item> items;
  for (const auto& slot : named_style_slots()) items.push_back({slot.name, slot.target});
  return items;
}

consteval std::vector<std::meta::info> look_specs(std::meta::info style) {
  std::vector<std::meta::info> out;
  for (const auto& item : look_items())
    out.push_back(std::meta::data_member_spec(style, {.name = str(item.name)}));
  return out;
}

consteval std::vector<std::meta::info> look_targets() {
  std::vector<std::meta::info> out;
  for (const auto& item : look_items()) out.push_back(item.target);
  return out;
}

// What a theme says about one kind of widget.
template <class D = void>
struct look_of {
  struct type;
  consteval { std::meta::define_aggregate(^^type, look_specs(^^style_t<D>)); }
};

template <class D = void>
inline constexpr auto look_members = std::define_static_array(
    std::meta::nonstatic_data_members_of(^^typename look_of<D>::type, std::meta::access_context::unchecked()));

template <class D = void>
inline constexpr std::span<const std::meta::info> look_selectors = std::define_static_array(look_targets());

// "lv_arc_class" -> "arc"; empty when `variable` is not the class of a widget.
consteval std::string_view widget_of_class(std::meta::info variable) {
  constexpr std::string_view prefix = "lv_", suffix = "_class";
  if (!std::meta::is_variable(variable) || !std::meta::has_identifier(variable)) return {};
  const auto name = std::meta::identifier_of(variable);
  if (!name.starts_with(prefix) || !name.ends_with(suffix)) return {};
  if (std::meta::dealias(std::meta::type_of(variable)) != std::meta::dealias(^^const lv_obj_class_t)) return {};
  return name.substr(prefix.size(), name.size() - prefix.size() - suffix.size());
}

// A member of a theme: its name and the class of its widgets.
struct theme_item {
  std::string name;
  std::meta::info widget;  // null for `screen`
};

// The member of a theme for a widget. `switch` is a C++ keyword.
consteval std::string theme_member(std::string_view widget) {
  return widget == "switch" ? "toggle" : str(widget);
}

// `screen` and a member for the class of each LVGL widget, in alphabetical order:
// the order designated initializers must follow.
consteval std::vector<theme_item> theme_items() {
  std::vector<theme_item> items{{"screen", std::meta::info{}}};
  for (auto m : std::meta::members_of(^^::, std::meta::access_context::unchecked()))
    if (const auto widget = widget_of_class(m); !widget.empty()) items.push_back({theme_member(widget), m});
  sort_by_name(items);
  return items;
}

// D only delays the scan.
template <class D>
consteval std::vector<std::meta::info> theme_specs() {
  std::vector<std::meta::info> out;
  for (const auto& item : theme_items())
    out.push_back(std::meta::data_member_spec(^^typename look_of<D>::type, {.name = item.name}));
  return out;
}

consteval std::vector<std::meta::info> theme_widgets() {
  std::vector<std::meta::info> out;
  for (const auto& item : theme_items()) out.push_back(item.widget);
  return out;
}

template <class D = void>
struct theme_of {
  struct type;
  consteval { std::meta::define_aggregate(^^type, theme_specs<D>()); }
};

template <class D = void>
using theme_spec = typename theme_of<D>::type;

template <class D = void>
inline constexpr auto theme_members = std::define_static_array(
    std::meta::nonstatic_data_members_of(^^theme_spec<D>, std::meta::access_context::unchecked()));

// For each member of a theme: the class of its widgets, or null for `screen`.
template <class D = void>
inline constexpr std::span<const std::meta::info> theme_classes = std::define_static_array(theme_widgets());

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
  fail("lv: a theme cannot hold the style property '" + str(name) +
       "': LVGL keeps no value for it in a style. Leave it out of the theme.");
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

// The styles that `look` sets, for the widgets of `widget`.
template <class D>
consteval void add_look(std::vector<theme_entry>& out, const lv_obj_class_t* widget,
                        const typename look_of<D>::type& look) {
  template for (constexpr auto j : indices<look_members<D>.size()>) {
    const auto& s = look.[:look_members<D>[j]:];
    if (is_set<style_t<D>, D>(s))
      out.push_back({widget, selector_of<look_selectors<D>[j]>(), stored_style<style_t<D>, D>(s)});
  }
}

template <class D>
consteval std::span<const theme_entry> theme_entries(const theme_spec<D>& spec) {
  std::vector<theme_entry> out;
  template for (constexpr auto i : indices<theme_members<D>.size()>) {
    const lv_obj_class_t* widget = nullptr;
    if constexpr (theme_classes<D>[i] != std::meta::info{}) widget = &[:theme_classes<D>[i]:];
    add_look<D>(out, widget, spec.[:theme_members<D>[i]:]);
  }
  out.push_back({nullptr, 0, nullptr});
  return std::define_static_array(out);
}

}  // namespace detail

template <class D = void>
consteval theme_styles theme(const detail::theme_spec<D>& spec) {
  const auto entries = detail::theme_entries<D>(spec);
  return detail::theme_access::make(entries.data(), entries.size() - 1);
}

}  // namespace lv
