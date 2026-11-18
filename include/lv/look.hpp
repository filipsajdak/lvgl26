#pragma once

// Named looks: a group of the project's design tokens becomes a style, by name.
//
//   // tokens.json: "queue": {"card": {"bg_color": "{color.card}", "radius": "{radius.card}",
//   //                                 "checked": {"bg_color": "{color.active}"}}}
//
//   lv::button({.style = lv::look<"queue.card">})              // what the element is
//   lv::button({.style = lv::look<"tile tile.print">})         // a variant: several looks, a later one wins
//   lv::button({.style = lv::look<"tile"> | lv::on("pressed", {...})})
//
// The code says what an element is; the token file says how it looks. Each entry of the group
// is matched to the style property of the same name, and its token type is checked against what
// the property takes: a color, a dimension, an opacity (a number from 0 to 1), a duration, a
// typography token (a font, drawn by the compiler through the project's lv::typeface), a string
// that names an LVGL enumerator (`"center"`, `"flex"`), or a composite token (a shadow, a border,
// a transition) that is merged as `style | token` is. A nested group named like an LVGL state
// (`checked`, `pressed`) or part (`knob`, `indicator`) is a layer: lv::on of its own. Any other
// nested group is a look of its own (`queue.card.title`), and is not part of the card's look.
//
// A look is one constant style per path, worked out by the compiler on first use: it costs
// nothing in a file that names none, and no heap at run time. Equal looks share one constant
// lv_style_t, as every style does.
//
// The project names its token file once, as lv::cfg is set, before the first use:
//
//   constexpr auto tok = lv::tokens<tokens_json>;
//   template <>
//   inline constexpr lv::design_of<decltype(tok)> lv::design<lv::override>{.tokens = tok};
//
// A part library that several designs share takes the design as a template parameter and passes
// it on: `template <class Design = lv::override> constexpr auto tile = ... lv::look<"tile", Design>`.
//
// The node's own properties beat a look, as LVGL's local styles beat an added style. What a
// factory such as lv::group or lv::column sets by default gives way to a look that says it: a
// group whose look sets a width does not also fill its parent.

#include <lv/config.hpp>
#include <lv/detail/fixed_string.hpp>
#include <lv/font.hpp>
#include <lv/repeat.hpp>
#include <lv/style.hpp>
#include <lv/tokens.hpp>

#include <algorithm>
#include <meta>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace lv {

namespace detail {
struct no_design {};
}  // namespace detail

// The tokens of a project and the typeface that draws its fonts. Specialize lv::design<lv::override>
// with this once, as lv::cfg is specialized.
template <class Tokens>
struct design_of {
  Tokens tokens;
  typeface type{};
};

template <class = override>
inline constexpr detail::no_design design{};

