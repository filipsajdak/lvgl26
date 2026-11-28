// display.follow(state, themes): the display's theme is the one of the state's value, and a change of the
// value restyles every widget in place. Signal's day and dark flip a button, an arc and the screen.
//
//   theme_follow <day reference.png> <dark reference.png>
//   theme_follow shape        (ends in a contract violation: two themes of different shapes)
//   theme_follow <case>       (each of the cases in main ends in a contract violation)

#include <lv/lv.hpp>
#include <lv/signal.hpp>
#include <lv/theme.hpp>

#include "check.hpp"

#include <cstddef>
#include <cstring>

namespace {

using namespace lv::units;
using lv::signal::mode;

constexpr auto ui = lv::column({.size = {160 * px, 120 * px}},
                               lv::button({.id = "start"}, lv::label({.text = "Start"})),
                               lv::arc({.id = "ring", .size = {60 * px, 60 * px}, .value = 60}),
                               lv::group({.id = "group"}, lv::button({.id = "inner"})));

constinit lv::frame<160, 120> frame;
constinit lv::state<mode> chosen{mode::day};

bool same(lv_color_t a, lv_color_t b) { return a.red == b.red && a.green == b.green && a.blue == b.blue; }
lv_color_t bg(lv_obj_t* o) { return lv_obj_get_style_bg_color(o, LV_PART_MAIN); }
lv_color_t ring_of(lv_obj_t* o) { return lv_obj_get_style_arc_color(o, LV_PART_INDICATOR); }

std::size_t used_bytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.total_size - m.free_size;
}

template <class Tokens>
void shows(lv::mounted<ui>& root, lv::display& display, const Tokens& tok) {
  check(same(bg(display.screen().raw()), *tok.color.ground));
  check(same(bg(root.get<"start">().raw()), *tok.color.active));
  check(same(ring_of(root.get<"ring">().raw()), *tok.color.active));
  check(same(lv_obj_get_style_text_color(root.get<"start">().raw(), LV_PART_MAIN), *tok.color.on_active));
}

void the_display_follows_the_state(const char* day_picture, const char* dark_picture) {
  lv::application app;
  lv::display display{app, frame};
  display.follow(chosen, {lv::signal::day, lv::signal::dark});

  auto root = lv::mount<ui>(display.screen());
  check(root.has_value());
  if (!root) return;
  shows(*root, display, lv::signal::day_tokens);
  check(lv_test_screenshot_compare(day_picture) == LV_TEST_SCREENSHOT_RESULT_PASSED);

  chosen = mode::dark;
  shows(*root, display, lv::signal::dark_tokens);
  // A button in a group is a button of the theme; the group itself held none.
  check(same(bg(root->get<"inner">().raw()), *lv::signal::dark_tokens.color.active));
  check(lv_test_screenshot_compare(dark_picture) == LV_TEST_SCREENSHOT_RESULT_PASSED);

  // A widget made after the switch gets the new theme.
  lv_obj_t* late = lv_button_create(display.screen().raw());
  check(same(bg(late), *lv::signal::dark_tokens.color.active));

  chosen = mode::day;
  shows(*root, display, lv::signal::day_tokens);
  check(same(bg(late), *lv::signal::day_tokens.color.active));
  chosen = mode::day;  // the same value: nothing happens
  check(same(bg(late), *lv::signal::day_tokens.color.active));

  // LVGL makes a few things of its own on the first change of a screen that was drawn. Then a switch
  // restyles in place: nothing is allocated or freed, in either direction.
  chosen = mode::dark;
  chosen = mode::day;
  const auto before = used_bytes();
  chosen = mode::dark;
  check(used_bytes() == before);
  chosen = mode::day;
  check(used_bytes() == before);
  chosen = mode::dark;
}

// A state that is deleted while the display lives, and a display that is deleted while the state lives.
void either_may_end_first() {
  lv::application app;
  {
    lv::state<mode> brief{mode::dark};
    lv::display display{app, frame};
    display.follow(brief, {lv::signal::day, lv::signal::dark});
    check(same(bg(display.screen().raw()), *lv::signal::dark_tokens.color.ground));
    brief.reset();  // the state ends its watchers; the display still shows dark
    check(same(bg(display.screen().raw()), *lv::signal::dark_tokens.color.ground));
  }
  chosen = mode::day;
  {
    lv::display display{app, frame};
    display.follow(chosen, {lv::signal::day, lv::signal::dark});
  }
  chosen = mode::dark;  // no display follows it any more
  check(chosen.get() == mode::dark);
}

