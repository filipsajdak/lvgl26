// The registration comes after a look has read the design: like lv::cfg, it must come first.
#include <lv/look.hpp>
#include <lv/lv.hpp>

constexpr char tokens_json[] = {
#embed "look_tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

constexpr auto ui = lv::box({.style = lv::look<"tile">});

template <>
inline constexpr lv::design_of<decltype(tok)> lv::design<lv::override>{.tokens = tok};

int main() {}
