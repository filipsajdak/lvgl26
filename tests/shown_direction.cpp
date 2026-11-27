// lv::shown with .enter_back and .leave_back: a change to a value that comes before the one shown, in the
// order the enum declares them, plays the transitions of the way back. Without them it plays the
// transitions of the way forward.
//
//   shown_direction <forward half way.png> <back half way.png>

#include <lv/lv.hpp>
#include <lv/shown.hpp>

#include "check.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>

namespace {

using namespace lv::units;

constexpr std::int32_t width = 200;

enum class step { one, two, three };
constinit lv::state<step> current{step::one};
constinit lv::state<step> plain{step::one};

constexpr lv_color_t red{.blue = 40, .green = 40, .red = 220};
constexpr lv_color_t green{.blue = 40, .green = 200, .red = 40};
constexpr lv_color_t blue{.blue = 220, .green = 60, .red = 40};

template <const lv_color_t& Color>
constexpr auto page_of_color(const char* text) {
  return lv::column({.size = {width * px, 100 * px}, .style = {.bg_color = lv::raw(Color), .bg_opa = lv::raw(lv::opa::cover)}},
                    lv::label({.text = text}));
}
constexpr auto page_one = page_of_color<red>("one");
constexpr auto page_two = page_of_color<green>("two");
constexpr auto page_three = page_of_color<blue>("three");

constexpr auto in_forward = lv::transition({.x = {width * px, 0 * px}}, 200 * ms);
constexpr auto out_forward = lv::transition({.x = {0 * px, -width * px}}, 200 * ms);
constexpr auto in_back = lv::transition({.x = {-width * px, 0 * px}}, 200 * ms);
constexpr auto out_back = lv::transition({.x = {0 * px, width * px}}, 200 * ms);

constexpr auto both_ways = lv::group({.size = {width * px, 100 * px}},
                                     lv::shown(current,
                                               {.enter = in_forward, .leave = out_forward, .enter_back = in_back, .leave_back = out_back},
                                               lv::is<step::one>(page_one), lv::is<step::two>(page_two), lv::is<step::three>(page_three)));

// Only the transitions of the way forward: the way back plays them too.
constexpr auto one_way = lv::group({.size = {width * px, 100 * px}},
                                   lv::shown(plain, {.enter = in_forward, .leave = out_forward}, lv::is<step::one>(page_one),
                                             lv::is<step::two>(page_two), lv::is<step::three>(page_three)));

std::size_t pages_in(lv_obj_t* holder) {
  std::size_t n = 0;
  for (std::uint32_t i = 0; i < lv_obj_get_child_count(holder); ++i)
    if (lv_obj_get_child_count(lv_obj_get_child(holder, static_cast<std::int32_t>(i))) > 0) ++n;
  return n;
}

lv_obj_t* root_of(lv_obj_t* holder, std::string_view text) {
  for (std::uint32_t i = 0; i < lv_obj_get_child_count(holder); ++i) {
    lv_obj_t* root = lv_obj_get_child(holder, static_cast<std::int32_t>(i));
    if (lv_obj_get_child_count(root) > 0 && std::string_view{lv_label_get_text(lv_obj_get_child(root, 0))} == text) return root;
  }
  return nullptr;
}

void forward_comes_in_from_the_right(lv::obj screen, const char* picture) {
  current = step::one;
  auto w = lv::mount<both_ways>(screen);
  check(w.has_value());
  lv_obj_t* holder = w.root().raw();
  current = step::two;
  lv::settle();
  lv_obj_t* one = root_of(holder, "one");
  lv_obj_t* two = root_of(holder, "two");
  check(one != nullptr && two != nullptr);
  lv_test_wait(5);  // LVGL sets the first value at its next step
  check(lv_obj_get_x(two) > width * 3 / 4 && lv_obj_get_x(one) <= 0 && lv_obj_get_x(one) > -width / 4);  // two comes from the right
  lv_test_wait(95);
  check(lv_obj_get_x(two) > 0 && lv_obj_get_x(two) < width && lv_obj_get_x(one) < 0 && lv_obj_get_x(one) > -width);
  check(lv_test_screenshot_compare(picture) == LV_TEST_SCREENSHOT_RESULT_PASSED);
  lv_test_wait(250);
  check(pages_in(holder) == 1 && lv_obj_get_x(root_of(holder, "two")) == 0);
}

void back_comes_in_from_the_left(lv::obj screen, const char* picture) {
  current = step::two;
  auto w = lv::mount<both_ways>(screen);
  lv_obj_t* holder = w.root().raw();
  current = step::one;
  lv::settle();
  lv_obj_t* one = root_of(holder, "one");
  lv_obj_t* two = root_of(holder, "two");
  check(one != nullptr && two != nullptr);
  lv_test_wait(5);
  check(lv_obj_get_x(one) < -width * 3 / 4 && lv_obj_get_x(two) >= 0 && lv_obj_get_x(two) < width / 4);  // one comes from the left
  lv_test_wait(95);
  check(lv_obj_get_x(one) < 0 && lv_obj_get_x(one) > -width && lv_obj_get_x(two) > 0 && lv_obj_get_x(two) < width);
  check(lv_test_screenshot_compare(picture) == LV_TEST_SCREENSHOT_RESULT_PASSED);
  lv_test_wait(250);
  check(pages_in(holder) == 1 && lv_obj_get_x(root_of(holder, "one")) == 0);
}

// The direction is the order of the enumerators, whatever the distance: three to one is back, one to three is forward.
void the_order_of_the_values_is_the_direction(lv::obj screen) {
  current = step::three;
  auto w = lv::mount<both_ways>(screen);
  lv_obj_t* holder = w.root().raw();
  current = step::one;
  lv::settle();
  lv_test_wait(5);
  check(lv_obj_get_x(root_of(holder, "one")) < -width * 3 / 4);
  lv_test_wait(250);
  current = step::three;
  lv::settle();
  lv_test_wait(5);
  check(lv_obj_get_x(root_of(holder, "three")) > width * 3 / 4);
  lv_test_wait(250);
  // The same value again changes nothing.
  current = step::three;
  lv::settle();
  check(pages_in(holder) == 1);
}

// Without transitions for the way back, the way back plays the ones of the way forward.
void without_back_transitions_the_way_back_plays_the_forward_ones(lv::obj screen) {
  plain = step::two;
  auto w = lv::mount<one_way>(screen);
  lv_obj_t* holder = w.root().raw();
  plain = step::one;
  lv::settle();
  lv_test_wait(5);
  check(lv_obj_get_x(root_of(holder, "one")) > width * 3 / 4 && lv_obj_get_x(root_of(holder, "two")) <= 0);  // from the right
  lv_test_wait(250);
  check(pages_in(holder) == 1);
}

// A change back while a page comes in turns it round: the page that came in leaves on the way back.
void a_change_while_a_page_comes_in(lv::obj screen) {
  current = step::one;
  auto w = lv::mount<both_ways>(screen);
  lv_obj_t* holder = w.root().raw();
  current = step::two;
  lv::settle();
  lv_test_wait(80);
  lv_obj_t* two = root_of(holder, "two");
  const std::int32_t two_at = lv_obj_get_x(two);
  check(two_at > 0 && two_at < width);
  current = step::one;  // back, while two comes in
  lv::settle();
  lv_test_wait(20);
  check(lv_obj_get_x(two) > two_at);  // two leaves to the right, from where it stood
  lv_test_wait(300);
  check(pages_in(holder) == 1 && root_of(holder, "one") != nullptr && lv_obj_get_x(root_of(holder, "one")) == 0);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    std::printf("usage: shown_direction <forward half way.png> <back half way.png>\n");
    return 2;
  }
  lv_init();
  lv_display_t* display = lv_test_display_create(width, 100);
  lv_display_set_theme(display, nullptr);
  lv_display_add_event_cb(display, &lv::detail::drain_deferred, LV_EVENT_REFR_START, nullptr);
  auto screen = lv::screen_active();

  forward_comes_in_from_the_right(screen, argv[1]);
  back_comes_in_from_the_left(screen, argv[2]);
  the_order_of_the_values_is_the_direction(screen);
  without_back_transitions_the_way_back_plays_the_forward_ones(screen);
  a_change_while_a_page_comes_in(screen);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
