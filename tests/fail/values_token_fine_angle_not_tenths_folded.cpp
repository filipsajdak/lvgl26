#include <lv/lv.hpp>
static_assert(lv::styled<lv::fine_angle>{lv::token<double>{22.55, lv::detail::from_design_tokens{}}}.raw() == 225);
