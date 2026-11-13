#pragma once

// Design tokens: a W3C Design Tokens (DTCG) JSON file becomes a typed constexpr value.
//
//   constexpr char tokens_json[] = {
//   #embed "tokens.json"
//       , 0};
//   constexpr auto tok = lv::tokens<tokens_json>;
//
//   lv::box({.style = {.bg_color = tok.color.surface, .radius = tok.radius.md}})
//
// Reflection generates a struct for every token group, with one member per token.
// A token is an lv::token<T>: a color holds an lv_color_t, a dimension an lv::length,
// a duration an lv::duration, a shadow an lv::shadow_token, and so on. It converts to
// its plain value, and it is what a style accepts (see lv/style.hpp).
// Aliases such as "{color.primary}" are resolved. A token that cannot be read, or an
// alias that points nowhere, is a compile error. A misspelled token name is too,
// because it is a struct member.

#include <lv/detail/api.hpp>
#include <lv/json.hpp>
#include <lv/token_types.hpp>
#include <lv/units.hpp>
#include <lv/values.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace lv {

namespace detail {

enum class token_type {
  none, color, dimension, duration, number, font_weight, string,
  cubic_bezier, shadow, border, typography, transition, gradient,
  unsupported
};

consteval token_type token_type_of(std::string_view t) {
  if (t == "color") return token_type::color;
  if (t == "dimension") return token_type::dimension;
  if (t == "duration") return token_type::duration;
  if (t == "number") return token_type::number;
  if (t == "fontWeight") return token_type::font_weight;
  if (t == "fontFamily" || t == "string" || t == "strokeStyle") return token_type::string;
  if (t == "cubicBezier") return token_type::cubic_bezier;
  if (t == "shadow") return token_type::shadow;
  if (t == "border") return token_type::border;
  if (t == "typography") return token_type::typography;
  if (t == "transition") return token_type::transition;
  if (t == "gradient") return token_type::gradient;
  return token_type::unsupported;
}

// The value type of a token, or a null reflection for a $type this file does not know.
consteval std::meta::info token_value_type(token_type t) {
  switch (t) {
    case token_type::color: return ^^lv_color_t;
    case token_type::dimension: return ^^length;
    case token_type::duration: return ^^duration;
    case token_type::number: return ^^double;
    case token_type::font_weight: return std::meta::dealias(^^::int32_t);
    case token_type::string: return ^^std::string_view;
    case token_type::cubic_bezier: return ^^bezier;
    case token_type::shadow: return ^^shadow_token;
    case token_type::border: return ^^border_token;
    case token_type::typography: return ^^typography_token;
    case token_type::transition: return ^^transition_token;
    case token_type::gradient: return ^^gradient_token;
    default: return {};
  }
}

// The member type of a token: token<value type>.
consteval std::meta::info token_cpp_type(token_type t) {
  const auto v = token_value_type(t);
  return v == std::meta::info{} ? v : std::meta::substitute(^^token, {v});
}

[[noreturn]] consteval void token_fail(std::string_view what) {
  throw unknown_name("lv: design tokens: " + str(what));
}

// The document without the trailing NUL that the #embed idiom adds.
template <const auto& Doc>
consteval std::string_view token_text() {
  std::string_view s{Doc, std::size(Doc)};
  while (!s.empty() && s.back() == '\0') s.remove_suffix(1);
  return s;
}

template <const auto& Doc>
consteval json::value token_root() {
  const auto v = json::value::parse(token_text<Doc>());
  if (!v || !v->is(json::kind::object)) token_fail("the document is not a JSON object");
  return *v;
}

struct resolved_token {
  json::value value;
  token_type type = token_type::none;
};

consteval resolved_token resolve_token(json::value root, json::value token, token_type inherited,
                                       int depth = 0);

// Follows "{group.token}" from the root. The type is inherited along the way.
consteval resolved_token follow_alias(json::value root, std::string_view alias, int depth) {
  if (depth > 8) token_fail("the alias " + str(alias) + " goes in a circle");
  auto path = alias.substr(1, alias.size() - 2);
  json::value at = root;
  token_type type = token_type::none;
  while (true) {
    const auto dot = path.find('.');
    at = at[path.substr(0, dot)];
    if (!at.is(json::kind::object)) token_fail("the alias " + str(alias) + " points at nothing");
    if (auto t = at["$type"]) type = token_type_of(t.string());
    if (dot == std::string_view::npos) break;
    path.remove_prefix(dot + 1);
  }
  if (!at["$value"]) token_fail("the alias " + str(alias) + " points at a group, not a token");
  return resolve_token(root, at, type, depth + 1);
}

consteval resolved_token resolve_token(json::value root, json::value token, token_type inherited,
                                       int depth) {
  const auto v = token["$value"];
  auto type = inherited;
  if (auto t = token["$type"]) type = token_type_of(t.string());
  if (v.is(json::kind::string) && v.string().starts_with("{") && v.string().ends_with("}")) {
    const auto target = follow_alias(root, v.string(), depth);
    return {target.value, type == token_type::none ? target.type : type};
  }
  return {v, type};
}

// Whether a member of a group becomes a struct member: a sub-group, or a token of
// a supported type. "$type", "$description" and the like are not members.
consteval bool token_member_used(json::value root, json::member m, token_type inherited) {
  if (m.key.starts_with("$")) return false;
  const json::value child{m.text};
  if (!child.is(json::kind::object))
    token_fail("'" + str(m.key) + "' is neither a group nor a token");
  if (!child["$value"]) return true;
  return token_cpp_type(resolve_token(root, child, inherited).type) != std::meta::info{};
}

// A JSON key as a C++ member name: "primary-500" -> primary_500, "2xl" -> _2xl.
consteval std::string token_member_name(std::string_view key) {
  std::string out;
  for (char c : key) {
    const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                    c == '_';
    out.push_back(ok ? c : '_');
  }
  if (out.empty() || (out[0] >= '0' && out[0] <= '9')) out.insert(out.begin(), '_');
  if (is_keyword(out)) out.push_back('_');
  return out;
}

template <const auto& Doc, std::size_t Offset, token_type Inherited>
struct token_group;

template <const auto& Doc, std::size_t Offset, token_type Inherited>
using token_group_t = typename token_group<Doc, Offset, Inherited>::type;

// The members of the struct for the group that starts at `offset` in the document.
template <const auto& Doc>
consteval std::vector<std::meta::info> token_specs(std::size_t offset, token_type inherited) {
  const auto doc = token_text<Doc>();
  const auto root = token_root<Doc>();
  const auto group = json::value::at_offset(doc, offset);
  if (auto t = group["$type"]) inherited = token_type_of(t.string());

  std::vector<std::string> names;
  std::vector<std::meta::info> specs;
  for (std::size_t i = 0; i < group.size(); ++i) {
    const auto m = group.member_at(i);
    if (!token_member_used(root, m, inherited)) continue;

    const json::value child{m.text};
    std::meta::info type;
    if (child["$value"]) {
      type = token_cpp_type(resolve_token(root, child, inherited).type);
    } else {
      const auto child_offset = static_cast<std::size_t>(m.text.data() - doc.data());
      type = std::meta::dealias(std::meta::substitute(
          ^^token_group_t, {std::meta::reflect_object(Doc), std::meta::reflect_constant(child_offset),
                            std::meta::reflect_constant(inherited)}));
    }

    auto name = token_member_name(m.key);
    for (const auto& n : names)
      if (n == name) token_fail("two tokens in one group are both called '" + name + "'");
    specs.push_back(std::meta::data_member_spec(type, {.name = name}));
    names.push_back(std::move(name));
  }
  return specs;
}

template <const auto& Doc, std::size_t Offset, token_type Inherited>
struct token_group {
  struct type;
  consteval { std::meta::define_aggregate(^^type, token_specs<Doc>(Offset, Inherited)); }
};

// ---- reading values ---------------------------------------------------------

consteval int hex_digit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  token_fail("a color has a character that is not a hex digit");
}

