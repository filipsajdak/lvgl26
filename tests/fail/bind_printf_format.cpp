#include <lv/lv.hpp>
struct kmh { int value; };
constinit lv::state<kmh> n{kmh{0}};
constexpr auto ui = lv::label({.text = lv::bind(n, "%d km/h")});
