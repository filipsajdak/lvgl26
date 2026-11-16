// A row with filter 7; the format has 0 to 4.
#include <lv/lv.hpp>

constexpr unsigned char file[] = {
#embed "images/bad_filter.png"
};
constexpr auto picture = lv::image_file<file>;

int main() {}
