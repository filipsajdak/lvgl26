#pragma once

// A page's title, in the font of the titles.

#include "../design/design.hpp"
#include "../design/fonts.hpp"

#include <lv/look.hpp>
#include <lv/lv.hpp>

namespace coffee {

constexpr auto title(const char* text) {
  return lv::label({.text = text, .style = lv::look<"title"> | title_text});
}

}  // namespace coffee
