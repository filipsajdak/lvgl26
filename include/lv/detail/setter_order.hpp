#pragma once

// The order in which a widget's setters are called. A node and a run-time set() call them in the
// order of the members of the props struct (the id, the properties, the styles, the handlers,
// each alphabetical): a tree is written in that order, and the same order is the order of the
// calls, unless LVGL needs another. The table below is the one place that says which setters it
// is, and why. A property in it runs before every other (`first`) or after every other (`last`) of
// its widget; a property that is not in it keeps its place.
//
// A rule is added for a real case only: an example that LVGL draws differently when its setters are
// called in the order of the props. LVGL's doc comments say nothing about these dependencies, so
// the table cannot be read from them. `tests/setter_order.cpp` checks that each entry names a real
// widget and property, and gives each rule a test that reads LVGL's state.

#include <lv/detail/fixed_string.hpp>

#include <cstddef>
#include <meta>
#include <span>
#include <string_view>
#include <vector>

namespace lv::detail {

enum setter_phase : int { setter_first = -1, setter_normal = 0, setter_last = 1 };

struct setter_rule {
  std::string_view widget;
  std::string_view property;
  int phase;
};

using namespace std::string_view_literals;

inline constexpr setter_rule setter_rules[] = {
    // A new map clears the controls and the checked button, so it comes before the other setters.
    {"buttonmatrix"sv, "map"sv, setter_first},
    // set_one_checked checks button 0 again, so the controls of ctrl_map come after it.
    {"buttonmatrix"sv, "ctrl_map"sv, setter_last},
    // set_text moves the cursor to the end of the text, so the cursor position comes after it.
    {"textarea"sv, "cursor_pos"sv, setter_last},
};

// Whether any rule is for this widget. A widget with none pays one compare for each rule, once.
consteval bool widget_has_setter_rule(std::string_view widget) {
  for (const auto& rule : setter_rules)
    if (rule.widget == widget) return true;
  return false;
}

template <fixed W>
inline constexpr bool has_setter_rule = widget_has_setter_rule(W.sv());

// The phase of property `name` of widget `widget`: setter_first, setter_normal or setter_last.
consteval int setter_phase_of(std::string_view widget, std::string_view name) {
  for (const auto& rule : setter_rules)
    if (rule.widget == widget && rule.property == name) return rule.phase;
  return setter_normal;
}

// The numbers of members, in the order that their setters are called: the first ones, then the
// others, then the last ones, each group in the order given. `members` are the reflections of the
// members of a props struct of widget W, and `numbers` indexes them.
template <fixed W>
consteval std::vector<std::size_t> in_setter_order(const std::vector<std::size_t>& numbers,
                                                   std::span<const std::meta::info> members) {
  std::vector<std::size_t> groups[3];
  for (const std::size_t i : numbers)
    groups[setter_phase_of(W.sv(), std::meta::identifier_of(members[i])) + 1].push_back(i);
  std::vector<std::size_t> out;
  for (const auto& group : groups) out.insert(out.end(), group.begin(), group.end());
  return out;
}

}  // namespace lv::detail
