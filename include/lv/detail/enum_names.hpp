#pragma once

// The enumerators of an enum by name, found by reflection. Used to read an
// enumerator from a document (lv/runtime.hpp) and to write one (lv/dump.hpp).

#include <cstdint>
#include <meta>
#include <span>
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

}  // namespace lv::detail