namespace detail {

// lv::style, named through D so that it is generated on first use, not where this header is read.
template <class D>
using look_style_of = style_t<std::void_t<D>>;

template <class D>
concept registered_design = requires { design<D>.tokens; };

// ---- names -------------------------------------------------------------------

struct piece {
  std::size_t at;
  std::size_t size;
};

// The parts of `s` between characters of `separators`. An empty part is kept only when asked.
consteval std::vector<piece> pieces(std::string_view s, std::string_view separators, bool keep_empty) {
  std::vector<piece> out;
  std::size_t start = 0;
  for (std::size_t i = 0; i <= s.size(); ++i) {
    if (i < s.size() && separators.find(s[i]) == std::string_view::npos) continue;
    if (i > start || keep_empty) out.push_back({start, i - start});
    start = i + 1;
  }
  return out;
}

template <std::size_t N>
consteval fixed<N + 1> fixed_from(std::string_view s) {
  char buffer[N + 1]{};
  for (std::size_t i = 0; i < N; ++i) buffer[i] = s[i];
  return fixed<N + 1>{buffer};
}

// How far apart two names are, counting a letter inserted, left out or changed.
consteval std::size_t distance_between(std::string_view a, std::string_view b) {
  std::vector<std::size_t> row(b.size() + 1);
  for (std::size_t j = 0; j <= b.size(); ++j) row[j] = j;
  for (std::size_t i = 1; i <= a.size(); ++i) {
    std::size_t diagonal = row[0];
    row[0] = i;
    for (std::size_t j = 1; j <= b.size(); ++j) {
      const std::size_t above = row[j];
      const std::size_t change = diagonal + (a[i - 1] == b[j - 1] ? 0 : 1);
      row[j] = std::min({above + 1, row[j - 1] + 1, change});
      diagonal = above;
    }
  }
  return row[b.size()];
}

// "a, b, c"
consteval std::string joined_names(const std::vector<std::string>& names) {
  std::string out;
  for (const auto& n : names) out += (out.empty() ? "" : ", ") + n;
  return out;
}

// How many names a message offers for a name that is none of them.
inline constexpr std::size_t names_offered = 5;

// The names of `names` nearest to `wanted`, nearest first, at most `names_offered`.
consteval std::vector<std::string> nearest_list(std::string_view wanted, std::vector<std::string> names) {
  std::vector<std::size_t> far;
  for (const auto& n : names) far.push_back(distance_between(wanted, n));
  std::vector<std::string> picked;
  for (std::size_t k = 0; k < names_offered && k < names.size(); ++k) {
    std::size_t best = 0;
    for (std::size_t i = 1; i < names.size(); ++i)
      if (far[i] < far[best]) best = i;
    picked.push_back(names[best]);
    far[best] = static_cast<std::size_t>(-1);
  }
  return picked;
}

consteval std::string nearest_names(std::string_view wanted, std::vector<std::string> names) {
  return joined_names(nearest_list(wanted, std::move(names)));
}

// A name this far from another (a letter more, less or different) is a likely typo for it.
inline constexpr std::size_t close_enough = 2;

// ---- the token groups --------------------------------------------------------

template <class Group>
inline constexpr auto group_members =
    std::define_static_array(std::meta::nonstatic_data_members_of(^^Group, std::meta::access_context::unchecked()));

// A member that is a group of tokens, not a token: a token is an lv::token<T>.
consteval bool is_group_member(std::meta::info member) {
  return !std::meta::has_template_arguments(std::meta::type_of(member));
}

consteval std::meta::info member_named(std::meta::info group, std::string_view name) {
  for (auto m : std::meta::nonstatic_data_members_of(group, std::meta::access_context::unchecked()))
    if (std::meta::identifier_of(m) == name) return m;
  return {};
}

// The names of the groups of `group`, or of all its members.
consteval std::vector<std::string> member_names(std::meta::info group, bool groups_only) {
  std::vector<std::string> out;
  for (auto m : std::meta::nonstatic_data_members_of(group, std::meta::access_context::unchecked()))
    if (!groups_only || is_group_member(m)) out.push_back(str(std::meta::identifier_of(m)));
  return out;
}

consteval std::string groups_hint(std::meta::info group, std::string_view wanted) {
  const auto names = member_names(group, true);
  if (names.empty()) return "It holds no group.";
  std::string out = "The groups there: " + joined_names(names) + ".";
  if (wanted.empty()) return out;
  const std::string closest = nearest_list(wanted, names).front();
  if (distance_between(wanted, closest) <= close_enough) out += " Did you mean '" + closest + "'?";
  return out;
}

// ---- one entry of a group ----------------------------------------------------

template <class V>
inline constexpr bool is_composite_token =
    std::same_as<V, token<shadow_token>> || std::same_as<V, token<border_token>> ||
    std::same_as<V, token<transition_token>>;

// What a token holds, in the words of a token file.
template <class V>
inline constexpr const char* token_kind = "a token of a kind that a style does not take";
template <>
inline constexpr const char* token_kind<token<lv_color_t>> = "a color";
template <>
inline constexpr const char* token_kind<token<length>> = "a dimension";
template <>
inline constexpr const char* token_kind<token<duration>> = "a duration";
template <>
inline constexpr const char* token_kind<token<double>> = "a number";
template <>
inline constexpr const char* token_kind<token<std::int32_t>> = "a font weight";
template <>
inline constexpr const char* token_kind<token<std::string_view>> = "a string";
template <>
inline constexpr const char* token_kind<token<typography_token>> = "a typography";
template <>
inline constexpr const char* token_kind<token<shadow_token>> = "a shadow";
template <>
inline constexpr const char* token_kind<token<border_token>> = "a border";
template <>
inline constexpr const char* token_kind<token<transition_token>> = "a transition";
template <>
inline constexpr const char* token_kind<token<bezier>> = "a cubic bezier";
template <>
inline constexpr const char* token_kind<token<gradient_token>> = "a gradient";

// What a style property takes, in the same words.
template <class T>
inline constexpr const char* property_kind = "a value of its own kind";
template <>
inline constexpr const char* property_kind<styled<lv_color_t>> = "a color";
template <>
inline constexpr const char* property_kind<styled<length>> = "a dimension (such as 8px)";
template <>
inline constexpr const char* property_kind<styled<opa>> = "a number from 0 to 1";
template <>
inline constexpr const char* property_kind<styled<duration>> = "a duration (such as 200ms)";
template <>
inline constexpr const char* property_kind<styled<const lv_font_t*>> = "a typography token";
template <class T>
inline constexpr const char* property_kind<styled<strict<T>>> = "a whole number";
template <class T>
  requires std::is_enum_v<T>
inline constexpr const char* property_kind<T> = "a string that names an LVGL value";

// The members of lv::style are in alphabetical order, so a name is found by halving: looking at
// every one of the 139 for each entry of each look was most of what a look cost to compile.
template <class D>
consteval std::meta::info style_property(std::string_view name) {
  const auto members = style_members<std::void_t<D>>;
  std::size_t low = 0, high = members.size();
  while (low < high) {
    const std::size_t middle = low + (high - low) / 2;
    const auto found = std::meta::identifier_of(members[middle]);
    if (found == name) return members[middle];
    (found < name ? low = middle + 1 : high = middle);
  }
  return {};
}

template <class D>
consteval std::vector<std::string> style_property_names() {
  std::vector<std::string> out;
  for (auto m : style_members<std::void_t<D>>) out.push_back(str(std::meta::identifier_of(m)));
  return out;
}

// The properties that `name` stands for when LVGL keeps no value under its own name (pad_all, pad_gap),
// or none. A look holds the long forms, so that its style is a constant and a layer can override one.
consteval std::vector<std::string_view> long_names(std::string_view name) {
  for (const auto& s : shorthands)
    if (s.name == name) {
      std::vector<std::string_view> out;
      for (auto target : s.sets)
        if (!target.empty()) out.push_back(target);
      return out;
    }
  return {};
}

template <class A>
using first_type = std::tuple_element_t<0, typename A::types>;

// The enum of `lv_<property>_t` in `scope`, for a property that LVGL types as a number (`layout`).
// The scope is a parameter: a scan of the global namespace written plainly would run in every
// file that reads this header.
consteval std::meta::info enum_of_property(std::meta::info scope, std::string_view name) {
  const std::string wanted = "lv_" + str(name) + "_t";
  for (auto m : std::meta::members_of(scope, std::meta::access_context::unchecked()))
    if (std::meta::has_identifier(m) && std::meta::identifier_of(m) == wanted && std::meta::is_type(m)) return m;
  return {};
}

// Found once for each property in a file, not once for each look that sets it.
template <std::meta::info Target>
inline constexpr std::meta::info number_enum = enum_of_property(^^::, std::meta::identifier_of(Target));

template <class D, std::meta::info Target, class V>
consteval void put_string(look_style_of<D>& s, const V& v, std::string_view entry, std::string_view where) {
  using A = typename[:std::meta::type_of(Target):];
  using first = first_type<A>;
  if constexpr (std::is_enum_v<first>) {
    s.[:Target:] = A(enumerator_named<first>(*v));
  } else if constexpr (number_enum<Target> != std::meta::info{}) {
    using named = typename[:number_enum<Target>:];
    s.[:Target:] = A(token<strict<std::uint16_t>>{strict<std::uint16_t>(static_cast<std::uint16_t>(enumerator_named<named>(*v))),
                                               from_design_tokens{}});
  } else {
    fail("lv: the look entry '" + str(entry) + "' of '" + str(where) + "' is a string, but the style property '" +
         str(entry) + "' takes " + property_kind<first> + ". Change its $type in the token file.");
  }
}

// The value of one token for each long form of the shorthand Target.
template <class D, std::meta::info Target, class V>
consteval void put_long_forms(look_style_of<D>& s, const V& v) {
  constexpr std::string_view shorthand_name = std::meta::identifier_of(Target);
  template for (constexpr std::size_t i : indices<long_names(shorthand_name).size()>) {
    constexpr auto m = style_property<D>(long_names(shorthand_name)[i]);
    using A = typename[:std::meta::type_of(m):];
    s.[:m:] = A(v);
  }
}

[[noreturn]] consteval void fail_several_values(std::string_view entry, std::string_view where) {
  fail("lv: the look entry '" + str(entry) + "' of '" + str(where) + "' sets several properties from several values, "
       "and a token holds one. Write each of them: " + (entry == "size" ? "width and height." : "see lv::style."));
}

template <class First, class V>
[[noreturn]] consteval void fail_wrong_kind(std::string_view entry, std::string_view where) {
  fail("lv: the look entry '" + str(entry) + "' of '" + str(where) + "' is " + token_kind<V> +
       ", but the style property '" + str(entry) + "' takes " + property_kind<First> +
       ". Change the entry's $type or its value in the token file.");
}

// A value that the property takes as it is. A shorthand is its long forms: a constant style holds
// those, and a layer can override one of them.
template <class D, std::meta::info Target, class A, class V>
consteval void put_plain(look_style_of<D>& s, const V& v) {
  if constexpr (long_names(std::meta::identifier_of(Target)).empty())
    s.[:Target:] = A(v);
  else
    put_long_forms<D, Target>(s, v);
}

template <class D, std::meta::info Target, class V>
consteval void put_value(look_style_of<D>& s, const V& v, const typeface& type, std::string_view entry,
                         std::string_view where) {
  using A = typename[:std::meta::type_of(Target):];
  using first = first_type<A>;
  if constexpr (std::tuple_size_v<typename A::types> != 1) {
    fail_several_values(entry, where);
  } else if constexpr (std::same_as<V, token<std::string_view>>) {
    put_string<D, Target>(s, v, entry, where);
  } else if constexpr (std::same_as<V, token<typography_token>> && std::same_as<first, styled<const lv_font_t*>>) {
    s.[:Target:] = A(type(v));
  } else if constexpr (!std::same_as<V, token<typography_token>> && std::is_constructible_v<first, const V&>) {
    put_plain<D, Target, A>(s, v);
  } else {
    fail_wrong_kind<first, V>(entry, where);
  }
}

// A typography token under a name that is no style property: its font and its letter spacing.
template <class D>
consteval void put_typography(look_style_of<D>& s, const token<typography_token>& v, const typeface& type) {
  s.text_font = type(v);
  s.text_letter_space = part(v->letter_spacing);
}

template <class D, std::meta::info M, class V>
consteval void put_entry(look_style_of<D>& s, const V& v, const typeface& type, std::string_view where) {
  constexpr std::string_view entry = std::meta::identifier_of(M);
  constexpr auto target = style_property<D>(entry);
  if constexpr (is_composite_token<V>) {
    s = s | v;
  } else if constexpr (target != std::meta::info{}) {
    put_value<D, target>(s, v, type, entry, where);
  } else if constexpr (std::same_as<V, token<typography_token>>) {
    put_typography<D>(s, v, type);
  } else {
    fail("lv: the look entry '" + str(entry) + "' of '" + str(where) + "' fits no style property. Nearest style properties: " +
         nearest_names(entry, style_property_names<D>()) +
         ". A shadow, border, transition or typography token may have any name; anything else is named for its property.");
  }
}

// ---- parts and states as layers ----------------------------------------------

// The enums are parameters only so that nothing is scanned until a name is read, as in lv::on.
template <class Part = lv_part_t, class State = lv_state_t>
consteval bool names_part_or_state(std::string_view name) {
  return selector_named<Part>(name) || selector_named<State>(name);
}

template <class Part = lv_part_t, class State = lv_state_t>
consteval lv_style_selector_t selector_of_name(std::string_view name) {
  if (const auto part = selector_named<Part>(name)) return *part;
  return *selector_named<State>(name);
}

template <class D, class Group>
consteval void fill_level(look_style_of<D>& own, std::vector<style_layer>& layers, const Group& g, lv_style_selector_t selector,
                          bool in_layer, std::string_view where);

template <class D, std::meta::info M, class Group>
consteval void nest(std::vector<style_layer>& layers, const Group& g, lv_style_selector_t selector, bool in_layer,
                    std::string_view where) {
  constexpr std::string_view name = std::meta::identifier_of(M);
  if constexpr (!names_part_or_state<>(name)) {
    // At the top a group of another name is a look of its own, such as tile.print.
    if (in_layer)
      fail("lv: in the look '" + str(where) + "' the group '" + str(name) +
           "' sits inside a state or a part, which holds values, parts and states only, and '" + str(name) +
           "' is neither. The parts: " + selector_names<lv_part_t>() + ". The states: " + selector_names<lv_state_t>() +
           ". Move it out of the layer: it is a look of its own, there.");
  } else {
    const lv_style_selector_t combined = selector | selector_of_name<>(name);
    look_style_of<D> own{};
    std::vector<style_layer> inner;
    fill_level<D>(own, inner, g.[:M:], combined, true, where);
    if (sets_a_value<look_style_of<D>, void>(own)) layers.push_back({combined, stored_style<look_style_of<D>, void>(own)});
    layers.insert(layers.end(), inner.begin(), inner.end());
  }
}

template <class D, class Group>
consteval void fill_level(look_style_of<D>& own, std::vector<style_layer>& layers, const Group& g, lv_style_selector_t selector,
                          bool in_layer, std::string_view where) {
  template for (constexpr auto m : group_members<Group>) {
    if constexpr (is_group_member(m))
      nest<D, m>(layers, g, selector, in_layer, where);
    else
      put_entry<D, m>(own, g.[:m:], design<D>.type, where);
  }
}

// The style of one group: its entries, then a layer for each of its parts and states.
template <class D, class Group>
consteval look_style_of<D> style_of_group(const Group& g, std::string_view where) {
  look_style_of<D> s{};
  std::vector<style_layer> layers;
  fill_level<D>(s, layers, g, 0, false, where);
  if (!layers.empty()) s.layers = style_layers(std::define_static_array(layers));
  return s;
}

// ---- following a path --------------------------------------------------------

template <fixed Path>
inline constexpr auto segments_of = std::define_static_array(pieces(Path.sv(), ".", true));

template <fixed Path, std::size_t I, class D, class Group>
consteval look_style_of<D> walk(const Group& g) {
  constexpr piece at = segments_of<Path>[I];
  constexpr std::string_view segment = Path.sv().substr(at.at, at.size);
  constexpr std::string_view so_far = Path.sv().substr(0, at.at + at.size);
  constexpr std::string_view quoted_path = Path.sv();
  if constexpr (segment.empty()) {
    fail("lv: the look '" + str(quoted_path) + "' has an empty name between its dots. Write lv::look<\"queue.card\">.");
  } else {
    constexpr auto m = member_named(^^Group, str(token_member_name(segment)));
    if constexpr (m == std::meta::info{}) {
      fail("lv: the look '" + str(quoted_path) + "' has no group '" + str(segment) + "'" +
           (I == 0 ? std::string(" in the token file. ") : " in '" + str(so_far.substr(0, at.at - 1)) + "'. ") +
           groups_hint(^^Group, segment));
    } else if constexpr (!is_group_member(m)) {
      fail("lv: the look '" + str(quoted_path) + "' names a token, not a group: '" + str(so_far) +
           "' holds one value. A look is a group of tokens, written lv::look<\"" + str(so_far.substr(0, at.at ? at.at - 1 : 0)) +
           "\">, and the style property is its entry '" + str(segment) + "'.");
    } else if constexpr (I + 1 == segments_of<Path>.size()) {
      return style_of_group<D>(g.[:m:], quoted_path);
    } else {
      return walk<Path, I + 1, D>(g.[:m:]);
    }
  }
}

// The style of one look: a path such as "queue.card".
template <fixed Path, class D>
inline constexpr look_style_of<D> one_look = walk<Path, 0, D>(design<D>.tokens);

template <fixed Paths>
inline constexpr auto words_of = std::define_static_array(pieces(Paths.sv(), " \t\n", false));

// Word `I` of `Paths` as a path of its own.
template <fixed Paths, std::size_t I>
inline constexpr auto word_path =
    fixed_from<words_of<Paths>[I].size>(Paths.sv().substr(words_of<Paths>[I].at, words_of<Paths>[I].size));

// Several looks merge left to right. A single one is not merged: copying a style is the cost.
template <fixed Paths, class D>
consteval look_style_of<D> merged_looks() {
  if constexpr (words_of<Paths>.empty()) {
    fail("lv: lv::look<\"" + str(Paths.sv()) + "\"> names no look. Write the path of a group of the token file: "
         "lv::look<\"queue.card\">. " + groups_hint(^^std::remove_cvref_t<decltype(design<D>.tokens)>, {}));
  } else {
    look_style_of<D> s = one_look<word_path<Paths, 0>, D>;
    template for (constexpr std::size_t i : indices<words_of<Paths>.size() - 1>) s = s | one_look<word_path<Paths, i + 1>, D>;
    return s;
  }
}

template <fixed Paths, class D>
consteval look_style_of<D> look_value() {
  if constexpr (registered_design<D>) {
    return merged_looks<Paths, D>();
  } else {
    static_assert(registered_design<D>,
                  "lv: no design is registered for lv::look. Name the token file once, before the first look, in a header "
                  "that every file includes: constexpr auto tok = lv::tokens<tokens_json>; template <> inline constexpr "
                  "lv::design_of<decltype(tok)> lv::design<lv::override>{.tokens = tok};");
    return {};
  }
}

}  // namespace detail

// The look of one or several groups of the project's tokens, as a style: lv::look<"queue.card">,
// or lv::look<"tile tile.print"> where a later one wins. D picks the registered design.
template <detail::fixed Paths, class D = override>
inline constexpr auto look = detail::look_value<Paths, D>();

namespace detail {

template <fixed Prefix, auto V>
consteval auto path_of_enumerator() {
  constexpr std::string_view name = name_of<V>;
  constexpr std::size_t size = Prefix.sv().size() + 1 + name.size();
  return fixed_from<size>(str(Prefix.sv()) + "." + str(name));
}

}  // namespace detail

// The look named for an enumerator: lv::look_of<"tile", subpage::print> is lv::look<"tile.print">.
template <detail::fixed Prefix, auto V, class D = override>
  requires std::is_enum_v<decltype(V)>
inline constexpr auto look_of = look<detail::path_of_enumerator<Prefix, V>(), D>;

}  // namespace lv
