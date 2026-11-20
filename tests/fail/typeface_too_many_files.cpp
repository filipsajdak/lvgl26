#include <lv/font.hpp>
#include <lv/lv.hpp>

using namespace lv::units;

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};

constexpr auto type = lv::typeface({{"A", 100, barlow_ttf}, {"A", 200, barlow_ttf}, {"A", 300, barlow_ttf},
                                    {"A", 400, barlow_ttf}, {"A", 500, barlow_ttf}, {"A", 600, barlow_ttf},
                                    {"A", 700, barlow_ttf}, {"A", 800, barlow_ttf}, {"A", 900, barlow_ttf}});

int main() {}
