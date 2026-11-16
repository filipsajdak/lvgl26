// A with a breve and a hook stands 1089 units high: 261 px at this size.
#include <lv/lv.hpp>

using namespace lv::units;

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};

constexpr const lv_font_t* font = lv::font<barlow_ttf>(240 * px, "\xE1\xBA\xB2");

int main() {}
