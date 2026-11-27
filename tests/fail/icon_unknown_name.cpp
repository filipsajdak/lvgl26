#include <lv/icons.hpp>
#include <lv/lv.hpp>

// A name that is no icon of Phosphor stops the build and lists the nearest names.
constexpr auto back = lv::label({.text = lv::glyph<"arow-left">});
