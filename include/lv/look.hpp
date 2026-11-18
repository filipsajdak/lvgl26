#pragma once

// Named looks: a group of the project's design tokens becomes a style, by name.
//
//   // tokens.json: "queue": {"card": {"bg_color": {"$value": "{color.card}"},
//   //                                 "radius": {"$value": "{radius.card}"},
//   //                                 "checked": {"bg_color": {"$value": "{color.active}"}}}}
//
//   lv::button({.style = lv::look<"queue.card">})              // what the element is
//   lv::button({.style = lv::look<"tile tile-print">})         // a variant: several looks, a later one wins
//   lv::button({.style = lv::look<"tile"> | lv::on("pressed", {...})})
//
// The code says what an element is; the token file says how it looks. The file is a plain Design
// Tokens file that design tools open. Each entry of the group is matched to the style property of
// the same name, and its token type is checked against what the property takes: a color (written
// #rrggbbaa, it also sets the opacity of the same name, bg_color with bg_opa), a dimension, an
// opacity (a number from 0 to 1), a duration, a typography token (a font, drawn by the compiler
// through the project's lv::typeface), or a composite token (a shadow, a border, a transition)
// that is merged as `style | token` is. What the standard has no type for goes in the group's
// "$extensions" under the key io.github.filipsajdak.lvgl26: an LVGL value by name, "flex_flow":
// "row", and a size in percent, "width": "50%".
//
// A group nested in a look is a layer, and nothing else: its name is an LVGL part (`knob`,
// `indicator`) or state (`checked`, `pressed`), as lv::on takes them. A variant is a sibling group,
// `tile-print` beside `tile`; lv::look<"tile tile-print"> stacks them. Any other nested group is a
// compile error. "$extends" is not read yet and is a compile error in a look.
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
#include <optional>
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

// The members that hold the vendor data of a group and its "$extends", not tokens or groups.
consteval bool is_vendor_member(std::meta::info member) {
  return std::meta::identifier_of(member) == vendor_member;
}
consteval bool is_extends_member(std::meta::info member) {
  return std::meta::identifier_of(member) == extends_member;
}

