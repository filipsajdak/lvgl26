// The file that draws a font names a token that the token file lacks.
#include <lv/look.hpp>

constexpr char tokens_json[] = {
#embed "look_tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;
template <>
inline constexpr lv::design_of<decltype(tok)> lv::design<lv::override>{.tokens = tok};

constexpr unsigned char barlow[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr auto type = lv::typeface({{"Barlow", 500, barlow}});

template <>
const lv::drawn_font lv::font_object<"text.title"> = lv::draw<"text.title">(type);

int main() {}
