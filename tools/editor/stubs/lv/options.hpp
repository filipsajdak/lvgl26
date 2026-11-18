#pragma once

// Editor stub of lv/options.hpp: lists of options for a dropdown and a roller.
//
//   lv::dropdown({.options = {"100%", "75%", "50%"}})
//   lv::dropdown({.options = lv::options_of<quality>, .selected = lv::bind(chosen)})
//   lv::roller({.options = {{"Mon", "Tue", "Wed"}, "infinite"}})

#include <lv/but.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace lv {

/// A list of options: the text LVGL takes, and what a state of an enum needs to follow it.
/// Made by `.options = {"a", "b", "c"}` and by lv::options_of<E>. A text with line breaks
/// converts to one as well.
struct option_list {
  constexpr option_list() = default;
  constexpr option_list(const char*) {}

  /// Two or more options. An empty option or one with a line break is a compile error.
  template <std::size_t... N>
    requires(sizeof...(N) >= 2)
  consteval option_list([[maybe_unused]] const char (&... items)[N]) {}
};

/// The text of an option made from an enumerator: its name. Specialize it to give another
/// text: `template <> struct lv::option_text<quality> { static consteval const char* of(quality q) { ... } };`
template <class E>
struct option_text {
  static consteval const char* of(E) { __builtin_unreachable(); }
};

/// One option for each enumerator of E, in the order they are declared, except those of Left.
/// `.selected = lv::bind(state)` of a state that holds an E follows the choice.
template <class E, class Left = but<>>
  requires std::is_enum_v<E>
inline constexpr option_list options_of{};

}  // namespace lv
