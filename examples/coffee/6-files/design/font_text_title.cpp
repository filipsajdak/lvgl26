// The only file that draws the font of text.title: the other files name it with
// lv::font_of<"text.title"> and draw nothing.

#include "design.hpp"

#include <lv/font.hpp>
#include <lv/look.hpp>

namespace {

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr auto type = lv::typeface({{"Barlow", 500, barlow_ttf}});

}  // namespace

//! [font]
template <>
const lv::drawn_font lv::font_object<"text.title"> = lv::draw<"text.title">(type);
//! [font]
