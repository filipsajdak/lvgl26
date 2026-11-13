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
int main() { return 0; }
