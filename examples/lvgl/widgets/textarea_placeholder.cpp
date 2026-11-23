#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// An empty field shows its hint; a field with a text hides it.
constexpr auto ui = lv::group(
  {.flex_flow = "column",
   .style     = lv::style({
         .flex_cross_place = "center",
         .flex_main_place  = "center",
         .flex_track_place = "center",
         .pad_row          = tok.gap.lg,
   })},
  lv::textarea({
    .one_line         = true,
    .placeholder_text = "Search...",
    .width            = 60 * pct,
  }),
  lv::textarea({
    .one_line         = true,
    .placeholder_text = "Search\xE2\x80\xA6",
    .text             = "Hello world!",
    .width            = 60 * pct,
  })
);

}  // namespace

void widgets::textarea_placeholder(lv::obj screen) { lv::place<ui>(screen); }
