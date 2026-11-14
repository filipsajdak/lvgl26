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

  // Cases that mutation testing showed were missing.
  check(number("1.25") == 1.25 && number("0.125") == 0.125);  // each fraction digit has its place
  check(value::parse(std::string("\r\n[\r1\r]\r\n")).has_value());  // a carriage return is space
  for (const char* bad : {"1e 5", "1e*5", "{\"a\" 11}", "{\"a\";1}", "{\"a\"}", "{\"a}"})
    check(!value::parse(std::string(bad)).has_value());
  // Nesting: 65 levels are read, 66 are refused (and so is anything deeper).
  const auto nested = [](std::size_t n) {
    return value::parse(std::string(n, '[') + std::string(n, ']')).has_value();
  };
  check(nested(65) && !nested(66) && !nested(1000));
  const auto nested_objects = [](std::size_t n) {
    std::string open;
    for (std::size_t i = 0; i < n; ++i) open += "{\"k\":";
    return value::parse(open + "1" + std::string(n, '}')).has_value();
  };
  check(nested_objects(64) && !nested_objects(65));

  // An element can start with any character; an index past the end gives nothing.
  const auto count = [](const char* t) { return value::parse(std::string(t))->size(); };
  check(count("[true]") == 1 && count("[{}]") == 1 && count("[null, false]") == 2);
  check(count("{\"a\": {}}") == 1 && count("[]") == 0 && count("{}") == 0);
  if (parsed) {
    const auto n = (*parsed)["color"]["n"];
    check(!n.at(4) && !n.at(100) && n.at(3));
    check(parsed->member_at(3).key.empty() && parsed->member_at(100).key.empty());
  }
  // Only space, tab, line feed and carriage return are space; only the right
  // character closes a container.
  for (const char* bad : {"[\v1]", "\f1", "[\x01]", "[1}", "[1 x]", "{\"a\":1]", "{\"a\":1 ~"})
    check(!value::parse(std::string(bad)).has_value());

  // Malformed documents are refused.
  for (const char* bad : {"", "{", "[1, 2", "{\"a\": }", "{\"a\" 1}", "{1: 2}", "[1 2]", "{} x",
                          "\"open", "tru"})
    check(!value::parse(std::string(bad)).has_value());
  for (const char* good : {"[]", "{}", " 12 ", "null", "false", "\"a\\\"b\"", "[[[]]]"})
    check(value::parse(std::string(good)).has_value());

  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
