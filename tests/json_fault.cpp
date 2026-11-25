// Where a JSON document stops being well-formed, for the message of a settings file (lv/detail/json_fault.hpp).
#include <lv/detail/json_fault.hpp>

#include <string>
#include <string_view>

namespace {

using lv::json::detail::find_fault;
using lv::json::detail::npos;
using namespace std::string_view_literals;

consteval bool at(std::string_view doc, std::size_t where, std::string_view expected) {
  const auto f = find_fault(doc);
  return f.at == where && f.expected == expected;
}

static_assert(find_fault(R"({"a": [1, 2.5, "x", true, null, {"b": {}}]})").at == npos);
static_assert(find_fault("  \n[]  ").at == npos);
static_assert(at(R"({"allow": {)", 11, "a key in quotes or '}'"));
static_assert(at(R"({"a": 1,)", 8, "a key in quotes"));
static_assert(at(R"({"a" 1})", 5, "':' after the key"));
static_assert(at("[1 2]", 3, "',' or ']' after the value"));
static_assert(at(R"({"a": 1 "b"})", 8, "',' or '}' after the value"));
static_assert(at(R"({"a": })", 6, "a value: a string, a number, true, false, null, an object or a list"));
static_assert(at("", 0, "a value"));
static_assert(at(R"("abc)", 0, "the closing quote of this string"));
static_assert(at(R"({"a": 1} x)", 9, "the end of the file"));
static_assert(at("{\n  \"a\": tru\n}", 9, "a value: a string, a number, true, false, null, an object or a list"));

consteval std::string_view described(std::string_view doc) {
  return std::define_static_string(lv::json::detail::describe_fault(doc, find_fault(doc)));
}
static_assert(described(R"({"allow": {)") == "At line 1, column 12 it expects a key in quotes or '}', and the file ends there."sv);
static_assert(described("{\n  \"a\": tru\n}") ==
              "At line 2, column 8 it expects a value: a string, a number, true, false, null, an object or a list, and it has 't' there."sv);

constexpr auto a_value = "a value: a string, a number, true, false, null, an object or a list"sv;

// A document that ends inside a container, at each place it can end.
static_assert(at("[", 1, "a value"));
static_assert(at("{", 1, "a key in quotes or '}'"));
static_assert(at("[1", 2, "',' or ']' after the value"));
static_assert(at("[1,", 3, "a value"));
static_assert(at("[1, ", 4, "a value"));
static_assert(at("{\"a\"", 4, "':' after the key"));
static_assert(at("{\"a\":", 5, "a value"));
static_assert(at("{\"a\":1", 6, "',' or '}' after the value"));
static_assert(at("{\"a\":1,", 7, "a key in quotes"));
static_assert(at("{\"abc", 1, "the closing quote of this string"));

// A mistake after a comma, a colon or a value, with white space around.
static_assert(at("{\"a\":1,}", 7, "a key in quotes"));
static_assert(at("[1,]", 3, a_value));
static_assert(at("[,]", 1, a_value));
static_assert(at("{\"a\":   }", 8, a_value));
static_assert(find_fault("{ \"a\" : 1 , \"b\" : [ 1 , 2 ] }").at == npos);
static_assert(find_fault("[\n1,\n2\n]").at == npos);
static_assert(at("{1: 2}", 1, "a key in quotes or '}'"));
static_assert(at("{\"a\": 1, 2: 3}", 9, "a key in quotes"));
static_assert(at("{\"a\": 1 ]", 8, "',' or '}' after the value"));
static_assert(at("[1 }", 3, "',' or ']' after the value"));
static_assert(at("[1 ,]", 4, a_value));
static_assert(at("{\"a\" 1}", 5, "':' after the key"));
static_assert(at("{\"a\"}", 4, "':' after the key"));
static_assert(find_fault("{\"a\"  :1}").at == npos);
static_assert(find_fault("{\"a\":\n1}").at == npos);
static_assert(find_fault("{}").at == npos && find_fault("[]").at == npos && find_fault("[ ]").at == npos && find_fault("{ }").at == npos);
static_assert(find_fault("[[],{},[{}]]").at == npos);
static_assert(find_fault("{\"a\":{\"b\":[1,{\"c\":2}]}}").at == npos);

// A scalar: a string, a word, a number.
static_assert(find_fault("\"a\"").at == npos && find_fault("true").at == npos && find_fault("null").at == npos && find_fault("-1.5e3").at == npos);
static_assert(at("x", 0, a_value));
static_assert(at("[\"a]", 1, "the closing quote of this string"));
static_assert(at("[tru]", 1, a_value));
static_assert(at("[1] 2", 4, "the end of the file"));
static_assert(at("{} {}", 3, "the end of the file"));
static_assert(at("  \n  x", 5, a_value));
static_assert(at("[]\n\n  x", 6, "the end of the file"));

// Characters just below the quote and the line break are neither: a key and a value start with more than "not above".
static_assert(at("{!}", 1, "a key in quotes or '}'"));
static_assert(at("!", 0, a_value));
static_assert(described("[\t1 2]") == "At line 1, column 5 it expects ',' or ']' after the value, and it has '2' there."sv);

// How deep a document may nest: 64 levels, as skip_value.
consteval std::string nested(std::size_t levels) {
  std::string text;
  for (std::size_t i = 0; i < levels; ++i) text += '[';
  for (std::size_t i = 0; i < levels; ++i) text += ']';
  return text;
}
consteval std::size_t fault_of_nested(std::size_t levels) { return find_fault(std::define_static_string(nested(levels))).at; }
static_assert(fault_of_nested(1) == npos);
static_assert(fault_of_nested(64) == npos);
static_assert(fault_of_nested(65) == npos);
static_assert(fault_of_nested(66) == 65);
static_assert(fault_of_nested(67) == 65);

consteval std::string_view deep_message() {
  const auto text = std::define_static_string(nested(70));
  return find_fault(text).expected;
}
static_assert(deep_message() == "fewer levels: a document nests 64 at most"sv);
constexpr auto deepest = lv::json::detail::deepest_nesting;
static_assert(deepest == 64);

// The place in words: the line, the column and what is there, on every line.
static_assert(described("[1 2]") == "At line 1, column 4 it expects ',' or ']' after the value, and it has '2' there."sv);
static_assert(described("[1,\n 2 3]") == "At line 2, column 4 it expects ',' or ']' after the value, and it has '3' there."sv);
static_assert(described("[\n\n\n x]") == "At line 4, column 2 it expects a value: a string, a number, true, false, null, an object or a list, and it has 'x' there."sv);
static_assert(described("x") == "At line 1, column 1 it expects a value: a string, a number, true, false, null, an object or a list, and it has 'x' there."sv);
static_assert(described("") == "At line 1, column 1 it expects a value, and the file ends there."sv);
static_assert(described("[\n") == "At line 2, column 1 it expects a value, and the file ends there."sv);
static_assert(described("[1]\n\n x") == "At line 3, column 2 it expects the end of the file, and it has 'x' there."sv);

}  // namespace

int main() { return 0; }
