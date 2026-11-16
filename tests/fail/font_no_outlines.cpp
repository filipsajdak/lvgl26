// An OpenType file with CFF outlines (.otf) has no glyf table.
#include <lv/lv.hpp>

using namespace lv::units;

constexpr unsigned char otf[] = {'O', 'T', 'T', 'O', 0, 0, 0, 0, 0, 0, 0, 0};
constexpr const lv_font_t* font = lv::font<otf>(18 * px);

int main() {}
