// Draws text with fonts made by font_from_ttf.hpp. Build it by hand with the flags of a test,
// --embed-dir=site/media/fonts and -fconstexpr-ops-limit=400000000.
#include "font_from_ttf.hpp"
#include <lv/lv.hpp>
#include <lv/png.hpp>
constexpr unsigned char barlow[] = {
#embed "Barlow-Medium.ttf"
};
constexpr unsigned char condensed[] = {
#embed "BarlowCondensed-SemiBold.ttf"
};
constexpr const lv_font_t* b14 = spike::make_font(barlow, {.px = 14});
constexpr const lv_font_t* b20 = spike::make_font(barlow, {.px = 20});
constexpr const lv_font_t* b32 = spike::make_font(barlow, {.px = 32});
constexpr const lv_font_t* c72 = spike::make_font(condensed, {.px = 72, .first = '0', .last = '9'});
constinit lv::frame<480, 320> frame;
void text(lv_obj_t* parent, const lv_font_t* font, const char* s, int y) {
  lv_obj_t* l = lv_label_create(parent);
  if (font) lv_obj_set_style_text_font(l, font, LV_PART_MAIN);
  lv_label_set_text(l, s);
  lv_obj_set_pos(l, 12, y);
}
int main(int, char** argv) {
  lv::application app;
  lv::display display{app, frame};
  lv_obj_t* s = display.screen().raw();
  text(s, nullptr, "Montserrat 14 (LVGL built in): The quick brown fox, 0123456789", 8);
  text(s, b14, "Barlow 14 from the TTF: The quick brown fox jumps, 0123456789 @#&%", 34);
  text(s, b20, "Barlow 20: Hamburgefonstiv AVATAR To Ty {}[]()", 60);
  text(s, b32, "Barlow 32: Speed 120 km/h Reset", 96);
  text(s, c72, "0123456789", 150);
  return lv::write_png(display.screen(), argv[1]) ? 0 : 2;
}
