// A PNG file that says it is 0 x 0 pixels.
#include <lv/lv.hpp>

constexpr unsigned char file[] = {
#embed "images/empty.png"
};
constexpr auto picture = lv::encoded_image<file>;

int main() {}
