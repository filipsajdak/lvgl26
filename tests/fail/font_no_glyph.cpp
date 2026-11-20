// Barlow has no Cyrillic letters.
#include <lv/font.hpp>
#include <lv/lv.hpp>

using namespace lv::units;

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};

constexpr const lv_font_t* font = lv::font<barlow_ttf>(18 * px, "\xD0\x96");

int main() {}
