// lv/image.hpp works without the rest of the library.
#include <lv/image.hpp>

constexpr unsigned char flag_png[] = {
#embed "images/flag.png"
};

int main() { return lv::image_file<flag_png>->header.w == 24 ? 0 : 1; }
