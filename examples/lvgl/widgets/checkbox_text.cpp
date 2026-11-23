#include "widgets.hpp"

namespace {

using namespace widgets;

// A short text, a longer one, and one with two lines.
constexpr auto ui = lv::group(
  {.flex_flow = "column", .style = centered | lv::style({.pad_row = tok.gap.lg})},
  lv::checkbox({.text = "Enable"}), lv::checkbox({.text = "Send anonymous usage data"}),
  lv::checkbox({.text = "I agree to the terms\nand the privacy policy"})
);

}  // namespace

void widgets::checkbox_text(lv::obj screen) { lv::place<ui>(screen); }
