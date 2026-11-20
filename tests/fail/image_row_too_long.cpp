// 16384 x 1 pixels is within the pixel limit, but at 32 bits a row is 65536 bytes:
// one more than LVGL's 16 bits for it hold.
#include <lv/image.hpp>
#include <lv/lv.hpp>

constexpr unsigned char file[] = {
#embed "images/row_65536.png"
};
constexpr auto picture = lv::detail::make_image(file, 32, 128 * 128);

int main() {}
