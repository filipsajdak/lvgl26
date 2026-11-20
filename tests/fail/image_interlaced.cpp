// An interlaced file.
#include <lv/image.hpp>
#include <lv/lv.hpp>

constexpr unsigned char file[] = {
#embed "images/interlaced.png"
};
constexpr auto picture = lv::image_file<file>;

int main() {}
