#include "widgets.hpp"

namespace {

using namespace widgets;

// The corners of the line. The widget keeps a pointer to them, not a copy.
constexpr lv_point_precise_t corners[] = {{5, 5}, {70, 70}, {120, 10}, {180, 60}, {240, 10}};

// A wide blue line with round ends, in the middle of the screen.
constexpr auto ui = lv::widget<"line">({.align = LV_ALIGN_CENTER,
                                        .points = {corners, 5},
                                        .style = {.line_color = tok.line.color,
                                                  .line_rounded = true,
                                                  .line_width = tok.line.width}});

}  // namespace

void widgets::line_points(lv::obj screen) {
  lv::place<ui>(screen);
}
