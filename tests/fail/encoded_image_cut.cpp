// A JPEG file that ends before it says its size.
#include <lv/encoded_image.hpp>
#include <lv/lv.hpp>

constexpr unsigned char file[] = {
#embed "images/cut.jpg"
};
constexpr auto picture = lv::encoded_image<file>;

int main() {}
