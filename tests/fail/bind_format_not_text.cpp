#include <lv/lv.hpp>
struct kmh { int value; };
constinit lv::state<kmh> n{kmh{0}};
constexpr auto ui = lv::arc({.value = lv::bind(n, "%d")});
