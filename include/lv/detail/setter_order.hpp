#pragma once

// The order in which a widget's setters are called. A node and a run-time set() call them in the
// order of the members of the props struct (the id, the properties, the styles, the handlers,
// each alphabetical): a tree is written in that order, and the same order is the order of the
// calls, unless LVGL needs another. The rules below are the one place that says which setters it
// is, and why. A property in them runs before every other (`first`) or after every other (`last`)
// of its widget; a property that is not in them keeps its place.
//
// A rule is added for a real case only: an example that LVGL draws differently when its setters are
// called in the order of the props. LVGL's doc comments say nothing about these dependencies, so
// the rules cannot be read from them. `tests/setter_order.cpp` checks that each rule names a real
// widget and property, and gives each rule a test that reads LVGL's state.
//
// Every function here is a template on the widget, so that nothing is worked out where this header
// is read: a call with constant arguments in a function that is no template runs in every file.

#include <lv/detail/fixed_string.hpp>

#include <cstddef>
#include <meta>
#include <span>
#include <string_view>
#include <vector>

namespace lv::detail {

enum setter_phase : int { setter_first = -1, setter_normal = 0, setter_last = 1 };

using namespace std::string_view_literals;

// Whether the widget has a rule: one compare for each widget in the rules below, once.
template <fixed W>
inline constexpr bool has_setter_rule = W.sv() == "buttonmatrix"sv || W.sv() == "textarea"sv;

// The phase of property `name` of widget W: setter_first, setter_normal or setter_last.
template <fixed W>
consteval int setter_phase_of(std::string_view name) {
  constexpr std::string_view widget = W.sv();
  if constexpr (widget == "buttonmatrix"sv) {
    // A new map clears the controls and the checked button, so it comes before the other setters.
    if (name == "map"sv) return setter_first;
    // set_one_checked checks button 0 again, so the controls of ctrl_map come after it.
    if (name == "ctrl_map"sv) return setter_last;
  } else if constexpr (widget == "textarea"sv) {
    // set_text moves the cursor to the end of the text, so the cursor position comes after it.
    if (name == "cursor_pos"sv) return setter_last;
  }
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
    groups[setter_phase_of<W>(std::meta::identifier_of(members[i])) + 1].push_back(i);
  std::vector<std::size_t> out;
  for (const auto& group : groups) out.insert(out.end(), group.begin(), group.end());
  return out;
}

}  // namespace lv::detail
