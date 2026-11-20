// Where a JSON document stops being well-formed, for the message of a settings file (lv/detail/json_fault.hpp).
#include <lv/detail/json_fault.hpp>

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

}  // namespace

int main() { return 0; }
