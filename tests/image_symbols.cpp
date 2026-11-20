// A large picture made by the compiler, in the 32-bit form. Nothing in its symbols may carry its pixels:
// tools/check_symbols.py reads the names of this object file (the test image_symbols).
//
// Apple's linker stops at a symbol name of a million characters, and a name made from the pixels is
// six characters for each byte: the 624 x 76 picture of a window application did not link.
// large.png is 256 x 128 RGBA, every alpha 255, with each row stored as the filter "up" over the one above:
// red is the column, green is twice the row, blue is 128. Its 131,072 bytes would give a name of 790,000
// characters.

#include <lv/image.hpp>
#include <lv/lv.hpp>

namespace {

constexpr unsigned char large_png[] = {
#embed "images/large.png"
};

constexpr auto picture = lv::detail::make_image(large_png, 32, 256 * 128);

}  // namespace

const lv_image_dsc_t* large_picture() { return picture; }
