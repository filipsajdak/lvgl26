#pragma once

// A page's title, in the font that design/font_text_title.cpp draws.

#include "../design/design.hpp"

#include <lv/look.hpp>
#include <lv/lv.hpp>

namespace coffee {

constexpr auto title(const char* text) {
  return lv::label(
    {.text = text, .style = lv::look<"title"> | lv::style({.text_font = lv::font_of<"text.title">})}
  );
}

}  // namespace coffee
