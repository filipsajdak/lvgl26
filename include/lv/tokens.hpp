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

#include <lv/config.hpp>
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

// The composite types, from shadow to gradient, stay together: check_forms tests the range.
enum class token_type {
  none, color, dimension, duration, number, font_weight, string,
  cubic_bezier, shadow, border, typography, transition, gradient,
  unsupported
};

consteval token_type token_type_of(std::string_view t) {
  struct entry {
    std::string_view name;
    token_type type;
  };
  for (const entry& e : {entry{"color", token_type::color},
                         entry{"dimension", token_type::dimension},
                         entry{"duration", token_type::duration},
                         entry{"number", token_type::number},
                         entry{"fontWeight", token_type::font_weight},
                         entry{"fontFamily", token_type::string},
                         entry{"string", token_type::string},
                         entry{"strokeStyle", token_type::string},
                         entry{"cubicBezier", token_type::cubic_bezier},
                         entry{"shadow", token_type::shadow},
                         entry{"border", token_type::border},
                         entry{"typography", token_type::typography},
                         entry{"transition", token_type::transition},
                         entry{"gradient", token_type::gradient}})
    if (e.name == t) return e.type;
  return token_type::unsupported;
}

// The value type of a token, or a null reflection for a $type this file does not know.
consteval std::meta::info token_value_type(token_type t) {
  struct entry {
    token_type type;
    std::meta::info value;
  };
  for (const entry& e : {entry{token_type::color, ^^lv_color_t},
                         entry{token_type::dimension, ^^length},
                         entry{token_type::duration, ^^duration},
                         entry{token_type::number, ^^double},
                         entry{token_type::font_weight, std::meta::dealias(^^::int32_t)},
                         entry{token_type::string, ^^std::string_view},
                         entry{token_type::cubic_bezier, ^^bezier},
                         entry{token_type::shadow, ^^shadow_token},
                         entry{token_type::border, ^^border_token},
                         entry{token_type::typography, ^^typography_token},
                         entry{token_type::transition, ^^transition_token},
                         entry{token_type::gradient, ^^gradient_token}})
    if (e.type == t) return e.value;
  return {};
}

// The member type of a token: token<value type>.
consteval std::meta::info token_cpp_type(token_type t) {
  const auto v = token_value_type(t);
  return v == std::meta::info{} ? v : std::meta::substitute(^^token, {v});
}

[[noreturn]] consteval void token_fail(std::string_view what) {
  fail("lv: design tokens: " + str(what));
}

// The key of the vendor data in "$extensions": reverse domain name, as the Design Tokens Format
// recommends (5.2.3). The one place it is written.
inline constexpr std::string_view vendor_key = "io.github.filipsajdak.lvgl26";

// What a group holds under "$extensions" for lvgl26: an LVGL value by name, such as
// "flex_flow": "row", or a size in percent, "width": "50%". One string token for each key.
// (A group struct has it as the member `lvgl26_vendor`, only when the file has the data.)
inline constexpr std::string_view vendor_member = "lvgl26_vendor";

// "$extends" is part of the standard and not read yet: a group that has it gets this member, and
// a look of the group is an error that says so.
struct unsupported_extends {
  std::string_view target;
};
inline constexpr std::string_view extends_member = "lvgl26_extends";

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

// Whether a member of a group is a group or a token: "$type", "$description" and the like are not.
consteval bool is_group_or_token(json::member m) {
  if (m.key.starts_with("$")) return false;
  if (!json::value{m.text}.is(json::kind::object))
    token_fail("'" + str(m.key) + "' is neither a group nor a token");
  return true;
}

// Whether a member of a group becomes a struct member: a sub-group, or a token of
// a supported type. "$type", "$description" and the like are not members.
consteval bool token_member_used(json::value root, json::member m, token_type inherited) {
  if (!is_group_or_token(m)) return false;
  const json::value child{m.text};
  if (!child["$value"]) return true;
  return token_cpp_type(resolve_token(root, child, inherited).type) != std::meta::info{};
}

// A color with an alpha: the object form has an "alpha" member, the form before 2025.10 is #rrggbbaa.
consteval bool has_alpha(json::value v) {
  // A search of the text: a color object has no string that could say "alpha" but its key.
  if (v.is(json::kind::object)) return v.text().find("\"alpha\"") != std::string_view::npos;
  return v.is(json::kind::string) && v.string().starts_with("#") && v.string().size() == 9;
}

