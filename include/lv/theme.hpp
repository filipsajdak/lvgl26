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
//
// A part in a state is a selector in `style`, as in a node:
//   .slider = {.style = base | lv::on("knob", "pressed", {.bg_color = tok.color.active})}

#include <lv/detail/missing.hpp>
#include <lv/detail/theme_styles.hpp>
#include <lv/props.hpp>
#include <lv/style.hpp>

#include <array>
#include <span>
#include <string_view>
#include <vector>

namespace lv::detail {
// This header is read: see lv/detail/missing.hpp.
template <>
inline constexpr bool included<"lv/theme.hpp"> = true;
}  // namespace lv::detail

namespace lv {

namespace detail {

// A style member of a theme's widget: its name and what selects it.
struct look_item {
  std::string_view name;
  std::meta::info target;  // as in member_item
};

// The scans below are templates over D, the delay of the whole header: a call that depends on
// nothing is run where the header is read, in every file, though no file names a theme
// (14 G instructions in each, measured 2026-11-19).
template <class D>
consteval std::vector<look_item> look_items() {
  std::vector<look_item> items;
  for (const auto& slot : named_style_slots()) items.push_back({slot.name, slot.target});
  return items;
}

template <class D>
consteval std::vector<std::meta::info> look_specs(std::meta::info style) {
  std::vector<std::meta::info> out;
  for (const auto& item : look_items<D>())
    out.push_back(std::meta::data_member_spec(style, {.name = str(item.name)}));
  return out;
}

template <class D>
consteval std::vector<std::meta::info> look_targets() {
  std::vector<std::meta::info> out;
  for (const auto& item : look_items<D>()) out.push_back(item.target);
  return out;
}

// What a theme says about one kind of widget.
template <class D = void>
struct look_of {
  struct type;
  consteval { std::meta::define_aggregate(^^type, look_specs<D>(^^style_t<D>)); }
};

template <class D = void>
inline constexpr auto look_members = std::define_static_array(
    std::meta::nonstatic_data_members_of(^^typename look_of<D>::type, std::meta::access_context::unchecked()));

template <class D = void>
inline constexpr std::span<const std::meta::info> look_selectors = std::define_static_array(look_targets<D>());

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
template <class D>
consteval std::vector<theme_item> theme_items() {
  std::vector<theme_item> items{{"screen", std::meta::info{}}};
  for (auto m : std::meta::members_of(^^::, std::meta::access_context::unchecked()))
    if (const auto widget = widget_of_class(m); !widget.empty()) items.push_back({theme_member(widget), m});
  sort_by_name(items);
  return items;
}

// A theme_item that can live in a static array.
struct theme_row {
  const char* name;
  std::size_t size;  // of name
  std::meta::info widget;
};

template <class D>
consteval std::vector<theme_row> theme_rows() {
  std::vector<theme_row> out;
  for (const auto& item : theme_items<D>()) out.push_back({std::define_static_string(item.name), item.name.size(), item.widget});
  return out;
}

// The scan runs once in a file: the members of a theme and their classes both come from this
// table. D only delays it.
template <class D = void>
inline constexpr auto theme_table = std::define_static_array(theme_rows<D>());

template <class D>
consteval std::vector<std::meta::info> theme_specs() {
  std::vector<std::meta::info> out;
  for (const auto& row : theme_table<D>)
    out.push_back(std::meta::data_member_spec(^^typename look_of<D>::type, {.name = str(std::string_view(row.name, row.size))}));
  return out;
}

template <class D>
consteval std::vector<std::meta::info> theme_widgets() {
  std::vector<std::meta::info> out;
  for (const auto& row : theme_table<D>) out.push_back(row.widget);
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
inline constexpr std::span<const std::meta::info> theme_classes = std::define_static_array(theme_widgets<D>());

// The layers that lv::on added to a style of a theme, after the style itself.
consteval void add_theme_layers(std::vector<theme_entry>& out, const lv_obj_class_t* widget,
                                lv_style_selector_t selector, style_layers layers) {
  if (layers.count != 0 && selector != LV_PART_MAIN)
    fail("lv: lv::on goes in .style, because it names its own part and state. Write "
         ".style = base | lv::on(\"knob\", \"pressed\", {...}) instead of .style_knob = lv::on(...).");
  for (const style_layer& l : layers.list()) out.push_back({widget, l.selector, l.style});
}

// The styles that `look` sets, for the widgets of `widget`.
template <class D>
consteval void add_look(std::vector<theme_entry>& out, const lv_obj_class_t* widget,
                        const typename look_of<D>::type& look) {
  template for (constexpr auto j : indices<look_members<D>.size()>) {
    const auto& s = look.[:look_members<D>[j]:];
    constexpr lv_style_selector_t selector = selector_of<look_selectors<D>[j]>();
    if (sets_a_value<style_t<D>, D>(s)) out.push_back({widget, selector, stored_style<style_t<D>, D>(s)});
    add_theme_layers(out, widget, selector, s.layers);
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
