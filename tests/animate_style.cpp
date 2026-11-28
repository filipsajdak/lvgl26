// lv::animate moves local style values: of the main part with the call of every animation, and of a part
// and a state with lv::in. What the animation set is read back as the local value of the style, which no
// theme or look has a say in.

#include <lv/animate.hpp>
#include <lv/lv.hpp>

#include "check.hpp"

#include <concepts>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace {

using namespace lv::units;

// The number that an object holds in its own style for a property, a part and a state: -1000000 for none.
std::int32_t local(lv_obj_t* o, lv_style_prop_t prop, lv_style_selector_t selector) {
  lv_style_value_t v{};
  return lv_obj_get_local_style_prop(o, prop, &v, selector) == LV_STYLE_RES_FOUND ? v.num : -1000000;
}

// The knob of an arc while it is pressed.
constexpr lv_style_selector_t knob_pressed =
    static_cast<lv_style_selector_t>(LV_PART_KNOB) | static_cast<lv_style_selector_t>(LV_STATE_PRESSED);

struct outcome {
  bool finished = false;
  bool ended = false;
};
struct at_exit {
  bool* flag;
  ~at_exit() { *flag = true; }
};

// ---- the struct ------------------------------------------------------------------------------------------

// The plain call: the widget's setters first, and the style properties that a setter does not name. `.width` is
// the widget's, in a plain call; behind lv::in it is the style's length.
static_assert(std::same_as<decltype(lv::animatable<"arc">{}.width), lv::from_to<lv::horizontal>>);
static_assert(std::same_as<decltype(lv::animatable<"arc">{}.value), lv::from_to<lv::strict<std::int32_t>>>);
static_assert(std::same_as<decltype(lv::animatable<"arc">{}.opa), lv::from_to<lv::opa>>);
static_assert(std::same_as<decltype(lv::animatable<"label">{}.translate_y), lv::from_to<lv::length>>);
static_assert(std::same_as<decltype(lv::animatable_style<>{}.width), lv::from_to<lv::length>>);
static_assert(std::same_as<decltype(lv::animatable_style<>{}.arc_width), lv::from_to<lv::length>>);
template <class T>
concept has_value = requires(T t) { t.value; };
template <class T>
concept has_bg_color = requires(T t) { t.bg_color; };
template <class T>
concept has_text = requires(T t) { t.text; };
template <class T>
concept has_layers = requires(T t) { t.layers; };
static_assert(!has_value<lv::animatable_style<>>);       // a setter of a widget, not a style property
static_assert(!has_bg_color<lv::animatable<"arc">>); // a color is not a number
static_assert(!has_text<lv::animatable_style<>> && !has_layers<lv::animatable_style<>>);

// ---- the main part -----------------------------------------------------------------------------------------

lv::task<> fade(lv::view<"label"> label, lv::duration time, outcome* out) {
  at_exit cleanup{&out->ended};
  out->finished = co_await lv::animate(label, {.opa = {0 * pct, 100 * pct}}, time);
}

lv::task<> rise(lv::view<"label"> label, int from, int to, lv::duration time, outcome* out) {
  out->finished = co_await lv::animate(label, {.translate_y = {from * px, to * px}}, time);
}

void the_main_part_is_animated_by_the_plain_call(lv::obj screen) {
  auto label = screen.add<"label">();
  outcome out;
  check(lv::spawn(fade(label, 300 * ms, &out)));
  check(local(label.raw(), LV_STYLE_OPA, 0) == LV_OPA_TRANSP);  // the start value is set at once
  lv_test_wait(150);
  const auto middle = local(label.raw(), LV_STYLE_OPA, 0);
  check(middle > LV_OPA_TRANSP && middle < LV_OPA_COVER);
  lv_test_wait(200);
  check(local(label.raw(), LV_STYLE_OPA, 0) == LV_OPA_COVER && out.finished && out.ended);
  check(lv_anim_count_running() == 0);

  outcome up;
  check(lv::spawn(rise(label, 0, -24, 100 * ms, &up)));
  lv_test_wait(150);
  check(local(label.raw(), LV_STYLE_TRANSLATE_Y, 0) == -24 && up.finished);
  lv_obj_delete(label.raw());
}

// ---- a part and a state -------------------------------------------------------------------------------------

lv::task<> widen_indicator(lv::view<"arc"> arc, outcome* out) {
  out->finished = co_await lv::animate(lv::in<"indicator">(arc), {.arc_width = {2 * px, 12 * px}}, 100 * ms);
}
lv::task<> show_pressed_knob(lv::view<"arc"> arc, outcome* out) {
  out->finished = co_await lv::animate(lv::in<"knob", "pressed">(arc), {.opa = {20 * pct, 100 * pct}}, 100 * ms);
}
lv::task<> checked_main(lv::view<"arc"> arc, outcome* out) {
  out->finished = co_await lv::animate(lv::in<"checked">(arc), {.translate_x = {0 * px, 5 * px}}, 100 * ms);
}

