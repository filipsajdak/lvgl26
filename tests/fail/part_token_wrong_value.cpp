#include <lv/lv.hpp>
constexpr auto filled(lv::token_of<lv_color_t> auto fill) { return lv::box({.style = {.bg_color = fill}}); }
constexpr auto ui = filled(lv::raw(lv_color_t{}));  // a token, not lv::raw
