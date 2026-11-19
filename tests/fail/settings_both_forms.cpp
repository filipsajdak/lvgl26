// A project with a settings file that also specializes lv::cfg: one or the other. The file's
// header, which CMake includes first, defines the specialization, so this one is a second.
#include <lv/lv.hpp>

template <>
inline constexpr lv::config lv::cfg<lv::override>{.rules = {.raw_style_values = true}};
