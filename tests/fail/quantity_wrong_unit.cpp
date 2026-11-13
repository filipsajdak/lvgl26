#include <lv/lv.hpp>
inline constexpr lv::unit<"km/h"> kmh;
inline constexpr lv::unit<"rpm"> rpm;
constinit lv::state speed{0 * kmh};
void f() { speed = 3000 * rpm; }
