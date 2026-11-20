// lv/lv.hpp alone: the error names the header of the name (lv/detail/missing.hpp).
#include <lv/lv.hpp>

constexpr unsigned char png[] = {1, 2, 3};
constexpr auto picture = lv::image_file<png>;
