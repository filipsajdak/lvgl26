#include <lv/lv.hpp>
inline constexpr lv::unit<"km/h"> kmh;
constinit lv::state n{0 * kmh};
constexpr auto ui = lv::arc({.value = lv::bind(n, "{}")});
