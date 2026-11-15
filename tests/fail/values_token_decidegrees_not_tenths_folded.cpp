#include <lv/lv.hpp>
static_assert(lv::styled<lv::decidegrees>{lv::token<double>{22.55, lv::detail::from_design_tokens{}}}.raw() == 225);
