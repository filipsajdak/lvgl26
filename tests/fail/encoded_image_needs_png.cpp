// Compiled as if LVGL had no PNG decoder (see tests/CMakeLists.txt).
#include <lv/encoded_image.hpp>
#include <lv/lv.hpp>

constexpr unsigned char file[] = {
#embed "images/ring.png"
};
constexpr auto picture = lv::encoded_image<file>;

int main() {}