consteval lv_color_t color_from_hex(std::string_view s) {
  if (!s.starts_with("#") || (s.size() != 4 && s.size() != 7 && s.size() != 9))
    token_fail("the color '" + str(s) + "' is not #rgb, #rrggbb or #rrggbbaa");
  const auto channel = [&](std::size_t i) {
    return s.size() == 4 ? hex_digit(s[1 + i]) * 17
                         : hex_digit(s[1 + 2 * i]) * 16 + hex_digit(s[2 + 2 * i]);
  };
  lv_color_t c{};
  c.red = static_cast<std::uint8_t>(channel(0));
  c.green = static_cast<std::uint8_t>(channel(1));
  c.blue = static_cast<std::uint8_t>(channel(2));
  return c;
}

consteval std::int32_t rounded(double v) {
  return static_cast<std::int32_t>(v < 0 ? v - 0.5 : v + 0.5);
}

// A number with a unit, written "8px" or {"value": 8, "unit": "px"}.
struct quantity {
  double value = 0;
  std::string_view unit;
};

consteval quantity read_quantity(json::value v) {
  if (v.is(json::kind::object)) return {v["value"].number(), v["unit"].string()};
  if (v.is(json::kind::number)) return {v.number(), {}};
  if (!v.is(json::kind::string)) token_fail("a dimension or duration must be a string or an object");
  const auto s = v.string();
  std::size_t i = 0;
  while (i < s.size() && (s[i] == '-' || s[i] == '.' || (s[i] >= '0' && s[i] <= '9'))) ++i;
  return {json::value{s.substr(0, i)}.number(), s.substr(i)};
}

