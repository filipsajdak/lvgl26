#pragma once

// Inter, the font of the original (OFL-1.1, see NOTICE.md and fonts/OFL.txt). The compiler draws
// a font where it is asked for, so a page makes only the sizes it shows:
//
//   constexpr auto body = printer::type(tok.text.body);

#include <lv/font.hpp>
#include <lv/lv.hpp>

namespace printer {

constexpr unsigned char inter_regular[] = {
#embed "fonts/Inter-24pt-Regular.ttf"
};
constexpr unsigned char inter_medium[] = {
#embed "fonts/Inter-24pt-Medium.ttf"
};

constexpr auto type = lv::typeface({
    {"Inter 24pt", 400, inter_regular},
    {"Inter 24pt", 500, inter_medium},
});

}  // namespace printer
