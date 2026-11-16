// The file says four rows and holds three.
#include <lv/lv.hpp>

constexpr unsigned char file[] = {
#embed "images/short.png"
};
constexpr auto picture = lv::image_file<file>;

int main() {}
