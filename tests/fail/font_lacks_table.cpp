// A file with outlines but without the tables that say how to use them.
#include <lv/font.hpp>
#include <lv/lv.hpp>

using namespace lv::units;

constexpr unsigned char broken[] = {0, 1, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0,                          // two tables
                                    'g', 'l', 'y', 'f', 0, 0, 0, 0, 0, 0, 0, 44, 0, 0, 0, 0,  // glyf
                                    'l', 'o', 'c', 'a', 0, 0, 0, 0, 0, 0, 0, 44, 0, 0, 0, 0};  // loca
constexpr const lv_font_t* font = lv::font<broken>(18 * px);

int main() {}
