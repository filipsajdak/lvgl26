// Prints the reference of lvgl26 as JSON: every widget with its properties and
// events, what each takes, the names of each enumerator, and the members of a style.
//
// It is the data behind editor completion, the agents' tools and the reference pages.
// Everything in it is read by reflection from the LVGL this program is built with,
// through the functions that generate the library's own structs: it cannot disagree
// with what compiles.
//
//   lv_reference > docs/reference/lvgl-v9.6.0.json
//
// The test `reference_data` fails when the committed file differs from the output.

#include <lv/lv.hpp>

#include <cstdio>
#include <meta>
#include <span>
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

// The name of a type as LVGL's headers write it.
consteval std::string type_name(std::meta::info t) {
  return str(std::meta::has_identifier(t) ? std::meta::identifier_of(t) : std::meta::display_string_of(t));
}

// The enum that a value of type `t` names, or a null reflection.
consteval std::meta::info enum_of(std::meta::info t) {
  return std::meta::is_enum_type(std::meta::dealias(t)) ? t : std::meta::info{};
}

// The names of an enum's enumerators as user code writes them: without what all of
// them share, in lower case.
consteval std::string enumerator_names(std::meta::info e) {
  std::vector<lv::detail::enum_name> names;
  for (auto x : std::meta::enumerators_of(std::meta::dealias(e)))
    names.push_back({std::define_static_string(std::meta::identifier_of(x)), 0});
  const auto prefix = lv::detail::prefix_of(names);
  std::string out = "[";
  for (std::size_t i = 0; i < names.size(); ++i)
    out += (i ? ", " : "") + quoted(lv::detail::short_name(names[i].name, prefix));
  return out + "]";
}

// What one value of a member takes: how to write it, and its enum when it is one.
consteval std::string value_json(std::meta::info t, std::vector<std::meta::info>& enums) {
  const auto e = enum_of(t);
  std::string out = "{\"write\": " + quoted(e != std::meta::info{} ? "one of the enum's names, in quotes" : lv::detail::how_to_write(t));
  if (e != std::meta::info{}) {
    out += ", \"enum\": " + quoted(type_name(e));
    bool known = false;
    for (auto seen : enums) known = known || type_name(seen) == type_name(e);
    if (!known) enums.push_back(e);
  }
  return out + "}";
}

// The values of a member whose type is lv::arg<Ts...>.
consteval std::string takes_json(std::meta::info arg, std::vector<std::meta::info>& enums) {
  std::string out = "[";
  bool first = true;
  for (auto t : std::meta::template_arguments_of(arg)) {
    out += (first ? "" : ", ") + value_json(t, enums);
    first = false;
  }
  return out + "]";
}

consteval std::string_view kind_of(const lv::detail::member_item& m) {
  if (m.target == std::meta::info{}) return "id";
  if (m.name.starts_with("style")) return "style";
  return std::meta::is_enumerator(m.target) ? "event" : "property";
}

consteval std::string member_json(const lv::detail::member_item& m, std::vector<std::meta::info>& enums) {
  const auto kind = kind_of(m);
  std::string out = "    {\"name\": " + quoted(m.name) + ", \"kind\": " + quoted(kind);
  if (kind == "property") {
    out += ", \"sets\": " + quoted(std::meta::identifier_of(m.target));
    out += ", \"takes\": " + takes_json(m.type, enums);
  }
  return out + "}";
}

// The members of `widget`: all of them for obj, and for another widget the ones obj lacks.
consteval std::string members_json(std::string_view widget, std::vector<std::meta::info>& enums) {
  const std::string own = "lv_" + str(widget) + "_";
  std::string out;
  for (const auto& m : lv::detail::members<>(widget)) {
    const bool of_widget = m.target != std::meta::info{} && std::meta::is_function(m.target) &&
                           std::meta::identifier_of(m.target).starts_with(own);
    if (widget != "obj" && !of_widget) continue;
    out += (out.empty() ? "" : ",\n") + member_json(m, enums);
  }
  return out;
}

// Every widget: what LVGL can create with one parent and nothing else.
consteval std::vector<std::string> widget_names() {
  std::vector<std::string> out;
  for (auto f : lv::detail::api_table<>) {
    const auto id = std::meta::identifier_of(f);
    if (!id.ends_with("_create") || std::meta::parameters_of(f).size() != 1) continue;
    if (std::meta::return_type_of(f) != ^^lv_obj_t*) continue;
    if (std::meta::type_of(std::meta::parameters_of(f)[0]) != ^^lv_obj_t*) continue;
    out.push_back(str(id.substr(3, id.size() - 3 - 7)));
  }
  return out;
}

consteval std::string style_json(std::vector<std::meta::info>& enums) {
  std::string out;
  for (std::size_t i = 0; i < lv::detail::style_table<>.size(); i += 3) {
    const auto name = std::meta::identifier_of(lv::detail::style_table<>[i + 1]).substr(lv::detail::style_prefix.size());
    out += (out.empty() ? "" : ",\n") + ("    {\"name\": " + quoted(name) + ", \"takes\": " +
                                          takes_json(lv::detail::style_table<>[i], enums) + "}");
  }
  return out;
}

// The pieces of the document, each small enough to be a constant of its own.
consteval std::span<const char* const> document() {
  std::vector<std::meta::info> enums;
  std::vector<const char*> out;
  const auto add = [&](const std::string& piece) { out.push_back(std::define_static_string(piece)); };
  add("  \"base\": [\n" + members_json("obj", enums) + "\n  ],\n  \"widgets\": {\n");
  bool first = true;
  for (const auto& widget : widget_names()) {
    if (widget == "obj") continue;
    add((first ? "" : ",\n") + ("    " + quoted(widget) + ": [\n" + members_json(widget, enums) + "\n    ]"));
    first = false;
  }
  add("\n  },\n  \"style\": [\n" + style_json(enums) + "\n  ],\n  \"enums\": {\n");
  // No property takes a part, and tools that read looks need the names: a group named like a part
  // is a layer (tools/look_groups.py).
  enums.push_back(^^lv_part_t);
  for (std::size_t i = 0; i < enums.size(); ++i)
    add((i ? ",\n" : "") + ("    " + quoted(type_name(enums[i])) + ": " + enumerator_names(enums[i])));
  add("\n  }\n}\n");
  return std::define_static_array(out);
}

constexpr auto pieces = document();

}  // namespace

int main() {
  std::printf("{\n  \"lvgl\": \"%d.%d.%d\",\n", lv_version_major(), lv_version_minor(), lv_version_patch());
  for (const char* piece : pieces) std::fputs(piece, stdout);
  return 0;
}