// The member type of a token: token<value type>, and token<rgba_color> for a color with alpha.
consteval std::meta::info token_member_type(const resolved_token& resolved) {
  if (resolved.type == token_type::color && has_alpha(resolved.value))
    return std::meta::substitute(^^token, {^^rgba_color});
  return token_cpp_type(resolved.type);
}

// The value that starts at `offset` in `doc`. The offset comes from a value of the
// same document.
consteval json::value value_at(std::string_view doc, std::size_t offset)
  pre(offset < doc.size())
{
  return json::value{doc.substr(offset, json::detail::skip_value(doc, offset) - offset)};
}

// A JSON key as a C++ member name: "primary-500" -> primary_500, "2xl" -> _2xl.
consteval std::string token_member_name(std::string_view key) {
  std::string out;
  for (char c : key) out.push_back(is_name_char(c) ? c : '_');
  if (out.empty() || json::detail::is_digit(out[0])) out.insert(out.begin(), '_');
  if (is_keyword(out)) out.push_back('_');
  return out;
}

template <const auto& Doc, std::size_t Offset, token_type Inherited>
struct token_group;

// The struct of the vendor data of a group: a string token for each key.
template <const auto& Doc, std::size_t Offset>
struct vendor_group;

template <const auto& Doc, std::size_t Offset>
using vendor_group_t = typename vendor_group<Doc, Offset>::type;

// The vendor object of a group, or a null value.
consteval json::value vendor_object(json::value group) {
  const auto extensions = group["$extensions"];
  if (!extensions.is(json::kind::object)) return {};
  return extensions[vendor_key];
}

template <const auto& Doc, std::size_t Offset, token_type Inherited>
using token_group_t = typename token_group<Doc, Offset, Inherited>::type;

// The members that follow the tokens of a group: its vendor data, and "$extends" for a group that has it.
template <const auto& Doc>
consteval void add_group_extras(std::vector<std::meta::info>& specs, json::value group) {
  if (const auto mine = vendor_object(group); mine.is(json::kind::object)) {
    const auto at = static_cast<std::size_t>(mine.text().data() - token_text<Doc>().data());
    specs.push_back(std::meta::data_member_spec(
        std::meta::dealias(std::meta::substitute(
            ^^vendor_group_t, {std::meta::reflect_object(Doc), std::meta::reflect_constant(at)})),
        {.name = vendor_member}));
  }
  if (group["$extends"]) specs.push_back(std::meta::data_member_spec(^^unsupported_extends, {.name = extends_member}));
}

