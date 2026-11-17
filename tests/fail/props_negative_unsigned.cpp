#include <lv/lv.hpp>
// lv_spinner_set_arc_sweep takes the angle unsigned.
constexpr auto ui = lv::spinner({.arc_sweep = -1 * lv::deg});
