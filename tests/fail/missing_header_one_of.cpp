// lv/lv.hpp alone: the error names the header of the name (lv/detail/missing.hpp).
#include <lv/lv.hpp>

constexpr auto a = lv::label();
constexpr auto b = lv::button();
lv::one_of<a, b> slot;
