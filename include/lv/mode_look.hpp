#pragma once

// A look whose styles follow the mode of a display (docs/looks.md, "A look that follows the mode").
//
// The project names one token file for each mode, of one shape, as it names its one file for lv::look:
//
//   struct day;
//   struct dark;
//   template <> inline constexpr lv::design_of<decltype(day_tokens)> lv::design<day>{.tokens = day_tokens};
//   template <> inline constexpr lv::design_of<decltype(dark_tokens)> lv::design<dark>{.tokens = dark_tokens};
//   template <> inline constexpr lv::mode_designs_of<day, dark> lv::mode_designs<lv::override>{};
//
// The designs are in the order of the enumerators of the state that the display follows
// (display.follow(mode, {day_theme, dark_theme})). A widget takes the look in its `.look`:
//
//   lv::button({.look = lv::mode_look<"tile">, .style = {.radius = tok.radius.card}})
//
// A switch puts the styles of the new mode where those of the old one stand, in place and with no
// allocation, so what the node's own `.style` or a later layer says still wins. Both designs hold the
// group, with the same properties set and the same layers: a mode look is worked out in every mode where
// it is written, and a difference is a compile error that names the property.
//
// A widget made after a switch takes the mode of the moment. A screen that is not loaded keeps the mode
// it was built with, as it keeps its theme.

#include <lv/detail/missing.hpp>
#include <lv/detail/mode_look.hpp>
#include <lv/look.hpp>

#include <meta>
#include <string>
#include <vector>

namespace lv::detail {
// This header is read: see lv/detail/missing.hpp.
template <>
inline constexpr bool included<"lv/mode_look.hpp"> = true;
}  // namespace lv::detail

namespace lv {

// The tags of the designs of the modes, in the order of the enumerators of the state:
// template <> inline constexpr lv::mode_designs_of<day, dark> lv::mode_designs<lv::override>{};
template <class... Designs>
struct mode_designs_of {};

namespace detail {
struct no_modes {};
}  // namespace detail

template <class = override>
inline constexpr detail::no_modes mode_designs{};

namespace detail {

template <class T>
struct designs_listed {
  static constexpr bool named = false;
};
template <class... Designs>
struct designs_listed<mode_designs_of<Designs...>> {
  static constexpr bool named = true;
  static constexpr std::size_t count = sizeof...(Designs);
};

template <class Modes>
concept modes_named = designs_listed<std::remove_cvref_t<Modes>>::named;

// The first difference between the look of the first design and the look of another, as text; empty
// when they have one shape: the same properties set, the same layers with the same selectors.
template <class S>
consteval std::string shape_difference(const S& first, const S& other) {
  std::string out;
  template for (constexpr auto m : style_members<void>) {
    if (out.empty() && first.[:m:].set != other.[:m:].set)
      out = "the property '" + str(std::meta::identifier_of(m)) + "' is set in " + (first.[:m:].set ? "the first design only" : "this design only");
  }
  if (out.empty() && first.layers.count != other.layers.count)
    out = "it has " + decimal(other.layers.count) + " layer(s) (a part or a state), the first design has " + decimal(first.layers.count);
  for (std::size_t i = 0; out.empty() && i < first.layers.count; ++i)
    if (first.layers.first[i].selector != other.layers.first[i].selector)
      out = "its layer number " + decimal(i + 1) + " is another part or state than in the first design";
  return out;
}

// The entries of one look in one design: its own style, then its layers.
template <fixed Paths, class D>
consteval std::vector<mode_entry> entries_of_look() {
  constexpr const auto& s = look<Paths, D>;
  std::vector<mode_entry> out;
  if (sets_a_value<look_style_of<D>, void>(s)) out.push_back({stored_style<look_style_of<D>, void>(s), LV_PART_MAIN});
  for (const style_layer& l : s.layers.list()) out.push_back({l.style, l.selector});
  return out;
}

template <fixed Paths, class First, class Other>
consteval void check_pair() {
  constexpr bool differs = !shape_difference(look<Paths, First>, look<Paths, Other>).empty();
  if constexpr (differs) {
    fail("lv: the mode look '" + str(Paths.sv()) + "' has another shape in the design '" +
         str(std::meta::display_string_of(^^Other)) + "' than in '" + str(std::meta::display_string_of(^^First)) +
         "': " + shape_difference(look<Paths, First>, look<Paths, Other>) + ". Both token files hold the group with the same entries, as the themes of lv::display::follow have one shape.");
  }
}

template <fixed Paths, class First, class... Rest>
consteval void check_shape() {
  (check_pair<Paths, First, Rest>(), ...);
}

// The table of the look in every design of `Modes`.
template <fixed Paths, class... Designs>
consteval const mode_object* mode_table() {
  check_shape<Paths, Designs...>();
  std::vector<mode_entry> all;
  std::size_t per = 0;
  const auto add = [&](std::vector<mode_entry> row) {
    per = row.size();
    all.insert(all.end(), row.begin(), row.end());
  };
  (add(entries_of_look<Paths, Designs>()), ...);
  return std::define_static_object(mode_object{std::define_static_array(all).data(), sizeof...(Designs), per});
}

template <fixed Paths, class Modes>
struct mode_look_of;
template <fixed Paths, class... Designs>
struct mode_look_of<Paths, mode_designs_of<Designs...>> {
  static constexpr mode_look_ref value{mode_table<Paths, Designs...>()};
};

template <fixed Paths, class D>
consteval mode_look_ref mode_look_value() {
  if constexpr (modes_named<decltype(mode_designs<D>)>) {
    return mode_look_of<Paths, std::remove_cvref_t<decltype(mode_designs<D>)>>::value;
  } else {
    static_assert(modes_named<decltype(mode_designs<D>)>,
                  "lv: no designs are named for lv::mode_look. Name the token file of each mode once, in a header that every "
                  "file includes: template <> inline constexpr lv::mode_designs_of<day, dark> lv::mode_designs<lv::override>{}; "
                  "with lv::design<day> and lv::design<dark> specialized as lv::design<lv::override> is for lv::look.");
    return {};
  }
}

}  // namespace detail

// The look of a group in the design of each mode, for `.look`: lv::mode_look<"tile">. D picks the
// registered list of designs, as it does for lv::look.
template <detail::fixed Paths, class D>
  requires detail::included<"lv/mode_look.hpp">
inline constexpr auto mode_look<Paths, D> = detail::mode_look_value<Paths, D>();

}  // namespace lv
