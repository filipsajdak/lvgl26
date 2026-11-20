#pragma once

// Editor stub of lv/look.hpp: a group of the project's design tokens as a style, by name.
//
//   template <>
//   inline constexpr lv::design_of<decltype(tok)> lv::design<lv::override>{.tokens = tok};
//
//   lv::button({.style = lv::look<"queue.card">})
//   lv::button({.style = lv::look<"tile tile.print"> | lv::look_of<"tile", Page>})
//
// The editor marks a look that names no group of the token file, where it is written: the stub
// of the token file lists the groups (lv/token_files.hpp, `look_groups`). It does not check the
// entries of a group against the style properties, nor the path of lv::look_of, which GCC does.

#include <lv/detail/missing.hpp>
#include <lv/config.hpp>
#include <lv/detail/fixed_string.hpp>
#include <lv/font.hpp>
#include <lv/repeat.hpp>
#include <lv/stub.hpp>
#include <lv/style.hpp>

#include <string_view>
#include <type_traits>

namespace lv::detail {
template <>
inline constexpr bool included<"lv/look.hpp"> = true;
}  // namespace lv::detail

namespace lv {

namespace stub {

struct no_design {};

// Whether every look of the space separated `looks` is one of the space separated `groups`.
constexpr bool names_groups(std::string_view looks, std::string_view groups) {
  while (!looks.empty()) {
    const auto start = looks.find_first_not_of(" \t\n");
    if (start == std::string_view::npos) break;
    looks.remove_prefix(start);
    const auto word = looks.substr(0, looks.find_first_of(" \t\n"));
    looks.remove_prefix(word.size());
    bool found = false;
    for (auto rest = groups; !rest.empty() && !found;) {
      const auto group = rest.substr(0, rest.find(' '));
      found = group == word;
      rest.remove_prefix(group.size() + (group.size() < rest.size() ? 1 : 0));
    }
    if (!found) return false;
  }
  return true;
}

}  // namespace stub

/// The tokens of a project and the typeface that draws its fonts. Specialize lv::design<lv::override>
/// with this once, before the first look.
template <class Tokens>
struct design_of {
  Tokens tokens;
  typeface type{};
};

/// The design of a project: the primary template holds none. Specialize it once, as lv::cfg.
template <class = override>
inline constexpr stub::no_design design{};

namespace stub {

template <detail::fixed Paths, class D>
consteval style_t checked_look() {
  static_assert(requires { design<D>.tokens; },
                "lv: no design is registered for lv::look. Name the token file once, before the first look: "
                "template <> inline constexpr lv::design_of<decltype(tok)> lv::design<lv::override>{.tokens = tok};");
  if constexpr (requires { design<D>.tokens; }) {
    static_assert(names_groups(Paths.sv(), look_groups<std::remove_cvref_t<decltype(design<D>.tokens)>>) ||
                      std::string_view{look_groups<std::remove_cvref_t<decltype(design<D>.tokens)>>}.empty(),
                  "lv: this look names no group of the token file. Write the path of a group as the token file has "
                  "it: lv::look<\"queue.card\">; several looks are separated by spaces. The groups are listed in "
                  "lv/token_files.hpp, under look_groups.");
  }
  return {};
}

}  // namespace stub

/// The look of one or several groups of the project's tokens, as a style: lv::look<"queue.card">,
/// or lv::look<"tile tile.print"> where a later one wins. D picks the registered design.
template <detail::fixed Paths, class D>
  requires detail::included<"lv/look.hpp">
inline constexpr stub::style_t look<Paths, D> = stub::checked_look<Paths, D>();

/// The look named for an enumerator: lv::look_of<"tile", subpage::print> is lv::look<"tile.print">.
template <detail::fixed Prefix, auto V, class D = override>
  requires std::is_enum_v<decltype(V)>
inline constexpr stub::style_t look_of{};

}  // namespace lv
