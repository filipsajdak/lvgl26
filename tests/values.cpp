// The value types and the helpers behind props are compile-time code, so their
// tests are static assertions.
#include <lv/lv.hpp>

#include <concepts>
#include <cstdio>
#include <meta>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace lv::units;
using std::string_view;

// A text binding format becomes LVGL's printf form. Only "{}" is the place of the
// value; a brace on its own is plain text, and a percent sign is escaped.
static_assert(string_view(lv::text_format("{} km/h").printf) == "%d km/h");
static_assert(string_view(lv::text_format("n={}").printf) == "n=%d");
static_assert(string_view(lv::text_format("{}% of 100%").printf) == "%d%% of 100%%");
static_assert(string_view(lv::text_format("x} {a ~} {~ {}").printf) == "x} {a ~} {~ %d");

// How an error message says what to write for a type.
static_assert(lv::detail::how_to_write(^^lv::angle) == "an angle (90 * lv::deg)");
static_assert(lv::detail::how_to_write(^^lv::duration) == "a time (300 * lv::ms)");
static_assert(lv::detail::how_to_write(^^lv::opa) == "an opacity (50 * lv::pct)");
static_assert(lv::detail::how_to_write(^^lv::strict<bool>) == "true or false");
static_assert(lv::detail::how_to_write(^^lv::strict<int>) == "a whole number (not bool, not floating point)");
static_assert(lv::detail::how_to_write(^^lv::styled<lv::angle>) ==
              "a design token (for example tok.space.md) or lv::raw(...), holding an angle (90 * lv::deg)");
static_assert(lv::detail::how_to_write(^^double) == "double");
static_assert(lv::detail::how_to_write_unit(^^double).empty());

// The unit of a number is taken from the name of the property or of its parameter.
static_assert(lv::detail::unit_of("bg_opa", "value") == ^^lv::opa);
static_assert(lv::detail::unit_of("value", "opa") == ^^lv::opa);
static_assert(lv::detail::unit_of("rotation", "value") == ^^lv::angle);
static_assert(lv::detail::unit_of("value", "start_angle") == ^^lv::angle);
static_assert(lv::detail::unit_of("anim_duration", "value") == ^^lv::duration);
static_assert(lv::detail::unit_of("pos", "x") == ^^lv::horizontal);
static_assert(lv::detail::unit_of("pos", "y") == ^^lv::vertical);
static_assert(lv::detail::unit_of("value", "value") == std::meta::info{});
static_assert(lv::detail::has("bg_opa", "opa") && !lv::detail::has("bg", "opa"));
static_assert(lv::detail::is_keyword("long") && !lv::detail::is_keyword("long_mode"));

// Sorting by name.
struct item {
  string_view name;
};
consteval bool sorts() {
  std::vector<item> items{{"c"}, {"a"}, {"b"}, {"a"}};
  lv::detail::sort_by_name(items);
  return items[0].name == "a" && items[1].name == "a" && items[2].name == "b" && items[3].name == "c";
}
static_assert(sorts());

// A property value can be copied, also from a variable that is not const.
constexpr bool copies() {
  lv::arg<lv::strict<int>> a{7};
  lv::arg<lv::strict<int>> b{a};
  return b.set && std::get<0>(b.v).raw() == 7;
}
static_assert(copies());

// A number token for an opacity is a share from 0 to 1, rounded to whole percent.
constexpr lv::token<double> number(double v) { return {v, lv::detail::from_design_tokens{}}; }
static_assert(lv::styled<lv::opa>{number(1.0)}.raw() == 255);
static_assert(lv::styled<lv::opa>{number(0.996)}.raw() == 255);
static_assert(lv::styled<lv::opa>{number(0.494)}.raw() == lv::opa{49 * pct}.raw());

// A number token for a whole-number value must be whole.
static_assert(lv::styled<lv::angle>{number(90.0)}.raw() == 90);
static_assert(lv::styled<lv::angle>{number(-90.0)}.raw() == -90);
static_assert(lv::detail::whole_number<int>(0.0) == 0 && lv::detail::whole_number<std::int8_t>(-7.0) == -7);

// A whole-number token is only for a value whose raw form is an integer.
struct ratio {
  constexpr double raw() const { return 0; }
};
static_assert(std::constructible_from<lv::styled<lv::angle>, const lv::token<double>&>);
static_assert(!std::constructible_from<lv::styled<ratio>, const lv::token<double>&>);

// The message for a wrong value says what was given, or that nothing was.
constexpr string_view gave_nothing = lv::detail::wrong_value<lv::angle>::message<>();
constexpr string_view gave_an_int = lv::detail::wrong_value<lv::angle>::message<int>();
static_assert(gave_nothing.ends_with("You gave nothing."));
// Searched as a std::string: with -fsanitize=undefined, GCC 16 does not treat a search
// in a view of a static string as a constant.
static_assert(gave_an_int.ends_with("You gave int.") &&
              lv::detail::str(gave_an_int).find("nothing") == std::string::npos);

// A plain value is an integer, bool or enum, and not a character.
enum class gear { low, high };
static_assert(lv::detail::is_plain_value_type(^^int) && lv::detail::is_plain_value_type(^^bool));
static_assert(lv::detail::is_plain_value_type(^^gear));
static_assert(!lv::detail::is_plain_value_type(^^char) && !lv::detail::is_plain_value_type(^^char8_t));
static_assert(!lv::detail::is_plain_value_type(^^double));

// A named value is a class with exactly one public member that is a plain value.
struct one_member {
  int value;
};
struct two_members {
  int a;
  int b;
};
struct no_members {};
class hidden_member {
  int value = 0;
};
struct real_member {
  double value;
};
static_assert(lv::detail::strong_value<one_member>);
static_assert(!lv::detail::strong_value<two_members> && !lv::detail::strong_value<no_members>);
static_assert(!lv::detail::strong_value<hidden_member> && !lv::detail::strong_value<real_member>);
static_assert(!lv::detail::strong_value<int>);

// A unit is inferred from a parameter's name for a number, never for a bool.
[[maybe_unused]] void set_width(lv_obj_t*, [[maybe_unused]] bool width, [[maybe_unused]] int32_t height,
                                [[maybe_unused]] int count) {}
consteval std::meta::info type_for(string_view prop, std::size_t parameter) {
  return lv::detail::value_type(prop, std::meta::parameters_of(^^set_width)[parameter]);
}
static_assert(type_for("width", 1) == ^^lv::strict<bool>);
static_assert(type_for("height", 2) == lv::detail::unit_of("height", "height"));
static_assert(type_for("height", 2) != ^^lv::strict<int32_t>);
static_assert(type_for("count", 3) == ^^lv::strict<int>);

}  // namespace

int main() { std::printf("ok\n"); }
