// The token asks for weight 700 and the typeface has only 400.
#include <lv/lv.hpp>

using namespace lv::units;

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

constexpr auto type = lv::typeface({{"Montserrat", 400, barlow_ttf}});
constexpr auto title = type(tok.text.title);

int main() {}
