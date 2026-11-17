#pragma once

// The enumerators of an enum by name, found by reflection. Used to read an
// enumerator from a document (lv/runtime.hpp) and to write one (lv/dump.hpp).

#include <lv/detail/api.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <meta>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace lv::detail {

struct enum_name {
  const char* name;  // the enumerator, in full
  std::int64_t value;
};

// The enumerators of E by name.
template <class E>
consteval std::vector<enum_name> scan_enum() {
  std::vector<enum_name> out;
  for (auto e : std::meta::enumerators_of(std::meta::dealias(^^E)))
    out.push_back({std::define_static_string(std::meta::identifier_of(e)),
                   static_cast<std::int64_t>(std::meta::extract<E>(e))});
  return out;
}
template <class E>
inline constexpr std::span<const enum_name> enum_names = std::define_static_array(scan_enum<E>());

// What LVGL's public enumerators start with. LVGL marks an internal one with a
// leading underscore instead, as in _LV_IMAGE_ALIGN_AUTO_TRANSFORM.
inline constexpr std::string_view lvgl_mark = "LV_";

// What the enumerators of an enum share: the family they belong to ("LV_", or nothing
// for an enum without LVGL's names) and how many characters its members have in common.
struct enum_prefix {
  std::string_view family;
  std::size_t size;
};

consteval std::string_view family_of(std::span<const enum_name> names) {
  for (const enum_name& e : names)
    if (std::string_view{e.name}.starts_with(lvgl_mark)) return lvgl_mark;
  return {};
}

// The first enumerator of the family: the one the others are compared with.
consteval std::string_view first_of_family(std::span<const enum_name> names, std::string_view family) {
  for (const enum_name& e : names)
    if (std::string_view{e.name}.starts_with(family)) return e.name;
  return {};
}

consteval std::size_t common_length(std::string_view a, std::string_view b) {
  std::size_t i = 0;
  while (i < a.size() && i < b.size() && a[i] == b[i]) ++i;
  return i;
}

// The prefix of the family, up to and with an underscore: "LV_ALIGN_" for lv_align_t.
// An enumerator outside the family is not compared, so it cannot shorten the prefix.
consteval enum_prefix prefix_of(std::span<const enum_name> names) {
  const std::string_view family = family_of(names), first = first_of_family(names, family);
  std::size_t same = first.size();
  for (const enum_name& other : names)
    if (std::string_view{other.name}.starts_with(family))
      same = std::min(same, common_length(first, other.name));
  while (same > 0 && first[same - 1] != '_') --same;
  return {family, same};
}

// How many characters the family of the enumerators shares: 9 for lv_align_t, whose
// enumerators all start with "LV_ALIGN_".
consteval std::size_t shared_prefix(std::span<const enum_name> names) {
  return prefix_of(names).size;
}

// The prefix of E, found once.
template <class E>
inline constexpr enum_prefix prefix_of_enum = prefix_of(enum_names<E>);

// `text` in lower case. The text is copied one character at a time: under a sanitizer
// GCC 16.2 does not take std::string's own copy from a pointer as a constant expression.
consteval std::string lowercase(std::string_view text) {
  std::string out;
  for (char c : text) out.push_back(c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c);
  return out;
}

// The name of an enumerator as user code writes it: "top_left" for LV_ALIGN_TOP_LEFT.
// One outside the family keeps its whole name: "_lv_image_align_auto_transform".
consteval std::string short_name(std::string_view full, enum_prefix prefix) {
  return lowercase(full.starts_with(prefix.family) ? full.substr(prefix.size) : full);
}

template <class E>
consteval std::string short_enum_name(const enum_name& e) {
  return short_name(e.name, prefix_of_enum<E>);
}

// "align has no value 'centre'. Its values: default, top_left, ..."
template <class E>
consteval std::string no_such_enumerator(std::string_view text) {
  const enum_prefix prefix = prefix_of_enum<E>;
  const std::string_view full = first_of_family(enum_names<E>, prefix.family);
  const std::size_t start = full.starts_with(lvgl_mark) ? lvgl_mark.size() : 0;
  std::string out = "lv: " +
                    lowercase(full.substr(start, prefix.size > start ? prefix.size - start - 1 : 0)) +
                    " has no value '";
  for (char c : text) out.push_back(c);
  out += "'. Its values: ";
  for (std::size_t i = 0; i < enum_names<E>.size(); ++i)
    out += (i ? ", " : "") + short_enum_name<E>(enum_names<E>[i]);
  return out + ".";
}

// The enumerator of E that user code names `text`. Anything else is a compile error
// that lists the names.
template <class E>
consteval E enumerator_named(std::string_view text) {
  for (const enum_name& e : enum_names<E>)
    if (short_enum_name<E>(e) == text) return static_cast<E>(e.value);
  fail(no_such_enumerator<E>(text));
}

}  // namespace lv::detail
