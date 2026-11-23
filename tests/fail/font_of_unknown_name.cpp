// A typo in the name of a font: the message lists the names there and the nearest.
#include <lv/look.hpp>

constexpr char tokens_json[] = {
#embed "look_tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;
template <>
inline constexpr lv::design_of<decltype(tok)> lv::design<lv::override>{.tokens = tok};

constexpr auto font = lv::font_of<"text.lable">;

int main() {}
