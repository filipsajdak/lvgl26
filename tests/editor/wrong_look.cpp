// A look that names no group of the token file is marked where it is written. Read by Clang
// with the editor stubs only (the test editor_wrong_look); GCC reports it with lvgl26's own text
// and lists the groups it could have meant.
#include <lv/lv.hpp>

constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

template <>
inline constexpr lv::design_of<decltype(tok)> lv::design<lv::override>{.tokens = tok};

constexpr auto right = lv::box({.style = lv::look<"tile-print">});
constexpr auto several = lv::box({.style = lv::look<"tile tile-print">});
constexpr auto spelled_either_way = lv::box({.style = lv::look<"tile-ink-level"> | lv::look<"tile_ink_level">});

// expected-error@*:* {{this look names no group of the token file}}
constexpr auto typo = lv::box({.style = lv::look<"tile.pritn">});

// expected-error@*:* {{this look names no group of the token file}}
constexpr auto in_a_list = lv::box({.style = lv::look<"tile tile-scann">});
