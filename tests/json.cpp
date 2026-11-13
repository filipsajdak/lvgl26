// The JSON reader is constexpr, so its tests are static assertions.
#include <lv/json.hpp>
using lv::json::value; using lv::json::kind;
constexpr std::string_view doc = R"({ "color": { "$type": "color", "primary": { "$value": "#3366ff" }, "n": [1, -2.5e1, true, null] },
  "space": { "md": { "$type": "dimension", "$value": { "value": 8, "unit": "px" } } }, "s": "a\"b" })";
constexpr auto v = *value::parse(doc);
static_assert(v.is(kind::object) && v.size() == 3);
static_assert(v["color"]["primary"]["$value"].string() == "#3366ff");
static_assert(v["color"]["n"].size() == 4 && v["color"]["n"].at(1).number() == -25.0);
static_assert(v["color"]["n"].at(2).boolean() && v["color"]["n"].at(3).is(kind::null));
static_assert(v["space"]["md"]["$value"]["value"].number() == 8 && v["space"]["md"]["$value"]["unit"].string() == "px");
static_assert(v.member_at(2).key == "s" && value{v.member_at(2).text}.string() == "a\\\"b");
static_assert(!v["nope"] && !v["color"]["primary"]["x"]);
static_assert(!value::parse("{ \"a\": }") && !value::parse("[1, 2") && !value::parse("{} x") && !value::parse(""));
static_assert(value::parse("[]")->size() == 0 && value::parse(" 12 ")->number() == 12);
// The same reader at run time, so that the code the program really executes is
// tested (and counted by the coverage report), not only its constant evaluation.
#include "check.hpp"

#include <string>

int main() {
  const std::string text(doc);  // not a constant: nothing below can be folded
  const auto parsed = value::parse(text);
  check(parsed.has_value());
  if (parsed) {
    const value& r = *parsed;
    check(r.is(kind::object) && r.size() == 3);
    check(r["color"]["primary"]["$value"].string() == "#3366ff");
    const auto n = r["color"]["n"];
    check(n.size() == 4 && n.at(0).number() == 1 && n.at(1).number() == -25.0);
    check(n.at(2).boolean() && n.at(2).is(kind::boolean) && n.at(3).is(kind::null));
    check(r["space"]["md"]["$value"]["value"].number() == 8);
    check(r.member_at(2).key == "s" && !r["nope"] && !r.at(0));
    check(!value{}.is(kind::object) && value{}.type() == kind::invalid);
  }

  // Numbers: sign, fraction, exponent with either sign.
  const auto number = [](std::string s) { return value::parse(s)->number(); };
  check(number("12") == 12 && number("-0.5") == -0.5);
  check(number("2e3") == 2000 && number("2E+3") == 2000 && number("25e-1") == 2.5);

  // A huge exponent gives zero or infinity, not an overflow. (Found by fuzzing.)
  check(number("1e-99999999999") == 0 && number("1e99999999999") > 1e300);

  // Numbers follow the JSON grammar exactly.
  for (const char* bad : {"+1", ".5", "1.", "01", "1e", "1e+", "1e2e3", "1-2", "-", "--1", "1.2.3"})
    check(!value::parse(std::string(bad)).has_value());
  for (const char* good : {"0", "-0", "0.5", "10", "1e5", "1E-5", "-1.25e+10"})
    check(value::parse(std::string(good)).has_value());

  // Malformed documents are refused.
  for (const char* bad : {"", "{", "[1, 2", "{\"a\": }", "{\"a\" 1}", "{1: 2}", "[1 2]", "{} x",
                          "\"open", "tru"})
    check(!value::parse(std::string(bad)).has_value());
  for (const char* good : {"[]", "{}", " 12 ", "null", "false", "\"a\\\"b\"", "[[[]]]"})
    check(value::parse(std::string(good)).has_value());

  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
