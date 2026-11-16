// 70000 pixels wide, in a project that raised the pixel limit.
#include <lv/lv.hpp>

template <>
inline constexpr lv::config lv::cfg<lv::override>{.compile_time_image_pixels = 100000};

constexpr unsigned char file[] = {
#embed "images/wide_70000.png"
};
constexpr auto picture = lv::image_file<file>;

int main() {}
