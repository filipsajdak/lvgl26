// LVGL's JPEG decoder reads baseline files only.
#include <lv/encoded_image.hpp>
#include <lv/lv.hpp>

constexpr unsigned char file[] = {
#embed "images/progressive.jpg"
};
constexpr auto picture = lv::encoded_image<file>;

int main() {}
