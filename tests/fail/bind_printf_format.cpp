#include <lv/lv.hpp>
inline constexpr lv::unit<"km/h"> kmh;
constinit lv::state n{0 * kmh};
constexpr auto ui = lv::label({.text = lv::bind(n, "%d km/h")});
