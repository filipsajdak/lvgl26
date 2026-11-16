// The per mille sign is wider than the em: 275 px at this size.
#include <lv/lv.hpp>

using namespace lv::units;

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};

constexpr const lv_font_t* font = lv::font<barlow_ttf>(250 * px, "\xE2\x80\xB0");

int main() {}
