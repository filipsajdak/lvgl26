// The four restrictions at once, for tests/settings_zero_cost.cpp.
#include <lv/config.hpp>

template <>
inline constexpr lv::config lv::cfg<lv::override>{
    .allow = {.allocation = false, .threads = false, .files = false, .floating_point = false}};