void a_part_and_a_state_are_named_with_lv_in(lv::obj screen) {
  auto arc = screen.add<"arc">();
  outcome a, b, c;
  check(lv::spawn(widen_indicator(arc, &a)));
  check(lv::spawn(show_pressed_knob(arc, &b)));
  check(lv::spawn(checked_main(arc, &c)));
  check(lv_anim_count_running() == 3);  // three selectors: three animations side by side
  check(local(arc.raw(), LV_STYLE_ARC_WIDTH, LV_PART_INDICATOR) == 2);
  check(local(arc.raw(), LV_STYLE_OPA, knob_pressed) == lv::opa{20 * pct}.raw());
  lv_test_wait(150);
  check(a.finished && b.finished && c.finished);
  check(local(arc.raw(), LV_STYLE_ARC_WIDTH, LV_PART_INDICATOR) == 12);
  check(local(arc.raw(), LV_STYLE_OPA, knob_pressed) == LV_OPA_COVER);
  check(local(arc.raw(), LV_STYLE_TRANSLATE_X, LV_STATE_CHECKED) == 5);
  // Nothing was set in the main part or in another state.
  check(local(arc.raw(), LV_STYLE_ARC_WIDTH, 0) == -1000000);
  check(local(arc.raw(), LV_STYLE_OPA, LV_PART_KNOB) == -1000000);
  lv_obj_delete(arc.raw());
}

// ---- one property, one selector ----------------------------------------------------------------------------

lv::task<> slide(lv::view<"arc"> arc, int to, outcome* out) {
  out->finished = co_await lv::animate(arc, {.translate_x = {0 * px, to * px}}, 200 * ms);
}
lv::task<> slide_main(lv::view<"arc"> arc, int to, outcome* out) {
  out->finished = co_await lv::animate(lv::in<"main">(arc), {.translate_x = {0 * px, to * px}}, 200 * ms);
}

void a_newer_animation_of_the_same_selector_wins(lv::obj screen) {
  auto arc = screen.add<"arc">();
  outcome first, second, third;
  check(lv::spawn(slide(arc, 100, &first)));
  lv_test_wait(50);
  // The plain call and lv::in<"main"> are one selector: the newer one takes over.
  check(lv::spawn(slide_main(arc, 40, &second)));
  lv_test_wait(1);
  check(!first.finished);
  lv_test_wait(250);
  check(second.finished && local(arc.raw(), LV_STYLE_TRANSLATE_X, 0) == 40);
  check(lv_anim_count_running() == 0);
  lv_obj_delete(arc.raw());
  (void)third;
}

// ---- when the task ends ------------------------------------------------------------------------------------

void a_cancelled_animation_is_deleted_and_the_value_stays(lv::obj screen) {
  auto owner = screen.add<"obj">();
  auto label = screen.add<"label">();
  outcome out;
  check(lv::spawn(fade(label, 400 * ms, &out), owner));
  lv_test_wait(150);
  const auto middle = local(label.raw(), LV_STYLE_OPA, 0);
  check(middle > LV_OPA_TRANSP && middle < LV_OPA_COVER && lv_anim_count_running() == 1);
  lv_obj_delete(owner.raw());  // the task ends with its owner
  check(out.ended && lv_anim_count_running() == 0);
  lv_test_wait(300);
  check(local(label.raw(), LV_STYLE_OPA, 0) == middle);  // where it was
  lv_obj_delete(label.raw());
}

lv::task<> fade_or_sleep(lv::view<"label"> label, int* answer) {
  *answer = co_await lv::first(lv::animate(label, {.opa = {0 * pct, 100 * pct}}, 400 * ms).then([](bool finished) {
                                 return finished ? 1 : -1;
                               }),
                               lv::sleep(100 * ms).then([] { return 2; }));
}

void an_animation_that_loses_in_first_is_deleted(lv::obj screen) {
  auto label = screen.add<"label">();
  int answer = 0;
  check(lv::spawn(fade_or_sleep(label, &answer)));
  lv_test_wait(150);
  check(answer == 2 && lv_anim_count_running() == 0);
  const auto stopped = local(label.raw(), LV_STYLE_OPA, 0);
  check(stopped > LV_OPA_TRANSP && stopped < LV_OPA_COVER);
  lv_test_wait(400);
  check(local(label.raw(), LV_STYLE_OPA, 0) == stopped);
  lv_obj_delete(label.raw());
}

// Each breaks a precondition of lv::animate on a part or a state. The values come from the command line,
// so that the compiler cannot fold the call.
int broken(std::string_view which, int argc) {
  lv::view<"arc"> arc{lv_arc_create(lv_screen_active())};
  const double far = argc * 2.0;  // 4: past the end of a curve's x
  if (which == "in_no_widget")
    static_cast<void>(lv::animate(lv::in<"knob">(lv::view<"arc">{nullptr}), {.opa = {0 * pct, 100 * pct}}, 100 * ms));
  if (which == "in_no_property") static_cast<void>(lv::animate(lv::in<"knob">(arc), {}, 100 * ms));
  if (which == "in_two_properties")
    static_cast<void>(lv::animate(lv::in<"knob">(arc), {.opa = {0 * pct, 100 * pct}, .translate_x = {0 * px, 5 * px}}, 100 * ms));
  if (which == "in_too_long")
    static_cast<void>(lv::animate(lv::in<"knob">(arc), {.opa = {0 * pct, 100 * pct}}, lv::duration{std::uint32_t{1} << (argc * 15 + 1)}));
  if (which == "in_unusable_curve")
    static_cast<void>(lv::animate(lv::in<"knob">(arc), {.opa = {0 * pct, 100 * pct}}, 100 * ms, lv::bezier{far, 0, 0.5, 1}));
  std::printf("no contract violation\n");
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  lv_init();
  lv_test_display_create(320, 240);
  auto screen = lv::screen_active();
  if (argc == 2) return broken(argv[1], argc);

  the_main_part_is_animated_by_the_plain_call(screen);
  a_part_and_a_state_are_named_with_lv_in(screen);
  a_newer_animation_of_the_same_selector_wins(screen);
  a_cancelled_animation_is_deleted_and_the_value_stays(screen);
  an_animation_that_loses_in_first_is_deleted(screen);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
