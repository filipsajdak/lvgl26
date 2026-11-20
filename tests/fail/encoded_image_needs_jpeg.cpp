// Compiled as if LVGL had no JPEG decoder (see tests/CMakeLists.txt).
#include <lv/encoded_image.hpp>
#include <lv/lv.hpp>

constexpr unsigned char file[] = {
#embed "images/gradient.jpg"
};
constexpr auto picture = lv::encoded_image<file>;

int main() {}
