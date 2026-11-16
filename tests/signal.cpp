// lv::signal: the project's design system as a ready theme, in its two forms.

#include <lv/lv.hpp>
#include <lv/signal.hpp>

#include "check.hpp"

namespace {

using namespace lv::units;

static_assert(!lv::signal::dark.empty() && !lv::signal::day.empty());

// The tokens are Signal's: design/signal/tokens.design.json.
static_assert(lv::signal::dark_tokens.color.active->red == 0xd4 && lv::signal::dark_tokens.color.active->blue == 0x3a);
static_assert(lv::signal::day_tokens.color.ground->red == 0xff);
static_assert(lv::signal::dark_tokens.space._4 == 16 * px && lv::signal::dark_tokens.stroke.track == 8 * px);
static_assert(lv::signal::dark_tokens.size.target == 48 * px && lv::signal::day_tokens.radius._0 == 0 * px);

constinit lv::frame<64, 48> frame;

bool same(lv_color_t a, lv_color_t b) { return a.red == b.red && a.green == b.green && a.blue == b.blue; }

// `tok` is the token set that `theme` was made from.
void follows(lv::theme_styles theme, const auto& tok) {
  lv::application app;
  lv::display display{app, frame, theme};
  lv_obj_t* screen = display.screen().raw();
  check(same(lv_obj_get_style_bg_color(screen, LV_PART_MAIN), *tok.color.ground));

  lv_obj_t* box = lv_obj_create(screen);
  check(lv_obj_get_style_bg_opa(box, LV_PART_MAIN) == LV_OPA_TRANSP);
  check(same(lv_obj_get_style_text_color(box, LV_PART_MAIN), *tok.color.reading));
  check(lv_obj_get_style_pad_top(box, LV_PART_MAIN) == 16 && lv_obj_get_style_pad_row(box, LV_PART_MAIN) == 12);

  // Square, flat, large enough for a finger.
  lv_obj_t* button = lv_button_create(screen);
  check(same(lv_obj_get_style_bg_color(button, LV_PART_MAIN), *tok.color.active));
  check(same(lv_obj_get_style_text_color(button, LV_PART_MAIN), *tok.color.on_active));
  check(lv_obj_get_style_radius(button, LV_PART_MAIN) == 0 && lv_obj_get_style_shadow_width(button, LV_PART_MAIN) == 0);
  check(lv_obj_get_style_min_height(button, LV_PART_MAIN) == 48);
  check(lv_obj_get_style_pad_left(button, LV_PART_MAIN) == 24 && lv_obj_get_style_pad_top(button, LV_PART_MAIN) == 12);
  lv_obj_add_state(button, LV_STATE_PRESSED);
  check(same(lv_obj_get_style_bg_color(button, LV_PART_MAIN), *tok.color.surface_raised));
  check(lv_obj_get_style_transform_width(button, LV_PART_MAIN) == 0);
  check(lv_obj_get_style_color_filter_opa(button, LV_PART_MAIN) == LV_OPA_TRANSP);

  lv_obj_t* arc = lv_arc_create(screen);
  check(same(lv_obj_get_style_arc_color(arc, LV_PART_MAIN), *tok.color.line));
  check(same(lv_obj_get_style_arc_color(arc, LV_PART_INDICATOR), *tok.color.active));
  check(lv_obj_get_style_arc_width(arc, LV_PART_MAIN) == 8 && lv_obj_get_style_arc_width(arc, LV_PART_INDICATOR) == 8);
  check(!lv_obj_get_style_arc_rounded(arc, LV_PART_MAIN) && !lv_obj_get_style_arc_rounded(arc, LV_PART_INDICATOR));
  check(lv_obj_get_style_bg_opa(arc, LV_PART_KNOB) == LV_OPA_TRANSP);

  lv_obj_t* bar = lv_bar_create(screen);
  check(same(lv_obj_get_style_bg_color(bar, LV_PART_MAIN), *tok.color.line));
  check(same(lv_obj_get_style_bg_color(bar, LV_PART_INDICATOR), *tok.color.active));
  check(lv_obj_get_style_radius(bar, LV_PART_MAIN) == 0 && lv_obj_get_style_radius(bar, LV_PART_INDICATOR) == 0);
}

}  // namespace

int main() {
  follows(lv::signal::dark, lv::signal::dark_tokens);
  follows(lv::signal::day, lv::signal::day_tokens);
  return failures;
}
