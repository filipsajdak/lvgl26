#include <lv/lv.hpp>

// The mistake that the operation limit of include_only catches: a table of the global
// namespace that a function with no template parameter reaches with constant arguments. GCC
// works the table out where the function is read, though nothing calls the function. This was
// detail/api.hpp's api until it took the name it searches for: the range of the loop is
// worked out where the function is read.
template <class D>
consteval std::vector<std::meta::info> functions_named_lv() {
  std::vector<std::meta::info> out;
  for (auto m : std::meta::members_of(^^::, std::meta::access_context::unchecked()))
    if (std::meta::has_identifier(m) && std::meta::identifier_of(m).starts_with("lv_")) out.push_back(m);
  return out;
}
template <class D = void>
inline constexpr std::span<const std::meta::info> lv_names = std::define_static_array(functions_named_lv<D>());

consteval std::span<const std::meta::info> names() {
  return std::meta::extract<const std::span<const std::meta::info>&>(std::meta::substitute(^^lv_names, {^^void}));
}
consteval bool named(std::string_view name) {
  for (auto f : names())
    if (std::meta::identifier_of(f) == name) return true;
  return false;
}
