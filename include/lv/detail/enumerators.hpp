#pragma once

// The enumerators of an enum, by position: how many there are, and where a value stands among them in
// the order the enum declares them. lv::display::follow indexes its themes by it, and lv::shown judges
// the direction of a change by it.

#include <cstddef>
#include <meta>

namespace lv::detail {

template <class E>
inline constexpr std::size_t enumerator_count = std::meta::enumerators_of(^^E).size();

template <class E>
std::size_t enumerator_index(E value) {
  std::size_t at = 0;
  template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E))) {
    if (value == [:e:]) return at;
    ++at;
  }
  return 0;
}

}  // namespace lv::detail
