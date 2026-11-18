// $extends is in the standard and not read yet: a look of such a group is refused.
#include <lv/lv.hpp>

constexpr char tokens_json[] = {
#embed "look_bad.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;
template <>
inline constexpr lv::design_of<decltype(tok)> lv::design<lv::override>{.tokens = tok};

constexpr auto ui = lv::box({.style = lv::look<"extends">});

int main() {}
