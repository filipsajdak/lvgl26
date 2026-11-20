// A string token is no standard value for a style property: the message points to $extensions.
#include <lv/look.hpp>

#include "legacy_tokens.hpp"

constexpr char tokens_json[] = {
#embed "look_bad.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json, legacy_tokens>;
template <>
inline constexpr lv::design_of<decltype(tok)> lv::design<lv::override>{.tokens = tok};

constexpr auto ui = lv::box({.style = lv::look<"stringtoken">});

int main() {}