// The members of the struct for the group that starts at `offset` in the document.
template <const auto& Doc>
consteval std::vector<std::meta::info> token_specs(std::size_t offset, token_type inherited) {
  const auto doc = token_text<Doc>();
  const auto root = token_root<Doc>();
  const auto group = value_at(doc, offset);
  if (auto t = group["$type"]) inherited = token_type_of(t.string());

  std::vector<std::string> names;
  std::vector<std::meta::info> specs;
  for (const json::member m : group.members()) {
    if (!is_group_or_token(m)) continue;

    const json::value child{m.text};
    std::meta::info type;
    if (child["$value"]) {
      type = token_member_type(resolve_token(root, child, inherited));
      if (type == std::meta::info{}) continue;
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
  add_group_extras<Doc>(specs, group);
  return specs;
}

template <const auto& Doc>
consteval std::vector<std::meta::info> vendor_specs(std::size_t offset) {
  const auto object = value_at(token_text<Doc>(), offset);
  std::vector<std::meta::info> specs;
  for (const json::member m : object.members()) {
    if (!json::value{m.text}.is(json::kind::string))
      token_fail("the value of '" + str(m.key) + "' in $extensions '" + str(vendor_key) +
                 "' is not a string. Write an LVGL value by its name (\"flex_flow\": \"row\") or a size in percent (\"width\": \"50%\")");
    specs.push_back(std::meta::data_member_spec(^^token<std::string_view>, {.name = token_member_name(m.key)}));
  }
  return specs;
}

template <const auto& Doc, std::size_t Offset>
struct vendor_group {
  struct type;
  consteval { std::meta::define_aggregate(^^type, vendor_specs<Doc>(Offset)); }
};

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
  if (s.starts_with("#") && s.size() == 9)
    token_fail("the color '" + str(s) + "' has an alpha channel, which a color token does not "
               "carry. Write it as #rrggbb and give the opacity its own token");
  if (!s.starts_with("#") || (s.size() != 4 && s.size() != 7))
    token_fail("the color '" + str(s) + "' is not #rgb or #rrggbb");
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
  const auto n = json::detail::number_length(s);
  if (n == 0) token_fail("'" + str(s) + "' has no number before its unit");
  return {json::value{s.substr(0, n)}.number(), s.substr(n)};
}

// read_token(type, root, value): one overload per token type. `root` is the
// document, for aliases inside composite tokens.
template <class T>
using read_as = std::type_identity<T>;

consteval lv_color_t read_token(read_as<lv_color_t>, json::value, json::value v) {
  if (v.is(json::kind::string)) return color_from_hex(v.string());
  if (auto hex = v["hex"]) return color_from_hex(hex.string());
  const auto parts = v["components"];
  if (parts.size() != 3) token_fail("a color needs a hex string or three components");
  lv_color_t c{};
  c.red = static_cast<std::uint8_t>(rounded(parts.at(0).number() * 255));
  c.green = static_cast<std::uint8_t>(rounded(parts.at(1).number() * 255));
  c.blue = static_cast<std::uint8_t>(rounded(parts.at(2).number() * 255));
  return c;
}

consteval rgba_color read_token(read_as<rgba_color>, json::value root, json::value v) {
  if (v.is(json::kind::object))
    return {read_token(read_as<lv_color_t>{}, root, v), static_cast<std::uint8_t>(rounded(v["alpha"].number() * 255))};
  const auto s = v.string();
  const auto rgb = color_from_hex(s.substr(0, 7));
  return {rgb, static_cast<std::uint8_t>(hex_digit(s[7]) * 16 + hex_digit(s[8]))};
}

consteval length read_token(read_as<length>, json::value, json::value v) {
  const auto q = read_quantity(v);
  if (q.unit == "px" || q.unit.empty()) return length::px(rounded(q.value));
  if (q.unit == "rem") return length::px(rounded(q.value * 16));
  if (q.unit == "%") return length{percent{rounded(q.value)}};
  token_fail("the dimension unit '" + str(q.unit) + "' is not px, rem or %");
}

consteval duration read_token(read_as<duration>, json::value, json::value v) {
  const auto q = read_quantity(v);
  if (q.unit == "ms" || q.unit.empty()) return {static_cast<std::uint32_t>(rounded(q.value))};
  if (q.unit == "s") return {static_cast<std::uint32_t>(rounded(q.value * 1000))};
  token_fail("the duration unit '" + str(q.unit) + "' is not ms or s");
}

consteval double read_token(read_as<double>, json::value, json::value v) {
  if (!v.is(json::kind::number)) token_fail("a number token must be a JSON number");
  return v.number();
}

consteval std::int32_t read_token(read_as<std::int32_t>, json::value, json::value v) {
  if (v.is(json::kind::number)) return rounded(v.number());
  if (v.string() == "normal") return 400;
  if (v.string() == "bold") return 700;
  token_fail("the font weight '" + str(v.string()) + "' is not a number, normal or bold");
}

consteval std::string_view read_token(read_as<std::string_view>, json::value, json::value v) {
  // A font family may be a list of fallbacks: the first one is used.
  if (v.is(json::kind::array)) return v.at(0).string();
  if (v.is(json::kind::object)) return "custom";  // a strokeStyle written as an object
  return v.string();
}

consteval bezier read_token(read_as<bezier>, json::value, json::value v) {
  if (!v.is(json::kind::array) || v.size() != 4)
    token_fail("a cubicBezier token must be a list of four numbers");
  return {v.at(0).number(), v.at(1).number(), v.at(2).number(), v.at(3).number()};
}

// A part of a composite token. It may be a literal or an alias to another token.
template <class T>
consteval T read_part(json::value root, json::value v, std::string_view what) {
  if (!v) token_fail("a composite token has no '" + str(what) + "'");
  if (v.is(json::kind::string) && v.string().starts_with("{") && v.string().ends_with("}"))
    return read_token(read_as<T>{}, root, follow_alias(root, v.string(), 0).value);
  return read_token(read_as<T>{}, root, v);
}

consteval shadow_token read_token(read_as<shadow_token>, json::value root, json::value v) {
  const auto s = v.is(json::kind::array) ? v.at(0) : v;
  return {read_part<lv_color_t>(root, s["color"], "color"),
          read_part<length>(root, s["offsetX"], "offsetX"),
          read_part<length>(root, s["offsetY"], "offsetY"),
          read_part<length>(root, s["blur"], "blur"),
          read_part<length>(root, s["spread"], "spread")};
}

consteval border_token read_token(read_as<border_token>, json::value root, json::value v) {
  return {read_part<lv_color_t>(root, v["color"], "color"),
          read_part<length>(root, v["width"], "width"),
          read_part<std::string_view>(root, v["style"], "style")};
}

consteval typography_token read_token(read_as<typography_token>, json::value root, json::value v) {
  return {read_part<std::string_view>(root, v["fontFamily"], "fontFamily"),
          read_part<length>(root, v["fontSize"], "fontSize"),
          read_part<std::int32_t>(root, v["fontWeight"], "fontWeight"),
          read_part<length>(root, v["letterSpacing"], "letterSpacing"),
          read_part<double>(root, v["lineHeight"], "lineHeight")};
}

consteval transition_token read_token(read_as<transition_token>, json::value root, json::value v) {
  return {read_part<duration>(root, v["duration"], "duration"),
          read_part<duration>(root, v["delay"], "delay"),
          read_part<bezier>(root, v["timingFunction"], "timingFunction")};
}

consteval gradient_token read_token(read_as<gradient_token>, json::value root, json::value v) {
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

template <class T>
inline constexpr bool is_token_leaf = false;
template <class T>
inline constexpr bool is_token_leaf<token<T>> = true;

template <class T>
struct token_payload;
template <class T>
struct token_payload<token<T>> {
  using type = T;
};

// ---- the forms before Design Tokens Format 2025.10 -----------------------------------------

// Whether `v` is written in a form that the standard replaced, for a value of type `kind`:
// "#22201c" for a color, "8px" or a bare number for a dimension or a duration.
consteval bool is_legacy_form(token_type kind, json::value v) {
  const bool alias = v.is(json::kind::string) && v.string().starts_with("{");
  switch (kind) {
    case token_type::color: return v.is(json::kind::string) && !alias;
    case token_type::dimension:
    case token_type::duration: return (v.is(json::kind::string) && !alias) || v.is(json::kind::number);
    default: return false;
  }
}

// What the standard writes in its place.
consteval std::string_view standard_form(token_type kind) {
  switch (kind) {
    case token_type::color: return "{\"colorSpace\": \"srgb\", \"components\": [0.2, 0.4, 1], \"hex\": \"#3366ff\"}";
    case token_type::duration: return "{\"value\": 200, \"unit\": \"ms\"}";
    default: return "{\"value\": 8, \"unit\": \"px\"}";
  }
}

consteval void fail_legacy(std::string_view where, token_type kind, json::value v) {
  token_fail("'" + str(where) + "' is written " + str(v.text()) + ", a form from before the Design Tokens Format 2025.10. "
             "The standard writes it as " + str(standard_form(kind)) + ". Run python3 tools/dtcg_fix.py on the token file, "
             "or set lv::config::tokens_legacy_values while the files convert");
}

// The members of a composite token that hold a color, a dimension or a duration.
struct composite_part {
  token_type owner;
  std::string_view member;
  token_type kind;
};

consteval void check_part(std::string_view where, std::string_view member, token_type kind, json::value v) {
  if (is_legacy_form(kind, v)) fail_legacy(str(where) + "' member '" + str(member), kind, v);
}

consteval void check_parts(std::string_view where, token_type owner, json::value v) {
  const std::initializer_list<composite_part> parts = {
      {token_type::border, "color", token_type::color}, {token_type::border, "width", token_type::dimension},
      {token_type::shadow, "color", token_type::color}, {token_type::shadow, "offsetX", token_type::dimension},
      {token_type::shadow, "offsetY", token_type::dimension}, {token_type::shadow, "blur", token_type::dimension},
      {token_type::shadow, "spread", token_type::dimension}, {token_type::typography, "fontSize", token_type::dimension},
      {token_type::typography, "letterSpacing", token_type::dimension},
      {token_type::transition, "duration", token_type::duration}, {token_type::transition, "delay", token_type::duration},
      {token_type::gradient, "color", token_type::color}};
  for (const composite_part& part : parts)
    if (part.owner == owner) check_part(where, part.member, part.kind, v[part.member]);
}

// A shadow may be a list of shadows and a gradient is a list of stops: each element is checked.
consteval void check_forms(std::string_view where, token_type kind, json::value v) {
  if (v.is(json::kind::array)) {
    for (const json::value element : v.elements()) check_forms(where, kind, element);
  } else if (is_legacy_form(kind, v)) {
    fail_legacy(where, kind, v);
  } else if (v.is(json::kind::object) && kind >= token_type::shadow && kind <= token_type::gradient) {
    check_parts(where, kind, v);
  }
}

// The strings of a vendor object, for the struct made by vendor_specs.
template <class G>
consteval G fill_vendor(json::value object) {
  G out{};
  auto walk = object.members().begin();
  template for (constexpr auto m : std::define_static_array(std::meta::nonstatic_data_members_of(
                    ^^G, std::meta::access_context::unchecked()))) {
    out.[:m:] = {json::value{(*walk).text}.string(), from_design_tokens{}};
    ++walk;
  }
  return out;
}

consteval bool is_named(std::meta::info member, std::string_view name) {
  return std::meta::identifier_of(member) == name;
}

// One token: its value is checked for a form from before the standard (unless the settings allow it) and read.
template <class M, bool Legacy>
consteval M fill_leaf(json::value root, std::string_view key, const resolved_token& resolved) {
  if constexpr (!Legacy) check_forms(key, resolved.type, resolved.value);
  return M{read_token(read_as<typename token_payload<M>::type>{}, root, resolved.value), from_design_tokens{}};
}

// Fills the generated struct G from its group. Struct members and used JSON
// members are in the same order, so they are walked together; the members that
// hold vendor data and "$extends" come last and have no JSON member of their own.
template <class G, const auto& Doc, bool Legacy>
consteval G fill_tokens(json::value group, token_type inherited) {
  const auto root = token_root<Doc>();
  if (auto t = group["$type"]) inherited = token_type_of(t.string());

  G out{};
  [[maybe_unused]] auto walk = group.members().begin();  // unused when the group has no members
  template for (constexpr auto m : std::define_static_array(std::meta::nonstatic_data_members_of(
                    ^^G, std::meta::access_context::unchecked()))) {
    using M = typename[:std::meta::type_of(m):];
    if constexpr (is_named(m, vendor_member)) {
      out.[:m:] = fill_vendor<M>(vendor_object(group));
    } else if constexpr (is_named(m, extends_member)) {
      out.[:m:] = {group["$extends"].is(json::kind::string) ? group["$extends"].string() : std::string_view{}};
    } else {
      while (!token_member_used(root, *walk, inherited)) ++walk;
      const json::member member = *walk;
      const json::value child{member.text};
      ++walk;
      if constexpr (is_token_leaf<M>) {
        out.[:m:] = fill_leaf<M, Legacy>(root, member.key, resolve_token(root, child, inherited));
      } else {
        out.[:m:] = fill_tokens<M, Doc, Legacy>(child, inherited);
      }
    }
  }
  return out;
}

// Where the root object starts in the document: after any leading white space.
template <const auto& Doc>
inline constexpr std::size_t token_root_offset =
    static_cast<std::size_t>(token_root<Doc>().text().data() - token_text<Doc>().data());

template <const auto& Doc, bool Legacy>
consteval auto load_tokens() {
  return fill_tokens<token_group_t<Doc, token_root_offset<Doc>, token_type::none>, Doc, Legacy>(
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
//
// A value in a form from before the Design Tokens Format 2025.10 ("#3366ff", "8px") is a compile error,
// unless the settings `O` name lv::config::tokens_legacy_values. tools/dtcg_fix.py rewrites a file.
template <const auto& Doc, class O = override>
  requires detail::embedded_text<decltype(Doc)>
inline constexpr auto tokens = detail::load_tokens<Doc, cfg<O>.tokens_legacy_values>();

}  // namespace lv
