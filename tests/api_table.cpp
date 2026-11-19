// The API table asks the name of a function only when it is declared on a line of a file
// (detail::declared_in_a_file). It must hold the same functions as a scan that asks every name,
// on every platform: if a compiler put an lv_ function at line 0 or 1, this file stops compiling.
#include <lv/detail/api.hpp>

#include <cstddef>
#include <meta>

namespace {

consteval std::size_t every_lv_function() {
  std::size_t n = 0;
  for (auto m : std::meta::members_of(^^::, std::meta::access_context::unchecked()))
    if (std::meta::is_function(m) && std::meta::has_identifier(m) && std::meta::identifier_of(m).starts_with("lv_"))
      ++n;
  return n;
}

consteval bool same_functions() {
  std::size_t i = 0;
  for (auto m : std::meta::members_of(^^::, std::meta::access_context::unchecked())) {
    if (!std::meta::is_function(m) || !std::meta::has_identifier(m) || !std::meta::identifier_of(m).starts_with("lv_"))
      continue;
    if (i >= lv::detail::api_table<>.size() || lv::detail::api_table<>[i] != m) return false;
    ++i;
  }
  return i == lv::detail::api_table<>.size();
}

static_assert(every_lv_function() > 1000);
static_assert(same_functions(), "the API table lost an lv_ function, or holds one twice");

}  // namespace

int main() {}
