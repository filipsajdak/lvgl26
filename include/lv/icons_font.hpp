#pragma once

// The font file of the icons of lv/icons.hpp: Phosphor Bold (MIT, include/lv/icons/LICENSE-Phosphor.txt).
// It is a header of its own: embedding 490 KB costs 1 G instructions in each file that reads it, and only
// the file that draws the font needs it.
//
//   lv::typeface({{"Phosphor", 700, lv::icons::phosphor_bold_ttf, lv::glyphs<"arrow-left", "gear">}})

namespace lv::icons {

// It does not reach the program: only the glyphs of the fonts you make do. Not `inline`, like the token
// files: under a sanitizer GCC 16.2 does not take the address of an inline variable for a constant.
constexpr unsigned char phosphor_bold_ttf[] = {
#embed "icons/Phosphor-Bold.ttf"
};

}  // namespace lv::icons
