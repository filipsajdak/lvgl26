// lv::shown with .alive above 2: the pages shown most recently stay built and hidden, a kept page
// comes back without being built again (its .on_front runs, its .on_shown does not), and the one
// shown longest ago is dropped when a slot is needed.
#include <lv/lv.hpp>
#include <lv/shown.hpp>

#include "check.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>

namespace {

using namespace lv::units;

enum class tab { a, b, c, d };
constinit lv::state<tab> current{tab::a};

constexpr auto page_a = lv::column({.size = {100 * px, 50 * px}})[lv::label({.text = "a"})];
constexpr auto page_b = lv::column({.size = {100 * px, 50 * px}})[lv::label({.text = "b"})];
constexpr auto page_c = lv::column({.size = {100 * px, 50 * px}})[lv::label({.text = "c"})];
constexpr auto page_d = lv::column({.size = {100 * px, 50 * px}})[lv::label({.text = "d"})];

int a_built = 0;
int a_front = 0;
void build_a(lv::held<page_a>) { ++a_built; }
void front_a(lv::held<page_a> page) {
  check(page.has_value());
  ++a_front;
}

// Three slots: the page in front, room for a leaving one, and one kept page.
constexpr auto kept_one = lv::group({.size = {100 * px, 50 * px}})[
    lv::shown(current, {.alive = 3})[
        lv::is<tab::a>(page_a, {.on_shown = &build_a, .on_front = &front_a}),
        lv::is<tab::b>(page_b), lv::is<tab::c>(page_c), lv::is<tab::d>(page_d)]];

lv_obj_t* root_of(lv_obj_t* holder, std::string_view text) {
  for (std::uint32_t i = 0; i < lv_obj_get_child_count(holder); ++i) {
    lv_obj_t* root = lv_obj_get_child(holder, static_cast<std::int32_t>(i));
    if (lv_obj_get_child_count(root) > 0 && std::string_view{lv_label_get_text(lv_obj_get_child(root, 0))} == text)
      return root;
  }
  return nullptr;
}

std::size_t visible_pages(lv_obj_t* holder) {
  std::size_t n = 0;
  for (std::uint32_t i = 0; i < lv_obj_get_child_count(holder); ++i)
    n += lv_obj_is_hidden(lv_obj_get_child(holder, static_cast<std::int32_t>(i))) ? 0u : 1u;
  return n;
}

void a_kept_page_comes_back(lv::obj screen) {
  current = tab::a;
  a_built = a_front = 0;
  auto w = lv::mount<kept_one>(screen);
  lv_obj_t* holder = w.root().raw();
  lv_obj_t* a = root_of(holder, "a");
  check(a_built == 1 && a_front == 1);
  current = tab::b;
  lv::settle();
  check(root_of(holder, "a") == a && lv_obj_is_hidden(a) && visible_pages(holder) == 1);
  current = tab::a;
  lv::settle();
  check(root_of(holder, "a") == a && !lv_obj_is_hidden(a));  // the same widgets
  check(a_built == 1 && a_front == 2);                         // not built again: .on_front alone
  check(lv_obj_is_hidden(root_of(holder, "b")) && visible_pages(holder) == 1);
}

void the_oldest_kept_page_goes(lv::obj screen) {
  current = tab::a;
  a_built = 0;
  auto w = lv::mount<kept_one>(screen);
  lv_obj_t* holder = w.root().raw();
  current = tab::b;  // a kept
  lv::settle();
  current = tab::c;  // b kept; one kept page only, so a goes
  lv::settle();
  check(root_of(holder, "a") == nullptr && root_of(holder, "b") != nullptr && lv_obj_get_child_count(holder) == 2);
  current = tab::a;  // built again
  lv::settle();
  check(a_built == 2 && root_of(holder, "c") != nullptr && root_of(holder, "b") == nullptr);
}

// With the default of 2 nothing is kept: the old page is deleted first, as before.
constexpr auto default_two = lv::group({.size = {100 * px, 50 * px}})[
    lv::shown(current)[lv::is<tab::a>(page_a), lv::is<tab::b>(page_b), lv::otherwise()]];

void two_keeps_nothing(lv::obj screen) {
  current = tab::a;
  auto w = lv::mount<default_two>(screen);
  current = tab::b;
  lv::settle();
  check(lv_obj_get_child_count(w.root().raw()) == 1 && root_of(w.root().raw(), "a") == nullptr);
}

// With transitions: a page that has left is kept hidden, and comes back with its enter.
constexpr auto slide_in = lv::transition({.x = {100 * px, 0 * px}}, 100 * ms);
constexpr auto slide_out = lv::transition({.x = {0 * px, 100 * px}}, 100 * ms);
constexpr auto sliding = lv::group({.size = {100 * px, 50 * px}})[
    lv::shown(current, {.alive = 3, .enter = slide_in, .leave = slide_out})[
        lv::is<tab::a>(page_a, {.on_shown = &build_a, .on_front = &front_a}),
        lv::is<tab::b>(page_b), lv::otherwise()]];

void kept_after_its_leave(lv::obj screen) {
  current = tab::a;
  a_built = a_front = 0;
  auto w = lv::mount<sliding>(screen);
  lv_obj_t* holder = w.root().raw();
  lv_obj_t* a = root_of(holder, "a");
  current = tab::b;
  lv::settle();
  lv_test_wait(150);
  check(root_of(holder, "a") == a && lv_obj_is_hidden(a));
  current = tab::a;
  lv::settle();
  check(!lv_obj_is_hidden(a) && a_built == 1 && a_front == 2);
  lv_test_wait(150);
  check(lv_obj_get_x(a) == 0 && lv_obj_is_hidden(root_of(holder, "b")));
}

// Pages of other files: one block of .alive slots from LVGL's heap, given back with the tree.
constinit const lv::page a_page = lv::page_of<page_a>;
constinit const lv::page b_page = lv::page_of<page_b>;
constexpr auto of_files = lv::group({.size = {100 * px, 50 * px}})[
    lv::shown(current, {.alive = 4})[lv::is<tab::a>(a_page), lv::is<tab::b>(b_page), lv::otherwise()]];

std::size_t heap_used() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.total_size - m.free_size;
}

void pages_of_files_kept(lv::obj screen) {
  current = tab::a;
  const std::size_t before = heap_used();
  {
    auto w = lv::mount<of_files>(screen);
    lv_obj_t* a = root_of(w.root().raw(), "a");
    current = tab::b;
    lv::settle();
    current = tab::a;
    lv::settle();
    check(root_of(w.root().raw(), "a") == a && !lv_obj_is_hidden(a));
  }
  check(heap_used() == before);
}

}  // namespace

int main() {
  lv_init();
  lv_display_t* display = lv_test_display_create(320, 240);
  lv_display_set_theme(display, nullptr);
  auto screen = lv::screen_active();

  a_kept_page_comes_back(screen);
  the_oldest_kept_page_goes(screen);
  two_keeps_nothing(screen);
  kept_after_its_leave(screen);
  pages_of_files_kept(screen);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
