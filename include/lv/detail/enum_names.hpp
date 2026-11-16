#pragma once

// The enumerators of an enum by name, found by reflection. Used to read an
// enumerator from a document (lv/runtime.hpp) and to write one (lv/dump.hpp).

#include <lv/detail/api.hpp>

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

// How many characters all enumerators of E share, up to and with an underscore:
// 9 for lv_align_t, whose enumerators all start with "LV_ALIGN_".
consteval std::size_t shared_prefix(std::span<const enum_name> names) {
  const std::string_view first = names[0].name;
  std::size_t same = first.size();
  for (const enum_name& other : names) {
    const std::string_view text = other.name;
    std::size_t i = 0;
    while (i < same && i < text.size() && text[i] == first[i]) ++i;
    same = i;
  }
  while (same > 0 && first[same - 1] != '_') --same;
  return same;
}

// `text` in lower case.
consteval std::string lowercase(std::string_view text) {
  std::string out{text};
  for (char& c : out)
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  return out;
}

// The name of an enumerator as user code writes it: "top_left" for LV_ALIGN_TOP_LEFT.
template <class E>
consteval std::string short_enum_name(const enum_name& e) {
  return lowercase(std::string_view{e.name}.substr(shared_prefix(enum_names<E>)));
}

// "align has no value 'centre'. Its values: default, top_left, ..."
template <class E>
consteval std::string no_such_enumerator(std::string_view text) {
  const std::size_t prefix = shared_prefix(enum_names<E>);
  const std::string_view full = enum_names<E>[0].name;
  const std::size_t start = full.starts_with("LV_") ? 3 : 0;
  std::string out = "lv: " + lowercase(full.substr(start, prefix > start ? prefix - start - 1 : 0)) +
                    " has no value '" + std::string{text} + "'. Its values: ";
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
