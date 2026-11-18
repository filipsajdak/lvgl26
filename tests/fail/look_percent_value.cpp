// A percentage as a dimension value: the standard has none, the message points to $extensions.
#include <lv/lv.hpp>

constexpr char tokens_json[] = {
#embed "look_bad.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;
template <>
inline constexpr lv::design_of<decltype(tok)> lv::design<lv::override>{.tokens = tok};

constexpr auto ui = lv::box({.style = lv::look<"percentvalue">});

int main() {}
