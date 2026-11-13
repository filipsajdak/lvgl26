#include <lv/lv.hpp>
#include <mp-units/systems/si.h>
using namespace mp_units::si::unit_symbols;
constinit lv::state n{0 * km / h};
constexpr auto ui = lv::arc({.value = lv::bind(n, "{}")});
