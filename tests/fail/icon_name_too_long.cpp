#include <lv/icons.hpp>
#include <lv/lv.hpp>

// A name longer than any icon's by more than a typo has no near names, and the message lists none.
constexpr auto sentence = lv::label({.text = lv::glyph<"the-gear-icon-at-the-top-right-corner">});
