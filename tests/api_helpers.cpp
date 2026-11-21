// The small functions of lv/detail/api.hpp that every message and every name lookup uses: what
// a name character is, a number in decimal, a text joined from parts, a prefix test, and the
// lookup of an LVGL function by name. All of it is constant evaluation, so the checks are static_asserts.
#include <lv/detail/api.hpp>

#include <string>
#include <string_view>

namespace {

using namespace lv::detail;

// The characters of a C name: letters, digits and the underscore, and nothing next to them in ASCII.
static_assert(is_name_char('a') && is_name_char('z') && is_name_char('A') && is_name_char('Z') && is_name_char('0') &&
              is_name_char('9') && is_name_char('_'));
static_assert(!is_name_char('`') && !is_name_char('{') && !is_name_char('@') && !is_name_char('[') && !is_name_char('/') &&
              !is_name_char(':') && !is_name_char('^') && !is_name_char('`') && !is_name_char('-') && !is_name_char(' '));

static_assert(str("") == "" && str("abc") == "abc");

// A number in decimal digits, with no leading zero, and zero as 0.
static_assert(decimal(0) == "0" && decimal(7) == "7" && decimal(10) == "10" && decimal(19) == "19" &&
              decimal(100) == "100" && decimal(1234567890) == "1234567890" && decimal(65535) == "65535");

static_assert(join({}) == "" && join({"a"}) == "a" && join({"a", "", "bc", "d"}) == "abcd");

// A prefix: the length and the character after "lv_" are tried before the whole compare.
static_assert(has_prefix("lv_label_set_text", "lv_label_"));
static_assert(has_prefix("lv_label_set_text", "lv_"));
static_assert(has_prefix("lv_label_set_text", ""));
static_assert(has_prefix("lv_label", "lv_label"));
static_assert(has_prefix("lv_a", "lv_a"));
static_assert(!has_prefix("lv_b", "lv_a"));
static_assert(!has_prefix("lv_label", "lv_labels"));  // the prefix is longer
static_assert(!has_prefix("lv_arc_set_value", "lv_label_"));
static_assert(!has_prefix("lv_labex", "lv_label"));
static_assert(!has_prefix("xx_label", "lv_"));
static_assert(!has_prefix("lv_label", "lv_labelx"));
static_assert(!has_prefix("lv", "lv_label") && !has_prefix("l", "lv_label") && !has_prefix("", "lv_x"));  // shorter than the prefix: no read past the end

// An LVGL function by name, or none.
static_assert(find("lv_label_set_text") != std::meta::info{});
static_assert(std::meta::identifier_of(find("lv_label_set_text")) == "lv_label_set_text");
static_assert(find("lv_label_set_nothing") == std::meta::info{});
static_assert(find("") == std::meta::info{});

// The same lookup, run when the argument is known.
static_assert(find_later<int>("lv_label_set_text") == find("lv_label_set_text"));
static_assert(find_later<int>("lv_label_set_nothing") == std::meta::info{});

// The names after a prefix, comma separated.
consteval bool lists_names() {
  const std::string led = names_after("lv_led_set_");
  const std::string none = names_after("lv_no_such_widget_set_");
  return led.find("color") != std::string::npos && led.find("brightness") != std::string::npos &&
         led.find("lv_") == std::string::npos && led.find(", ") != std::string::npos && led.front() != ',' && led.back() != ' ' &&
         none.empty();
}
static_assert(lists_names());

// A property of a widget: its own setter first, then the one every object has.
static_assert(std::meta::identifier_of(resolve("label", "set", "text")) == "lv_label_set_text");
static_assert(std::meta::identifier_of(resolve("label", "set", "width")) == "lv_obj_set_width");
static_assert(std::meta::identifier_of(resolve_create("label")) == "lv_label_create");

}  // namespace

int main() { return 0; }
