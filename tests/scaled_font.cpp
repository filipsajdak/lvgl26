// lv::scaled_font: a font that the device draws from a TrueType file, at a size that can change.
//
//   scaled_font             runs the checks
//   scaled_font zero        resizes to 0 px: must end with a contract violation
//   scaled_font negative    resizes to -5 px: the same

#include <lv/lv.hpp>
#include <lv/scaled_font.hpp>

#include "check.hpp"

#include <cstdio>
#include <cstring>

namespace {

using namespace lv::units;

constexpr unsigned char barlow_ttf[] = {
#embed "fonts/Barlow-Medium.ttf"
};
constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

constinit lv::scaled_font title{barlow_ttf, tok.text.title};  // 20 px
constinit lv::scaled_font odd{barlow_ttf, lv::raw(31 * px)};

constexpr auto ui = lv::column({.size = {320 * px, 240 * px}})[
    lv::label({.id = "title", .text = "Wake up", .style = {.text_color = tok.color.text, .text_font = lv::bind(title)}}),
    lv::label({.id = "plain", .text = "Wake up"})];

std::size_t used_bytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.total_size - m.free_size;
}

void follows_its_font() {
  check(title.size() == 20 * px && odd.size() == 31 * px);
  auto root = lv::mount<ui>(lv::screen_active());
  check(root.has_value());
  if (!root) return;
  lv_obj_t* label = root->get<"title">().raw();
  lv_obj_t* plain = root->get<"plain">().raw();
  lv_obj_update_layout(lv_screen_active());

  // The widget holds the font of the object; the rest of its style is a constant as before.
  check(lv_obj_get_style_text_font(label, LV_PART_MAIN) == title.raw());
  check(lv_obj_get_style_text_font(plain, LV_PART_MAIN) != title.raw());
  check(lv_obj_get_style_text_color(label, LV_PART_MAIN).red == tok.color.text->red);
  const auto height20 = lv_obj_get_height(label), width20 = lv_obj_get_width(label);
  const auto plain_height = lv_obj_get_height(plain);

  // A new size reaches the widget without touching it.
  title.resize(tok.text.body);  // 14 px
  lv_obj_update_layout(lv_screen_active());
  check(title.size() == 14 * px);
  check(lv_obj_get_height(label) < height20 && lv_obj_get_width(label) < width20);
  check(lv_obj_get_height(plain) == plain_height);

  title.resize(lv::raw(40 * px));
  lv_obj_update_layout(lv_screen_active());
  check(lv_obj_get_height(label) > height20 && lv_obj_get_width(label) > width20);
  check(lv_obj_get_style_text_font(label, LV_PART_MAIN) == title.raw());
}

// At run time, with a view: the same binding.
void binds_through_a_view() {
  auto label = lv::screen_active().add<"label">().set({.text = "31"});
  // Which font is only known when the program runs, so the binding is made then.
  volatile bool use_title = false;
  lv::scaled_font& chosen = use_title ? title : odd;
  label.set({.style = lv::style({.text_font = lv::bind(chosen)})});
  check(lv_obj_get_style_text_font(label.raw(), LV_PART_MAIN) == odd.raw());
  lv_obj_delete(label.raw());
}

void is_made_on_first_use_and_given_back() {
  const auto before = used_bytes();
  {
    lv::scaled_font local{barlow_ttf, lv::raw(18 * px)};
    local.resize(lv::raw(22 * px));  // not used yet: nothing to change, nothing made
    check(used_bytes() == before && local.size() == 22 * px);
    check(local.raw() != nullptr && used_bytes() > before);
    check(local.raw()->line_height > 18);  // made at 22 px, not at 18
  }
  check(used_bytes() == before);
}

// A font that was never used has made nothing, and gives nothing back.
void gives_back_nothing_when_never_made() {
  const auto before = used_bytes();
  { lv::scaled_font unused{barlow_ttf, lv::raw(18 * px)}; }
  check(used_bytes() == before);
}

// A size the compiler cannot see, so that the contract is checked when the program runs.
volatile int no_size = 0, less_than_none = -5;

}  // namespace

int main(int argc, char** argv) {
  if (argc == 2) {
    odd.resize(lv::raw((std::strcmp(argv[1], "zero") == 0 ? no_size : less_than_none) * px));
    std::printf("no contract violation\n");
    return 0;
  }
  lv_init();
  lv_test_display_create(320, 240);
  is_made_on_first_use_and_given_back();
  gives_back_nothing_when_never_made();
  follows_its_font();
  binds_through_a_view();
  title.reset();
  odd.reset();
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
