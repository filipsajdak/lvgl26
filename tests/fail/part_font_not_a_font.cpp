#include <lv/font.hpp>
#include <lv/lv.hpp>
constexpr auto titled(const char* text, const lv::font_value auto& font) { return lv::label({.style = {.text_font = font}, .text = text}); }
constexpr auto ui = titled("Title", 14);  // a font comes from lv::typeface, lv::raw(...) or lv::bind(...)
