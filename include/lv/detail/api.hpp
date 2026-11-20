#pragma once

// The reflected LVGL C API.
//
// The global namespace is scanned once in a file, where lvgl.h is visible, and only when
// the file first needs a function of LVGL by name. Everything else searches the resulting
// table by name. A template instantiated by a module
// importer cannot scan the global namespace itself (see spikes/README.md).

#include <lvgl/lvgl.h>

#include <meta>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace lv::detail {

// A std::string from a view. Use this instead of std::string(view) or append(view) in
// consteval code: with -fsanitize=undefined, GCC 16 does not treat the null check inside
// them as a constant when the view points at a static string. The constructor from two
// iterators has no such check, and costs a quarter of a push_back for each character.
constexpr std::string str(std::string_view v) { return std::string(v.begin(), v.end()); }

// A character of a C or C++ name.
consteval bool is_name_char(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

// Whether `m` is declared on a line of a file: GCC's builtins are at line 0 or 1, and about
// half of the functions in the global namespace. The line is free to ask for; the first
// identifier_of of a declaration is not. tests/api_table.cpp checks that no lv_ function is lost.
consteval bool declared_in_a_file(std::meta::info m) { return std::meta::source_location_of(m).line() > 1; }

// A template on D so that the scan is not run where this header is read.
template <class D>
consteval std::vector<std::meta::info> scan_api() {
  std::vector<std::meta::info> v;
  for (auto m : std::meta::members_of(^^::, std::meta::access_context::unchecked()))
    if (std::meta::is_function(m) && declared_in_a_file(m) && std::meta::has_identifier(m) &&
        std::meta::identifier_of(m).starts_with("lv_"))
      v.push_back(m);
  return v;
}

// Every lv_* function, in declaration order. Sorting ~2000 names at compile time
// costs more than the linear searches it would save.
template <class D = void>
inline constexpr std::span<const std::meta::info> api_table = std::define_static_array(scan_api<D>());

// The table, worked out on the first call of a file. It is reached through substitute, not
// named: a function that names api_table<> instantiates it, and runs the scan, where the
// function is read (9.0 G instructions in every file, measured 2026-11-19). `asked` is the
// name or prefix the caller searches for, and is not read: a call with no argument that is not
// a constant is run by GCC where a non-template function is read, which would run the scan there.
consteval std::span<const std::meta::info> api(std::string_view /*asked*/) {
  return std::meta::extract<const std::span<const std::meta::info>&>(std::meta::substitute(^^api_table, {^^void}));
}

// Stops a constant evaluation with a message: the compile error a user sees for a
// wrong name, id, token or format. It is not an exception, so that the library builds
// with -fno-exceptions, as microcontroller projects do: GCC refuses the word `throw`
// there, even in code that only runs at compile time.
[[noreturn]] consteval void fail(std::string_view message) {
  __builtin_constexpr_diag(2, "", message);  // 2: an error
  // Reading through a null pointer is not a constant, which ends the evaluation here:
  // what follows a failure must not run.
  const int* nothing = nullptr;
  const int stop = *nothing;
  for (;;) static_cast<void>(stop);
}

// A number in decimal digits.
consteval std::string decimal(std::size_t n) {
  std::string digits;
  do digits.insert(digits.begin(), static_cast<char>('0' + n % 10));
  while ((n /= 10) > 0);
  return digits;
}

// The parts one after the other, as one text. An operator+ chain costs the compiler an overload
// resolution among the many operator+ of std::string at each +; a message is built here.
consteval std::string join(std::initializer_list<std::string_view> parts) {
  std::string out;
  for (const std::string_view part : parts) out += str(part);  // not append(part): see str
  return out;
}

// Whether `id` starts with `prefix`. In constant evaluation a call of starts_with costs about
// 0.37 M instructions; the size and the character after "lv_" reject most names for far less.
consteval bool has_prefix(std::string_view id, std::string_view prefix) {
  if (id.size() < prefix.size()) return false;
  if (prefix.size() > 3 && id[3] != prefix[3]) return false;
  return id.starts_with(prefix);
}

// The function called `name`, or a null reflection.
consteval std::meta::info find(std::string_view name) {
  for (auto f : api(name))
    if (std::meta::identifier_of(f) == name) return f;
  return {};
}

// find(name), run only when D is known. A variable template that calls find(name) directly runs
// the search where the header is read, in every file (0.9 G instructions for the three of
// lv/scaled_font.hpp, measured 2026-11-19).
template <class D>
consteval std::meta::info find_later(std::string_view name) {
  return find(name);
}

// The names that follow `prefix` in the API, comma separated.
consteval std::string names_after(std::string_view prefix) {
  std::string out;
  for (auto f : api(prefix)) {
    auto id = std::meta::identifier_of(f);
    if (!has_prefix(id, prefix)) continue;
    if (!out.empty()) out += ", ";
    out += id.substr(prefix.size());
  }
  return out;
}

// lv_<widget>_<verb>_<prop>, falling back to lv_obj_<verb>_<prop>.
consteval std::meta::info resolve(std::string_view widget, std::string_view verb,
                                  std::string_view prop) {
  const std::string own = "lv_" + str(widget) + "_" + str(verb) + "_";
  const std::string base = "lv_obj_" + str(verb) + "_";
  if (auto f = find(own + str(prop)); f != std::meta::info{}) return f;
  if (auto f = find(base + str(prop)); f != std::meta::info{}) return f;

  std::string msg = "lv: widget '" + str(widget) + "' has no property '" +
                    str(prop) + "' to " + str(verb) + ". Its own: " +
                    names_after(own) + ".";
  if (widget != "obj") msg += " Every '" + base + "*' property of obj works too.";
  fail(msg);
}

// lv_<widget>_create.
consteval std::meta::info resolve_create(std::string_view widget) {
  if (auto f = find("lv_" + str(widget) + "_create"); f != std::meta::info{}) return f;

  std::string valid;
  for (auto f : api(widget)) {
    auto id = std::meta::identifier_of(f);
    if (!id.ends_with("_create") || std::meta::parameters_of(f).size() != 1) continue;
    if (std::meta::return_type_of(f) != ^^lv_obj_t*) continue;
    if (!valid.empty()) valid += ", ";
    valid += id.substr(3, id.size() - 3 - 7);
  }
  fail("lv: there is no widget '" + str(widget) + "'. Valid: " + valid + ".");
}

}  // namespace lv::detail
