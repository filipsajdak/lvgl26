#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

constexpr auto field = lv::style({.bg_color = tok.field.surface,
                                  .bg_opa = tok.opacity.opaque,
                                  .border_color = tok.field.border,
                                  .border_width = tok.field.spinbox.border_width,
                                  .pad_all = tok.field.spinbox.pad,
                                  .radius = tok.field.spinbox.radius,
                                  .text_color = tok.field.ink,
                                  .text_letter_space = tok.field.spinbox.letter_space});

// The digit under the cursor: no fill and no text, with an outline around it.
constexpr auto cursor = lv::style({.bg_opa = tok.opacity.none,
                                   .border_color = tok.field.accent,
                                   .border_width = tok.field.spinbox.cursor_border,
                                   .pad_ver = tok.field.spinbox.cursor_pad,
                                   .radius = tok.field.spinbox.cursor_radius,
                                   .text_opa = tok.opacity.none});

constexpr auto ui = lv::group({.flex_flow = "column", .style = centered | lv::style({.pad_row = tok.gap.lg})})[
    lv::widget<"spinbox">({.dec_point_pos = 2, .digit_count = 5, .step = 1, .value = 12345,
                 .width = 180 * px, .style = field, .style_cursor = cursor})];

}  // namespace

void widgets::spinbox_styling(lv::obj screen) {
  lv::place<ui>(screen);
}
