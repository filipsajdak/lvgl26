// A font file is not a picture.
#include <lv/image.hpp>
#include <lv/lv.hpp>

constexpr unsigned char file[] = {
#embed "fonts/OFL.txt"
};
constexpr auto picture = lv::image_file<file>;

int main() {}