// A display that has a theme of its own, of the shape of the themes it then follows.
void a_theme_of_the_constructor_is_the_first_one() {
  lv::application app;
  lv::display display{app, frame, lv::signal::day};
  display.follow(chosen, {lv::signal::day, lv::signal::dark});
  check(same(bg(display.screen().raw()), *lv::signal::dark_tokens.color.ground));  // chosen is dark
  chosen = mode::day;
  check(same(bg(display.screen().raw()), *lv::signal::day_tokens.color.ground));
}

constexpr auto other_shape = lv::theme({.screen = {.style = {.bg_color = lv::raw(lv_color_t{.blue = 1, .green = 2, .red = 3})}}});

void themes_of_two_shapes() {
  lv::application app;
  lv::display display{app, frame};
  display.follow(chosen, {lv::signal::day, other_shape});  // the contract ends the program here
}

// A display follows a state of an enum: one theme for each enumerator, in their order.
template <class S>
concept follows = requires(lv::display& d, const S& s) { d.follow(s, {lv::signal::day, lv::signal::dark}); };
static_assert(follows<lv::state<mode>> && !follows<lv::state<bool>>);

enum class one { only };
enum class three { a, b, c };
enum class eight { a, b, c, d, e, f, g, h };
enum class nine { a, b, c, d, e, f, g, h, i };
constinit lv::state<one> single{one::only};
constinit lv::state<three> trio{three::a};
constinit lv::state<eight> octet{eight::h};
constinit lv::state<nine> nonet{nine::a};

// As many themes as a display can follow, the most there is room for.
void the_most_themes() {
  lv::application app;
  lv::display display{app, frame};
  display.follow(octet, {lv::signal::day, lv::signal::dark, lv::signal::day, lv::signal::dark, lv::signal::day,
                         lv::signal::dark, lv::signal::day, lv::signal::dark});
  check(same(bg(display.screen().raw()), *lv::signal::dark_tokens.color.ground));  // the eighth
  octet = eight::a;
  check(same(bg(display.screen().raw()), *lv::signal::day_tokens.color.ground));
}

// Each of these breaks a precondition of follow(), or a contract inside it.
void one_theme() {
  lv::application app;
  lv::display display{app, frame};
  display.follow(single, {lv::signal::day});
}

void more_themes_than_room() {
  lv::application app;
  lv::display display{app, frame};
  display.follow(nonet, {lv::signal::day, lv::signal::dark, lv::signal::day, lv::signal::dark, lv::signal::day,
                         lv::signal::dark, lv::signal::day, lv::signal::dark, lv::signal::day});
}

void fewer_themes_than_values() {
  lv::application app;
  lv::display display{app, frame};
  display.follow(trio, {lv::signal::day, lv::signal::dark});
}

void more_themes_than_values() {
  lv::application app;
  lv::display display{app, frame};
  display.follow(chosen, {lv::signal::day, lv::signal::dark, lv::signal::day});
}

void following_twice() {
  lv::application app;
  lv::display display{app, frame};
  display.follow(chosen, {lv::signal::day, lv::signal::dark});
  display.follow(chosen, {lv::signal::day, lv::signal::dark});
}

void a_constructor_theme_of_another_shape() {
  lv::application app;
  lv::display display{app, frame, other_shape};
  display.follow(chosen, {lv::signal::day, lv::signal::dark});
}

// LVGL's heap has no room for the theme that the first switch makes.
void no_room_for_the_theme() {
  lv::application app;
  lv::display display{app, frame};
  for (std::size_t size = std::size_t{1} << 24; size > 0; size /= 2)
    while (lv_malloc(size) != nullptr) {
    }
  display.follow(chosen, {lv::signal::day, lv::signal::dark});
}

struct violation {
  const char* name;
  void (*run)();
};
constexpr violation violations[] = {
    {"shape", &themes_of_two_shapes},
    {"one_theme", &one_theme},
    {"more_themes_than_room", &more_themes_than_room},
    {"fewer_themes_than_values", &fewer_themes_than_values},
    {"more_themes_than_values", &more_themes_than_values},
    {"following_twice", &following_twice},
    {"constructor_shape", &a_constructor_theme_of_another_shape},
    {"no_room_for_the_theme", &no_room_for_the_theme},
};

}  // namespace

int main(int argc, char** argv) {
  for (const violation& v : violations)
    if (argc == 2 && std::strcmp(argv[1], v.name) == 0) {
      v.run();
      std::printf("no contract violation\n");
      return 0;
    }
  if (argc != 3) {
    std::printf("usage: theme_follow <day reference.png> <dark reference.png>\n");
    return 2;
  }
  chosen = mode::day;
  the_display_follows_the_state(argv[1], argv[2]);
  chosen = mode::dark;
  either_may_end_first();
  chosen = mode::dark;
  a_theme_of_the_constructor_is_the_first_one();
  the_most_themes();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