// A member that is a group of tokens, not a token: a token is an lv::token<T>.
consteval bool is_group_member(std::meta::info member) {
  return !std::meta::has_template_arguments(std::meta::type_of(member)) && !is_vendor_member(member) &&
         !is_extends_member(member);
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
  // A name in the token file may hold a dash; the compiler knows it with an underscore.
  const std::string like_a_member = token_member_name(wanted);
  const std::string closest = nearest_list(like_a_member, names).front();
  if (distance_between(like_a_member, closest) <= close_enough) out += " Did you mean '" + closest + "'?";
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
template <>
inline constexpr const char* token_kind<token<rgba_color>> = "a color with an alpha";

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

// A font that a look draws is named by what the drawing depends on: the file of the typeface and
// the size. The same file and size are drawn once for the whole program, however many looks and
// tokens ask for them. A look cannot rely on the compiler remembering the call: GCC caches the
// result of a call only down to -fconstexpr-cache-depth levels of nesting (8), and a look is
// deeper than that, so each look drew its fonts again (6.5 G instructions each).
struct font_key {
  std::size_t file;
  std::int32_t px;
  friend constexpr bool operator==(const font_key&, const font_key&) = default;
};

template <class D, font_key K>
inline constexpr const lv_font_t* drawn_font = design<D>.type.draw(K.file, K.px);

// The font of the typography token T, from the typeface of the design D.
template <class D, const auto& T>
consteval const lv_font_t* font_of() {
  constexpr font_key key{design<D>.type.file_of(T), T->font_size.raw()};
  return drawn_font<D, key>;
}

// Found once for each property in a file, not once for each look that sets it.
template <std::meta::info Target>
inline constexpr std::meta::info number_enum = enum_of_property(^^::, std::meta::identifier_of(Target));

// The properties that LVGL lets take a size in percent.
inline constexpr std::string_view percent_properties[] = {
    "width", "height", "min_width", "min_height", "max_width", "max_height", "x", "y",
    "transform_width", "transform_height", "translate_x", "translate_y"};

consteval bool takes_percent(std::string_view property) {
  for (auto name : percent_properties)
    if (name == property) return true;
  return false;
}

consteval std::string percent_properties_list() {
  std::string out;
  for (auto name : percent_properties) out += (out.empty() ? "" : ", ") + str(name);
  return out;
}

// "50%" is a size in percent.
consteval bool is_percent_text(std::string_view text) {
  return text.size() > 1 && text.ends_with("%");
}

consteval std::int32_t percent_of(std::string_view text, std::string_view entry, std::string_view where) {
  std::int32_t n = 0;
  for (char c : text.substr(0, text.size() - 1)) {
    if (c < '0' || c > '9')
      fail("lv: the $extensions entry '" + str(entry) + "' of '" + str(where) + "' is '" + str(text) +
           "', and a percentage is written as whole digits and %, such as \"50%\".");
    n = n * 10 + (c - '0');
  }
  return n;
}

// An LVGL value by its name, from the vendor data: "flex_flow": "row".
template <class D, std::meta::info Target>
consteval void put_name(look_style_of<D>& s, std::string_view text, std::string_view entry, std::string_view where) {
  using A = typename[:std::meta::type_of(Target):];
  using first = first_type<A>;
  if constexpr (std::is_enum_v<first>) {
    s.[:Target:] = A(enumerator_named<first>(text));
  } else if constexpr (number_enum<Target> != std::meta::info{}) {
    using named = typename[:number_enum<Target>:];
    s.[:Target:] = A(token<strict<std::uint16_t>>{strict<std::uint16_t>(static_cast<std::uint16_t>(enumerator_named<named>(text))),
                                               from_design_tokens{}});
  } else {
    fail("lv: the $extensions entry '" + str(entry) + "' of '" + str(where) + "' is the text '" + str(text) +
         "', and the style property '" + str(entry) + "' takes " + property_kind<first> +
         ". $extensions holds LVGL values by name and sizes in percent: write this one as a token, with its $type.");
  }
}

// A size in percent, from the vendor data: "width": "50%".
template <class D, std::meta::info Target>
consteval void put_percent(look_style_of<D>& s, std::string_view text, std::string_view entry, std::string_view where) {
  using A = typename[:std::meta::type_of(Target):];
  using first = first_type<A>;
  if (!takes_percent(entry))
    fail("lv: the $extensions entry '" + str(entry) + "' of '" + str(where) + "' is a percentage, and LVGL takes one for " +
         percent_properties_list() + " only.");
  if constexpr (std::is_constructible_v<first, const token<length>&>)
    s.[:Target:] = A(token<length>{length{percent{percent_of(text, entry, where)}}, from_design_tokens{}});
}

// One entry of the vendor data: the style property of its name, from a name or a percentage.
template <class D, std::meta::info M, const auto& T>
consteval void put_vendor(look_style_of<D>& s, std::string_view where) {
  constexpr std::string_view entry = std::meta::identifier_of(M);
  constexpr auto target = style_property<D>(entry);
  if constexpr (target == std::meta::info{}) {
    fail("lv: the $extensions entry '" + str(entry) + "' of '" + str(where) + "' fits no style property. Nearest style properties: " +
         nearest_names(entry, style_property_names<D>()) + ".");
  } else {
    const std::string_view text = *T;
    if (is_percent_text(text))
      put_percent<D, target>(s, text, entry, where);
    else
      put_name<D, target>(s, text, entry, where);
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

[[noreturn]] consteval void fail_percent_value(std::string_view entry, std::string_view where);

// A value that the property takes as it is. A shorthand is spelled out when its level is done
// (with_long_forms), so that a constant style holds the long forms and a layer can override one.
template <class D, std::meta::info Target, class A, class V>
consteval void put_plain(look_style_of<D>& s, const V& v, std::string_view entry, std::string_view where) {
  if constexpr (std::same_as<V, token<length>>) {
    if (LV_COORD_IS_PCT(v->raw())) fail_percent_value(entry, where);
  }
  s.[:Target:] = A(v);
}

template <class First>
[[noreturn]] consteval void fail_string_token(std::string_view entry, std::string_view where) {
  fail("lv: the look entry '" + str(entry) + "' of '" + str(where) + "' is a string token, and a string is no standard "
       "value for the style property '" + str(entry) + "', which takes " + property_kind<First> +
       ". An LVGL value by name (\"flex_flow\": \"row\") goes in \"$extensions\": {\"" + str(vendor_key) +
       "\": {...}} of the group.");
}

[[noreturn]] consteval void fail_percent_value(std::string_view entry, std::string_view where) {
  fail("lv: the look entry '" + str(entry) + "' of '" + str(where) + "' is a percentage, and Design Tokens Format 2025.10 has no "
       "percentage. Write a size in pixels as the $value, for design tools, and the percentage in \"$extensions\": {\"" +
       str(vendor_key) + "\": {\"" + str(entry) + "\": \"50%\"}} of the group.");
}

// The opacity property that goes with a color property: bg_color with bg_opa, image_recolor with
// image_recolor_opa. Empty when there is none.
consteval std::string opacity_name(std::string_view color) {
  if (color.ends_with("_color")) return str(color.substr(0, color.size() - 6)) + "_opa";
  if (color.ends_with("recolor")) return str(color) + "_opa";
  return {};
}

// A color with alpha: the color, and the opacity of the same name unless the group sets one itself.
template <class D, std::meta::info Target, const auto& T>
consteval void put_rgba(look_style_of<D>& s, std::string_view entry, std::string_view where) {
  using A = typename[:std::meta::type_of(Target):];
  if constexpr (!std::is_constructible_v<first_type<A>, const token<lv_color_t>&>) {
    fail_wrong_kind<first_type<A>, token<rgba_color>>(entry, where);
  } else {
    s.[:Target:] = A(token<lv_color_t>{T->color, from_design_tokens{}});
    constexpr std::string_view opa_property = std::define_static_string(opacity_name(std::meta::identifier_of(Target)));
    constexpr auto opacity = style_property<D>(opa_property);
    if constexpr (opacity == std::meta::info{}) {
      fail("lv: the look entry '" + str(entry) + "' of '" + str(where) + "' carries an alpha, and the style has no opacity property for '" +
           str(entry) + "'. Write the color as #rrggbb.");
    } else if (T->alpha != 255) {
      using O = typename[:std::meta::type_of(opacity):];
      if (!s.[:opacity:].set) s.[:opacity:] = O(raw_value<opa>{opa::from_raw(T->alpha)});
    }
  }
}

template <class D, std::meta::info Target, const auto& T>
consteval void put_value(look_style_of<D>& s, std::string_view entry, std::string_view where) {
  const auto& v = T;
  using V = std::remove_cvref_t<decltype(T)>;
  using A = typename[:std::meta::type_of(Target):];
  using first = first_type<A>;
  if constexpr (std::tuple_size_v<typename A::types> != 1) {
    fail_several_values(entry, where);
  } else if constexpr (std::same_as<V, token<std::string_view>>) {
    fail_string_token<first>(entry, where);
  } else if constexpr (std::same_as<V, token<rgba_color>>) {
    put_rgba<D, Target, T>(s, entry, where);
  } else if constexpr (std::same_as<V, token<typography_token>> && std::same_as<first, styled<const lv_font_t*>>) {
    s.[:Target:] = A(token<const lv_font_t*>{font_of<D, T>(), from_design_tokens{}});
  } else if constexpr (!std::same_as<V, token<typography_token>> && std::is_constructible_v<first, const V&>) {
    put_plain<D, Target, A>(s, v, entry, where);
  } else {
    fail_wrong_kind<first, V>(entry, where);
  }
}

// A typography token under a name that is no style property: its font and its letter spacing.
template <class D, const auto& T>
consteval void put_typography(look_style_of<D>& s) {
  s.text_font = token<const lv_font_t*>{font_of<D, T>(), from_design_tokens{}};
  s.text_letter_space = part(T->letter_spacing);
}

template <class D, std::meta::info M, const auto& T>
consteval void put_entry(look_style_of<D>& s, std::string_view where) {
  const auto& v = T;
  using V = std::remove_cvref_t<decltype(T)>;
  constexpr std::string_view entry = std::meta::identifier_of(M);
  constexpr auto target = style_property<D>(entry);
  if constexpr (is_composite_token<V>) {
    s = s | v;
  } else if constexpr (target != std::meta::info{}) {
    put_value<D, target, T>(s, entry, where);
  } else if constexpr (std::same_as<V, token<typography_token>>) {
    put_typography<D, T>(s);
  } else {
    fail("lv: the look entry '" + str(entry) + "' of '" + str(where) + "' fits no style property. Nearest style properties: " +
         nearest_names(entry, style_property_names<D>()) +
         ". A shadow, border, transition or typography token may have any name; anything else is named for its property.");
  }
}

// ---- parts and states as layers ----------------------------------------------

// The key that the token reader made the member `member` of: default_ was "default", a keyword.
// A dash is an underscore in both, so the names of parts and states need nothing else.
consteval std::string_view key_of_member(std::string_view member) {
  const std::string_view bare = member.substr(0, member.size() - 1);
  return member.ends_with('_') && is_keyword(bare) ? bare : member;
}

// The part or state of E whose name, read as the token reader reads a key, is `member`: the
// group "default" is the member default_, "focus-key" is focus_key. `any` is not a layer.
template <class E>
consteval std::optional<lv_style_selector_t> layer_named(std::string_view member) {
  const std::string_view key = key_of_member(member);
  if (key == "any") return std::nullopt;
  for (const enum_name& e : enum_names<E>)
    if (short_enum_name<E>(e) == key) return static_cast<lv_style_selector_t>(e.value);
  return std::nullopt;
}

// The enums are parameters only so that nothing is scanned until a name is read, as in lv::on.
template <class Part = lv_part_t, class State = lv_state_t>
consteval bool names_part_or_state(std::string_view member) {
  return layer_named<Part>(member) || layer_named<State>(member);
}

template <class Part = lv_part_t, class State = lv_state_t>
consteval lv_style_selector_t selector_of_name(std::string_view member) {
  if (const auto part = layer_named<Part>(member)) return *part;
  return *layer_named<State>(member);
}

[[noreturn]] consteval void fail_extends(std::string_view where) {
  fail("lv: the group of the look '" + str(where) + "' has \"$extends\", which Design Tokens Format 2025.10 has and lvgl26 does "
       "not read yet. Write the entries of the group out, or use the sibling groups of a variant: lv::look<\"a a-b\">.");
}

template <class D, const auto& V>
consteval void put_vendor_entries(look_style_of<D>& s, std::string_view where) {
  template for (constexpr auto m : group_members<std::remove_cvref_t<decltype(V)>>)
    put_vendor<D, m, V.[:m:]>(s, where);
}

template <class D, const auto& G>
consteval void fill_level(look_style_of<D>& own, std::vector<style_layer>& layers, lv_style_selector_t selector,
                          std::string_view where);

// The names of the parts and states of LVGL, without `any`.
consteval std::vector<std::string> layer_names() {
  std::vector<std::string> out;
  for (const enum_name& e : enum_names<lv_part_t>)
    if (short_enum_name<lv_part_t>(e) != "any") out.push_back(short_enum_name<lv_part_t>(e));
  for (const enum_name& e : enum_names<lv_state_t>)
    if (short_enum_name<lv_state_t>(e) != "any") out.push_back(short_enum_name<lv_state_t>(e));
  return out;
}

[[noreturn]] consteval void fail_not_a_layer(std::string_view name, std::string_view where) {
  const auto near = nearest_list(name, layer_names());
  std::string hint;
  if (distance_between(name, near.front()) <= close_enough) hint = " Did you mean '" + near.front() + "'?";
  fail("lv: in the look '" + str(where) + "' the group '" + str(name) + "' is neither a part nor a state." + hint +
       " A group nested in a look is a layer: a part (" + selector_names<lv_part_t>() + ") or a state (" +
       selector_names<lv_state_t>() + "). A variant of a look is a sibling group, 'a-" + str(name) +
       "' beside 'a', used as lv::look<\"a a-" + str(name) + "\">.");
}

template <class D, std::meta::info M, const auto& G>
consteval void nest(look_style_of<D>& here, std::vector<style_layer>& layers, lv_style_selector_t selector,
                    std::string_view where) {
  constexpr std::string_view name = std::meta::identifier_of(M);
  if constexpr (!names_part_or_state<>(name)) {
    fail_not_a_layer(name, where);
  } else if constexpr (selector_of_name<>(name) == 0) {
    look_style_of<D> own{};  // main or default: this level itself, over what it sets beside
    fill_level<D, G.[:M:]>(own, layers, selector, where);
    here = here | (sets_shorthand<void>(own) ? with_long_forms<void>(own) : own);
  } else {
    const lv_style_selector_t combined = selector | selector_of_name<>(name);
    look_style_of<D> own{};
    std::vector<style_layer> inner;
    fill_level<D, G.[:M:]>(own, inner, combined, where);
    if (sets_a_value<look_style_of<D>, void>(own)) layers.push_back({combined, stored_style<look_style_of<D>, void>(own)});
    layers.insert(layers.end(), inner.begin(), inner.end());
  }
}

// A group named main or default: a layer that adds nothing to the selector, so it is the level it is in.
template <class Part = lv_part_t, class State = lv_state_t>
consteval bool adds_no_selector(std::meta::info member) {
  const std::string_view name = std::meta::identifier_of(member);
  return is_group_member(member) && names_part_or_state<Part, State>(name) && selector_of_name<Part, State>(name) == 0;
}

// Where an entry goes in the order of a look, the highest first: a main or default group last.
template <class Part = lv_part_t, class State = lv_state_t>
consteval std::size_t order_rank(std::meta::info member) {
  return adds_no_selector<Part, State>(member) ? 0 : 1;
}

// The members of a group in the order a look puts them, each rank in file order. A JSON object has
// no order, so a main or default group beats the entries beside it however they are written; the
// shorthands among the entries follow the one rule of with_long_forms.
template <class Group>
consteval std::vector<std::meta::info> ordered_members() {
  std::vector<std::meta::info> out;
  std::vector<std::size_t> ranks;  // of `out`, each worked out once
  for (auto m : group_members<Group>) {
    const std::size_t r = order_rank(m);
    std::size_t at = 0;
    while (at < ranks.size() && ranks[at] >= r) ++at;
    out.insert(out.begin() + static_cast<std::ptrdiff_t>(at), m);
    ranks.insert(ranks.begin() + static_cast<std::ptrdiff_t>(at), r);
  }
  return out;
}

template <class Group>
inline constexpr auto entry_order = std::define_static_array(ordered_members<Group>());

template <class D, const auto& G>
consteval void fill_level(look_style_of<D>& own, std::vector<style_layer>& layers, lv_style_selector_t selector,
                          std::string_view where) {
  template for (constexpr auto m : entry_order<std::remove_cvref_t<decltype(G)>>) {
    if constexpr (is_extends_member(m))
      fail_extends(where);
    else if constexpr (is_vendor_member(m))
      put_vendor_entries<D, G.[:m:]>(own, where);
    else if constexpr (is_group_member(m))
      nest<D, m, G>(own, layers, selector, where);
    else
      put_entry<D, m, G.[:m:]>(own, where);
  }
}

// The style of one group: its entries, then a layer for each of its parts and states.
template <class D, const auto& G>
consteval look_style_of<D> style_of_group(std::string_view where) {
  look_style_of<D> s{};
  std::vector<style_layer> layers;
  fill_level<D, G>(s, layers, 0, where);
  if (sets_shorthand<void>(s)) s = with_long_forms<void>(s);
  if (!layers.empty()) s.layers = style_layers(std::define_static_array(layers));
  return s;
}

// ---- following a path --------------------------------------------------------

template <fixed Path>
inline constexpr auto segments_of = std::define_static_array(pieces(Path.sv(), ".", true));

template <fixed Path, std::size_t I, class D, const auto& G>
consteval look_style_of<D> walk() {
  using Group = std::remove_cvref_t<decltype(G)>;
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
      return style_of_group<D, G.[:m:]>(quoted_path);
    } else {
      return walk<Path, I + 1, D, G.[:m:]>();
    }
  }
}

// The style of one look: a path such as "queue.card".
template <fixed Path, class D>
inline constexpr look_style_of<D> one_look = walk<Path, 0, D, design<D>.tokens>();

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
  return fixed_from<size>(str(Prefix.sv()) + "-" + str(name));
}

}  // namespace detail

// The look named for an enumerator: lv::look_of<"tile", subpage::print> is lv::look<"tile-print">, a
// sibling group of "tile" (a group nested in a look is a layer, so a variant cannot be nested).
template <detail::fixed Prefix, auto V, class D = override>
  requires std::is_enum_v<decltype(V)>
inline constexpr auto look_of = look<detail::path_of_enumerator<Prefix, V>(), D>;

}  // namespace lv
