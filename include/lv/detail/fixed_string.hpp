#pragma once

#include <cstddef>
#include <string_view>

namespace lv::detail {

// A string usable as a template argument, e.g. widget<"label"> or set<"text">.
template <std::size_t N>
struct fixed {
  char s[N]{};

  consteval fixed(const char (&a)[N]) {
    for (std::size_t i = 0; i < N; ++i) s[i] = a[i];
  }

  constexpr std::string_view sv() const { return {s, N - 1}; }

  friend constexpr bool operator==(const fixed&, const fixed&) = default;
};

}  // namespace lv::detail
