// The only file that draws the font of text.body: the other files name it with
// lv::font_of<"text.body"> and draw nothing.

#include "design.hpp"

#include <lv/font.hpp>
#include <lv/look.hpp>

//! [font]
namespace {

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr auto type = lv::typeface({{"Barlow", 500, barlow_ttf}});

}  // namespace

template <>
const lv::drawn_font lv::font_object<"text.body"> = lv::draw<"text.body">(type);
//! [font]
