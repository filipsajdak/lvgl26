#include <lv/font.hpp>
#include <lv/lv.hpp>

using namespace lv::units;

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};

// An extra character that the file lacks is the message of any character it lacks.
constexpr const lv_font_t* font = lv::font<barlow_ttf>(18 * px, lv::ascii_plus<"\xD0\x96">);

int main() {}
