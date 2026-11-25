#pragma once

// The fonts of the machine, as styles to merge. Each is drawn in a file of its own,
// font_text_title.cpp and font_text_body.cpp; a file that includes this header draws nothing.

#include "design.hpp"

#include <lv/look.hpp>
#include <lv/lv.hpp>

namespace coffee {

//! [fonts]
inline constexpr auto title_text = lv::style({.text_font = lv::font_of<"text.title">});
inline constexpr auto body_text  = lv::style({.text_font = lv::font_of<"text.body">});
//! [fonts]

}  // namespace coffee
