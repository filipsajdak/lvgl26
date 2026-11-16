// Sixteen bits for each channel.
#include <lv/lv.hpp>

constexpr unsigned char file[] = {
#embed "images/deep.png"
};
constexpr auto picture = lv::image_file<file>;

int main() {}
