#pragma once

// Editor stub of lv/detail/fixed_string.hpp: a string as a template argument.

#include <cstddef>
#include <string_view>

namespace lv::detail {

/// A string literal as a template argument: lv::view<"arc">, get<"speedo">().
template <std::size_t N>
struct fixed {
  char s[N]{};

  consteval fixed(const char (&a)[N]) {
    for (std::size_t i = 0; i < N; ++i) s[i] = a[i];
  }

  // The first N - 1 characters of `text`: how the stubs name a widget they looked up.
  constexpr fixed(const char* text, std::size_t) {
    for (std::size_t i = 0; i + 1 < N; ++i) s[i] = text[i];
  }

  constexpr std::string_view sv() const { return {s, N - 1}; }

  friend constexpr bool operator==(const fixed&, const fixed&) = default;
};

}  // namespace lv::detail
