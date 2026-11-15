#include <lv/lv.hpp>
static_assert(lv::styled<lv::angle>{lv::token<double>{2.5, lv::detail::from_design_tokens{}}}.raw() == 2);
