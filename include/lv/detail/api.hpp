#pragma once

// The reflected LVGL C API.
//
// The global namespace is scanned once, here, where lvgl.h is visible. Everything
// else searches the resulting table by name. A template instantiated by a module
// importer cannot scan the global namespace itself (see spikes/README.md).

#include <lvgl/lvgl.h>

#include <exception>
#include <meta>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace lv::detail {

consteval std::vector<std::meta::info> scan_api() {
  std::vector<std::meta::info> v;
  for (auto m : std::meta::members_of(^^::, std::meta::access_context::unchecked()))
    if (std::meta::is_function(m) && std::meta::has_identifier(m) &&
        std::meta::identifier_of(m).starts_with("lv_"))
      v.push_back(m);
  return v;
}

// Every lv_* function, in declaration order. Sorting ~2000 names at compile time
// costs more than the linear searches it would save.
inline constexpr std::span<const std::meta::info> api = std::define_static_array(scan_api());

// The compile error a user sees for a wrong widget or property name.
struct unknown_name : std::exception {
  const char* msg;
  consteval explicit unknown_name(std::string_view m) : msg(std::define_static_string(m)) {}
  constexpr const char* what() const noexcept override { return msg; }
};

// The function called `name`, or a null reflection.
consteval std::meta::info find(std::string_view name) {
  for (auto f : api)
    if (std::meta::identifier_of(f) == name) return f;
  return {};
}

// The names that follow `prefix` in the API, comma separated.
consteval std::string names_after(std::string_view prefix) {
  std::string out;
  for (auto f : api) {
    auto id = std::meta::identifier_of(f);
    if (!id.starts_with(prefix)) continue;
    if (!out.empty()) out += ", ";
    out += id.substr(prefix.size());
  }
  return out;
}

// lv_<widget>_<verb>_<prop>, falling back to lv_obj_<verb>_<prop>.
consteval std::meta::info resolve(std::string_view widget, std::string_view verb,
                                  std::string_view prop) {
  const std::string own = "lv_" + std::string(widget) + "_" + std::string(verb) + "_";
  const std::string base = "lv_obj_" + std::string(verb) + "_";
  if (auto f = find(own + std::string(prop)); f != std::meta::info{}) return f;
  if (auto f = find(base + std::string(prop)); f != std::meta::info{}) return f;

  std::string msg = "lv: widget '" + std::string(widget) + "' has no property '" +
                    std::string(prop) + "' to " + std::string(verb) + ". Its own: " +
                    names_after(own) + ".";
  if (widget != "obj") msg += " Every '" + base + "*' property of obj works too.";
  throw unknown_name(msg);
}

// lv_<widget>_create.
consteval std::meta::info resolve_create(std::string_view widget) {
  if (auto f = find("lv_" + std::string(widget) + "_create"); f != std::meta::info{}) return f;

  std::string valid;
  for (auto f : api) {
    auto id = std::meta::identifier_of(f);
    if (!id.ends_with("_create") || std::meta::parameters_of(f).size() != 1) continue;
    if (std::meta::return_type_of(f) != ^^lv_obj_t*) continue;
    if (!valid.empty()) valid += ", ";
    valid += id.substr(3, id.size() - 3 - 7);
  }
  throw unknown_name("lv: there is no widget '" + std::string(widget) + "'. Valid: " + valid + ".");
}

}  // namespace lv::detail
