// Prints the settings of lvgl26 as data, read by reflection from lv::config: every section and
// key of a settings file, its type, its default and its description. With `schema`, prints a
// JSON Schema of the file instead, for editors that complete and check it.
//
//   lv_settings_reference > docs/reference/settings.json
//   lv_settings_reference schema > docs/reference/lvgl26-settings.schema.json
//
// The tests `settings_reference` and `settings_schema` fail when a committed file differs.
#include <lv/lv.hpp>

#include <cstdio>
#include <meta>
#include <string>
#include <string_view>
#include <vector>

namespace {

using lv::detail::str;

consteval std::string quoted(std::string_view text) {
  std::string out = "\"";
  for (const char c : text) {
    if (c == '"' || c == '\\') out.push_back('\\');
    out.push_back(c);
  }
  return out + "\"";
}

// The members of S, in the order they are declared.
template <class S>
consteval auto members_of() {
  return std::define_static_array(std::meta::nonstatic_data_members_of(^^S, std::meta::access_context::unchecked()));
}

// The text of the lv::described annotation of a member.
template <std::meta::info Member>
consteval std::string description_of() {
  std::string out;
  template for (constexpr auto a : std::define_static_array(std::meta::annotations_of(Member))) {
    using annotation = [:std::meta::type_of(a):];
    out = std::meta::extract<annotation>(a).text;
  }
  if (out.empty()) lv::detail::fail("lv_settings_reference: " + str(std::meta::identifier_of(Member)) + " has no description");
  return out;
}

consteval std::string decimal(std::size_t n) { return lv::detail::decimal(n); }

consteval std::string unit_list(std::span<const lv::settings_unit> units) {
  std::string out;
  for (const lv::settings_unit& u : units) out += (out.empty() ? "" : ", ") + quoted(u.name);
  return out;
}

// The entry of one setting in the reference: its type, its default, its units.
consteval std::string reference_value(bool value) {
  return "\"type\": \"boolean\", \"default\": " + std::string{value ? "true" : "false"};
}

template <lv::detail::settings_amount Q>
consteval std::string reference_value(const Q& value) {
  return "\"type\": \"amount\", \"default\": {\"value\": " + decimal(value.value) + ", \"unit\": " +
         quoted(Q::units[0].name) + "}, \"units\": [" + unit_list(Q::units) + "]";
}

template <class S>
consteval std::string reference_section(const S& defaults, std::string_view indent) {
  std::string out;
  template for (constexpr auto m : members_of<S>()) {
    out += str(indent) + "{\"key\": " + quoted(std::meta::identifier_of(m)) + ", " + reference_value(defaults.[:m:]) +
           ",\n" + str(indent) + " \"description\": " + quoted(description_of<m>()) + "},\n";
  }
  out.resize(out.size() - 2);
  return out + "\n";
}

// A template, because GCC 16.2 reports each expansion of a `template for` in a plain function
// as shadowing the one before it.
template <class C = lv::config>
consteval std::string reference() {
  constexpr C defaults{};
  std::string out = "{\n  \"file\": \"lvgl26.json\",\n  \"sections\": [\n";
  template for (constexpr auto section : members_of<C>()) {
    out += "    {\"key\": " + quoted(std::meta::identifier_of(section)) + ",\n     \"description\": " +
           quoted(description_of<section>()) + ",\n     \"settings\": [\n" + reference_section(defaults.[:section:], "       ") +
           "     ]},\n";
  }
  out.resize(out.size() - 2);
  return out + "\n  ]\n}\n";
}

// The schema of one setting.
consteval std::string schema_value(bool value) {
  return "\"type\": \"boolean\", \"default\": " + std::string{value ? "true" : "false"};
}

template <lv::detail::settings_amount Q>
consteval std::string schema_value(const Q& value) {
  return "\"type\": \"object\", \"additionalProperties\": false, \"required\": [\"value\", \"unit\"],\n"
         "          \"properties\": {\"value\": {\"type\": \"number\", \"minimum\": 0}, \"unit\": {\"enum\": [" +
         unit_list(Q::units) + "]}},\n          \"default\": {\"value\": " + decimal(value.value) +
         ", \"unit\": " + quoted(Q::units[0].name) + "}";
}

template <class S>
consteval std::string schema_section(const S& defaults) {
  std::string out;
  template for (constexpr auto m : members_of<S>()) {
    out += "        " + quoted(std::meta::identifier_of(m)) + ": {\"description\": " + quoted(description_of<m>()) +
           ",\n          " + schema_value(defaults.[:m:]) + "},\n";
  }
  out.resize(out.size() - 2);
  return out + "\n";
}

template <class C = lv::config>
consteval std::string schema() {
  constexpr C defaults{};
  std::string out =
      "{\n  \"$schema\": \"https://json-schema.org/draft/2020-12/schema\",\n"
      "  \"$id\": \"https://raw.githubusercontent.com/filipsajdak/lvgl26/main/docs/reference/lvgl26-settings.schema.json\",\n"
      "  \"title\": \"lvgl26 settings\",\n"
      "  \"description\": \"The settings file of a program built with lvgl26: lvgl26_settings(target lvgl26.json).\",\n"
      "  \"type\": \"object\",\n  \"additionalProperties\": false,\n  \"properties\": {\n"
      "    \"$schema\": {\"type\": \"string\"},\n";
  template for (constexpr auto section : members_of<C>()) {
    out += "    " + quoted(std::meta::identifier_of(section)) + ": {\n      \"description\": " + quoted(description_of<section>()) +
           ",\n      \"type\": \"object\",\n      \"additionalProperties\": false,\n      \"properties\": {\n" +
           schema_section(defaults.[:section:]) + "      }\n    },\n";
  }
  out.resize(out.size() - 2);
  return out + "\n  }\n}\n";
}

constexpr std::string_view reference_text = std::define_static_string(reference<>());
constexpr std::string_view schema_text = std::define_static_string(schema<>());

}  // namespace

int main(int argc, char** argv) {
  const bool wants_schema = argc > 1 && std::string_view{argv[1]} == "schema";
  const std::string_view text = wants_schema ? schema_text : reference_text;
  std::fwrite(text.data(), 1, text.size(), stdout);
  return 0;
}