template <class T>
consteval T read_token(json::value root, json::value v);

// A part of a composite token. It may be a literal or an alias to another token.
template <class T>
consteval T read_part(json::value root, json::value v, std::string_view what) {
  if (!v) token_fail("a composite token has no '" + str(what) + "'");
  if (v.is(json::kind::string) && v.string().starts_with("{") && v.string().ends_with("}"))
    return read_token<T>(root, follow_alias(root, v.string(), 0).value);
  return read_token<T>(root, v);
}

template <class T>
consteval T read_token(json::value root, json::value v) {
  if constexpr (std::same_as<T, lv_color_t>) {
    if (v.is(json::kind::string)) return color_from_hex(v.string());
    if (auto hex = v["hex"]) return color_from_hex(hex.string());
    const auto parts = v["components"];
    if (parts.size() != 3) token_fail("a color needs a hex string or three components");
    lv_color_t c{};
    c.red = static_cast<std::uint8_t>(rounded(parts.at(0).number() * 255));
    c.green = static_cast<std::uint8_t>(rounded(parts.at(1).number() * 255));
    c.blue = static_cast<std::uint8_t>(rounded(parts.at(2).number() * 255));
    return c;
  } else if constexpr (std::same_as<T, length>) {
    const auto q = read_quantity(v);
    if (q.unit == "px" || q.unit.empty()) return length::px(rounded(q.value));
    if (q.unit == "rem") return length::px(rounded(q.value * 16));
    if (q.unit == "%") return length{percent{rounded(q.value)}};
    token_fail("the dimension unit '" + str(q.unit) + "' is not px, rem or %");
  } else if constexpr (std::same_as<T, duration>) {
    const auto q = read_quantity(v);
    if (q.unit == "ms" || q.unit.empty()) return {static_cast<std::uint32_t>(rounded(q.value))};
    if (q.unit == "s") return {static_cast<std::uint32_t>(rounded(q.value * 1000))};
    token_fail("the duration unit '" + str(q.unit) + "' is not ms or s");
  } else if constexpr (std::same_as<T, double>) {
    if (!v.is(json::kind::number)) token_fail("a number token must be a JSON number");
    return v.number();
  } else if constexpr (std::same_as<T, std::int32_t>) {
    if (v.is(json::kind::number)) return rounded(v.number());
    if (v.string() == "normal") return 400;
    if (v.string() == "bold") return 700;
    token_fail("the font weight '" + str(v.string()) + "' is not a number, normal or bold");
  } else if constexpr (std::same_as<T, std::string_view>) {
    // A font family may be a list of fallbacks: the first one is used.
    if (v.is(json::kind::array)) return v.at(0).string();
    if (v.is(json::kind::object)) return "custom";  // a strokeStyle written as an object
    return v.string();
  } else if constexpr (std::same_as<T, bezier>) {
    if (!v.is(json::kind::array) || v.size() != 4)
      token_fail("a cubicBezier token must be a list of four numbers");
    return {v.at(0).number(), v.at(1).number(), v.at(2).number(), v.at(3).number()};
  } else if constexpr (std::same_as<T, shadow_token>) {
    const auto s = v.is(json::kind::array) ? v.at(0) : v;
    return {read_part<lv_color_t>(root, s["color"], "color"),
            read_part<length>(root, s["offsetX"], "offsetX"),
            read_part<length>(root, s["offsetY"], "offsetY"),
            read_part<length>(root, s["blur"], "blur"),
            read_part<length>(root, s["spread"], "spread")};
  } else if constexpr (std::same_as<T, border_token>) {
    return {read_part<lv_color_t>(root, v["color"], "color"),
            read_part<length>(root, v["width"], "width"),
            read_part<std::string_view>(root, v["style"], "style")};
  } else if constexpr (std::same_as<T, typography_token>) {
    return {read_part<std::string_view>(root, v["fontFamily"], "fontFamily"),
            read_part<length>(root, v["fontSize"], "fontSize"),
            read_part<std::int32_t>(root, v["fontWeight"], "fontWeight"),
            read_part<length>(root, v["letterSpacing"], "letterSpacing"),
            read_part<double>(root, v["lineHeight"], "lineHeight")};
  } else if constexpr (std::same_as<T, transition_token>) {
    return {read_part<duration>(root, v["duration"], "duration"),
            read_part<duration>(root, v["delay"], "delay"),
            read_part<bezier>(root, v["timingFunction"], "timingFunction")};
  } else {
    static_assert(std::same_as<T, gradient_token>);
    gradient_token g{};
    if (!v.is(json::kind::array) || v.size() > g.stops.size())
      token_fail("a gradient token must be a list of at most eight stops");
    for (; g.count < v.size(); ++g.count) {
      const auto stop = v.at(g.count);
      g.stops[g.count] = {read_part<lv_color_t>(root, stop["color"], "color"),
                          read_part<double>(root, stop["position"], "position")};
    }
    return g;
  }
}

