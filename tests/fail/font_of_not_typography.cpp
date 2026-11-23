// A font named by a token that is no typography: a colour.
#include <lv/look.hpp>

constexpr char tokens_json[] = {
#embed "look_tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;
template <>
inline constexpr lv::design_of<decltype(tok)> lv::design<lv::override>{.tokens = tok};

constexpr auto font = lv::font_of<"color.blue">;

int main() {}
