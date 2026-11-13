#include <lv/lv.hpp>
#include <mp-units/systems/si.h>
using namespace mp_units::si::unit_symbols;
constinit lv::state speed{0 * km / h};
void f() { speed = 33 * m / s; }  // 118.8 km/h does not fit an int
