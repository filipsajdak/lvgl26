#include <lv/font.hpp>
#include <lv/lv.hpp>

using namespace lv::units;

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};

// The extra characters are UTF-8 text: a byte of a cut character is refused.
constexpr const lv_font_t* font = lv::font<barlow_ttf>(10 * px, lv::latin1_plus<"a\xE2\x82">);

int main() {}
