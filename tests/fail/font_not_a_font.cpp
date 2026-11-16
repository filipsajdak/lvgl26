// The embedded file is not a font.
#include <lv/lv.hpp>

using namespace lv::units;

constexpr unsigned char text[] = {'n', 'o', 't', ' ', 'a', ' ', 'f', 'o', 'n', 't', '.', '.', '.'};
constexpr const lv_font_t* font = lv::font<text>(18 * px);

int main() {}
