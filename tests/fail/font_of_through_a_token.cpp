// A font path that goes on after a token: a token holds no group.
#include <lv/look.hpp>

constexpr char tokens_json[] = {
#embed "look_tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;
template <>
inline constexpr lv::design_of<decltype(tok)> lv::design<lv::override>{.tokens = tok};

constexpr auto font = lv::font_of<"text.label.big">;

int main() {}
