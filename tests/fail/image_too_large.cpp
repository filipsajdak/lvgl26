// 200 x 100 pixels is more than the 128 x 128 the compiler decodes by default.
#include <lv/lv.hpp>

constexpr unsigned char file[] = {
#embed "images/wide.png"
};
constexpr auto picture = lv::image_file<file>;

int main() {}
