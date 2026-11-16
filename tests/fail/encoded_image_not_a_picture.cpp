// A licence text is neither a PNG nor a JPEG.
#include <lv/lv.hpp>

constexpr unsigned char file[] = {
#embed "fonts/OFL.txt"
};
constexpr auto picture = lv::encoded_image<file>;

int main() {}
