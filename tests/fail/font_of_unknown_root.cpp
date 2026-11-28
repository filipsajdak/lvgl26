// A typo in the first name of a font: the message says it is not in the token file.
#include <lv/look.hpp>

constexpr char tokens_json[] = {
#embed "look_tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;
template <>
inline constexpr lv::design_of<decltype(tok)> lv::design<lv::override>{.tokens = tok};

constexpr auto font = lv::font_of<"txt.label">;

int main() {}
