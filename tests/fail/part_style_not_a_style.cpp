#include <lv/lv.hpp>
constexpr auto styled_box(lv::style_value auto style) { return lv::box({.style = style}); }
constexpr auto ui = styled_box(5);  // lv::style({...})
