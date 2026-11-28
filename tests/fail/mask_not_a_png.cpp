#include <lv/image.hpp>
#include <lv/lv.hpp>

// A mask is decoded by the compiler, from a PNG file.
constexpr unsigned char file[] = {1, 2, 3, 4};
constexpr auto steam = lv::mask_file<file>;
