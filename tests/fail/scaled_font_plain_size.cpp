// A size without a token and without lv::raw.
#include <lv/lv.hpp>

using namespace lv::units;

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};

constinit lv::scaled_font title{barlow_ttf, 20 * px};

int main() {}
