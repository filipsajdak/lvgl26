// A typo of the state 'alt', which sorts before 'any': it is a state and gets its hint.
#include <lv/look.hpp>

#include "legacy_tokens.hpp"

constexpr char tokens_json[] = {
#embed "look_bad.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json, legacy_tokens>;
template <>
inline constexpr lv::design_of<decltype(tok)> lv::design<lv::override>{.tokens = tok};

constexpr auto ui = lv::box({.style = lv::look<"layeralt">});

int main() {}