template <class T>
inline constexpr bool is_token_leaf = false;
template <class T>
inline constexpr bool is_token_leaf<token<T>> = true;

template <class T>
struct token_value;
template <class T>
struct token_value<token<T>> {
  using type = T;
};

// Fills the generated struct G from its group. Struct members and used JSON
// members are in the same order, so they are walked together.
template <class G, const auto& Doc>
consteval G fill_tokens(json::value group, token_type inherited) {
  const auto root = token_root<Doc>();
  if (auto t = group["$type"]) inherited = token_type_of(t.string());

  G out{};
  [[maybe_unused]] std::size_t i = 0;  // unused when the group has no members
  template for (constexpr auto m : std::define_static_array(std::meta::nonstatic_data_members_of(
                    ^^G, std::meta::access_context::unchecked()))) {
    while (!token_member_used(root, group.member_at(i), inherited)) ++i;
    const json::value child{group.member_at(i).text};
    ++i;

    using M = typename[:std::meta::type_of(m):];
    if constexpr (is_token_leaf<M>)
      out.[:m:] = M{read_token<typename token_value<M>::type>(
                        root, resolve_token(root, child, inherited).value),
                    from_design_tokens{}};
    else
      out.[:m:] = fill_tokens<M, Doc>(child, inherited);
  }
  return out;
}

// Where the root object starts in the document: after any leading white space.
template <const auto& Doc>
inline constexpr std::size_t token_root_offset =
    static_cast<std::size_t>(token_root<Doc>().text().data() - token_text<Doc>().data());

template <const auto& Doc>
consteval auto load_tokens() {
  return fill_tokens<token_group_t<Doc, token_root_offset<Doc>, token_type::none>, Doc>(
      token_root<Doc>(), token_type::none);
}

}  // namespace detail

namespace detail {
// A document embedded as an array of char: `constexpr char doc[] = { #embed ... , 0 };`
template <class T>
concept embedded_text = std::is_bounded_array_v<std::remove_cvref_t<T>> &&
                        std::same_as<std::remove_cv_t<std::remove_extent_t<std::remove_cvref_t<T>>>, char>;
}  // namespace detail

// The design tokens of the embedded DTCG document `Doc`, as a typed constant.
template <const auto& Doc>
  requires detail::embedded_text<decltype(Doc)>
inline constexpr auto tokens = detail::load_tokens<Doc>();

}  // namespace lv
