// lv::shown with .enter and .leave: the new page is built in a free slot before the old one
// leaves, a page asked for again while it leaves comes back from where it stands, and a leaving
// page takes no clicks. And pages of other files (lv::page), whose slots come from LVGL's heap.
#include <lv/lv.hpp>
#include <lv/shown.hpp>

#include "check.hpp"
#include "full_heap.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

namespace {

using namespace lv::units;

constexpr std::int32_t width = 200;

enum class sub { none, a, b, c };
constinit lv::state<sub> current{sub::none};

int clicks_on_a = 0;
void click_a() { ++clicks_on_a; }

constexpr auto page_a = lv::column({.size = {width * px, 100 * px}})[
    lv::label({.text = "a"}), lv::button({.on_clicked = &click_a, .size = {width * px, 60 * px}})];
constexpr auto page_b = lv::column({.size = {width * px, 100 * px}})[lv::label({.text = "b"})];
constexpr auto page_c = lv::column({.size = {width * px, 100 * px}})[lv::label({.text = "c"})];

int a_built = 0;
std::string order;  // what ran for page a: s for .on_shown, f for .on_front
void on_a(lv::held<page_a>) {
  ++a_built;
  order += 's';
}
void front_a(lv::held<page_a> page) {
  check(page.has_value());
  order += 'f';
}

constexpr auto slide_in = lv::transition({.x = {width * px, 0 * px}}, 200 * ms);
constexpr auto slide_out = lv::transition({.x = {0 * px, width * px}}, 200 * ms);

constexpr auto sliding = lv::group({.size = {width * px, 100 * px}})[
    lv::shown(current, {.enter = slide_in, .leave = slide_out})[
        lv::is<sub::none>(),
        lv::is<sub::a>(page_a, {.on_shown = &on_a, .on_front = &front_a}),
        lv::is<sub::b>(page_b),
        lv::is<sub::c>(page_c)]];

// The pages under the holder: their roots, without the glass, which is a plain widget with no child.
std::size_t pages_in(lv_obj_t* holder) {
  std::size_t n = 0;
  for (std::uint32_t i = 0; i < lv_obj_get_child_count(holder); ++i)
    if (lv_obj_get_child_count(lv_obj_get_child(holder, static_cast<std::int32_t>(i))) > 0) ++n;
  return n;
}

lv_obj_t* root_of(lv_obj_t* holder, std::string_view text) {
  for (std::uint32_t i = 0; i < lv_obj_get_child_count(holder); ++i) {
    lv_obj_t* root = lv_obj_get_child(holder, static_cast<std::int32_t>(i));
    if (lv_obj_get_child_count(root) > 0 && std::string_view{lv_label_get_text(lv_obj_get_child(root, 0))} == text)
      return root;
  }
  return nullptr;
}

std::size_t heap_used() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.total_size - m.free_size;
}

// The new page is built before the old one leaves; both are alive until the leave ends.
void slides_between_pages(lv::obj screen) {
  current = sub::a;
  auto w = lv::mount<sliding>(screen);
  check(w.has_value());
  lv_obj_t* holder = w.root().raw();
  check(pages_in(holder) == 1);
  current = sub::b;
  lv::settle();
  check(pages_in(holder) == 2);
  lv_obj_t* a = root_of(holder, "a");
  lv_obj_t* b = root_of(holder, "b");
  check(a != nullptr && b != nullptr);
  lv_test_wait(100);
  check(lv_obj_get_x(a) > 0 && lv_obj_get_x(a) < width && lv_obj_get_x(b) > 0 && lv_obj_get_x(b) < width);
  lv_test_wait(150);
  check(pages_in(holder) == 1 && root_of(holder, "a") == nullptr && lv_obj_get_x(root_of(holder, "b")) == 0);
  // To nothing: the page leaves, and nothing comes in.
  current = sub::none;
  lv::settle();
  check(pages_in(holder) == 1);
  lv_test_wait(250);
  check(pages_in(holder) == 0);
}

// A page asked for again while it leaves turns round where it stands, and is not built again.
void comes_back_while_it_leaves(lv::obj screen) {
  current = sub::a;
  a_built = 0;
  order.clear();
  auto w = lv::mount<sliding>(screen);
  lv_obj_t* holder = w.root().raw();
  current = sub::b;
  lv::settle();
  lv_test_wait(100);
  lv_obj_t* a = root_of(holder, "a");
  const std::int32_t a_at = lv_obj_get_x(a);
  current = sub::a;
  lv::settle();
  check(a_built == 1 && root_of(holder, "a") == a);  // the same widgets
  check(order == "sff");  // built: .on_shown, then .on_front; back to the front: .on_front alone
  lv_test_wait(20);
  check(lv_obj_get_x(a) < a_at);  // on its way back
  lv_test_wait(300);
  check(pages_in(holder) == 1 && root_of(holder, "a") == a && lv_obj_get_x(a) == 0);
}

// A third page while two are alive: the leaving one is cut, the entering one leaves from where it
// stands, and the third comes in. Never more than two pages.
void a_third_page_cuts_the_leaving_one(lv::obj screen) {
  current = sub::a;
  auto w = lv::mount<sliding>(screen);
  lv_obj_t* holder = w.root().raw();
  current = sub::b;
  lv::settle();
  lv_test_wait(100);
  lv_obj_t* b = root_of(holder, "b");
  const std::int32_t b_at = lv_obj_get_x(b);
  current = sub::c;
  lv::settle();
  check(pages_in(holder) == 2 && root_of(holder, "a") == nullptr && root_of(holder, "b") == b);
  lv_test_wait(20);
  check(lv_obj_get_x(b) > b_at);  // b leaves to the right, from where it stood
  lv_test_wait(300);
  check(pages_in(holder) == 1 && root_of(holder, "c") != nullptr);
}

// A tap on a leaving page reaches nothing of it.
void a_leaving_page_takes_no_clicks(lv::obj screen) {
  lv_test_indev_create_all();
  current = sub::a;
  clicks_on_a = 0;
  auto w = lv::mount<sliding>(screen);
  lv_obj_t* holder = w.root().raw();
  lv_refr_now(nullptr);
  lv_area_t at;
  lv_obj_get_coords(lv_obj_get_child(root_of(holder, "a"), 1), &at);
  lv_test_mouse_click_at((at.x1 + at.x2) / 2, (at.y1 + at.y2) / 2);
  check(clicks_on_a == 1);
  current = sub::none;
  lv::settle();
  lv_test_wait(20);
  lv_obj_get_coords(lv_obj_get_child(root_of(holder, "a"), 1), &at);
  lv_test_mouse_click_at(std::min<std::int32_t>(at.x1 + 5, width - 1), (at.y1 + at.y2) / 2);
  check(clicks_on_a == 1);
  lv_test_wait(300);
}

// With a transition both pages are alive during it: the heap holds two pages at its peak.
void the_peak_holds_two_pages(lv::obj screen) {
  current = sub::none;
  auto w = lv::mount<sliding>(screen);
  const std::size_t base = heap_used();
  current = sub::a;
  lv::settle();
  lv_test_wait(250);
  const std::size_t with_a = heap_used() - base;
  current = sub::b;
  lv::settle();
  const std::size_t both = heap_used() - base;
  check(both > with_a);  // b is built while a still leaves
  lv_test_wait(250);
}

// ---- pages from other files ----

enum class screen_page { first, second };
constinit lv::state<screen_page> current_page{screen_page::first};

constexpr auto first_ui = lv::column({.size = {width * px, 100 * px}})[lv::label({.id = "title", .text = "first"})];
constexpr auto second_ui = lv::column({.size = {width * px, 100 * px}})[
    lv::label({.id = "title", .text = "second"}), lv::label({.id = "extra", .text = "more"})];

int second_shown = 0;
int second_front = 0;
void on_second(lv::held<second_ui> page) {
  ++second_shown;
  check(second_front == second_shown - 1);  // .on_shown runs before .on_front
  check(page.has_value() && std::string_view{lv_label_get_text(page->get<"extra">().raw())} == "more");
}
void front_second(lv::held<second_ui> page) {
  ++second_front;
  check(page.has_value());
}

// What another file would hold: the pages, defined there and declared where they are shown.
constinit const lv::page first_page = lv::page_of<first_ui>;
constinit const lv::page second_page = lv::page_of<second_ui, {.on_shown = &on_second, .on_front = &front_second}>;

constexpr auto pages_window = lv::group({.size = {width * px, 100 * px}})[
    lv::shown(current_page)[lv::is<screen_page::first>(first_page), lv::is<screen_page::second>(second_page)]];

static_assert(lv::page_of<first_ui>.size == sizeof(lv::mounted<first_ui>) && lv::page_of<second_ui>.align == alignof(lv::mounted<second_ui>));

// The slots of pages of other files come from LVGL's heap at mount, as one block, and go with
// the tree. A switch takes nothing more.
void pages_of_other_files(lv::obj screen) {
  current_page = screen_page::first;
  second_shown = 0;
  second_front = 0;
  const std::size_t before = heap_used();
  {
    auto w = lv::mount<pages_window>(screen);
    check(w.has_value());
    current_page = screen_page::second;
    lv::settle();
    check(second_shown == 1);
    const std::size_t with_second = heap_used();
    current_page = screen_page::first;
    lv::settle();
    current_page = screen_page::second;
    lv::settle();
    check(second_shown == 2 && second_front == 2 && heap_used() == with_second);
  }
  check(heap_used() == before);
}

// Without room for the block of slots the mount fails, as for any other lack of memory.
void no_room_for_the_slots(lv::obj screen) {
  lv_log_register_print_cb([](lv_log_level_t, const char*) {});
  lv::mounted<pages_window> w;
  bool failed = false;
  for (std::size_t room = 0; room < 4096 && !failed; room += 8) {
    full_heap full{room};
    auto r = w.mount(screen);
    failed = !r.has_value() && r.error() == lv::mount_error::out_of_memory;
    if (r.has_value()) w.unmount();
  }
  check(failed);
  lv_log_register_print_cb(nullptr);
}

// A transition made at run time, from values the compiler cannot fold.
void a_transition_at_run_time(int argc) {
  const std::int32_t from = 100 + argc;
  const auto t = lv::transition({.y = {from * px, 0 * px}}, (150 + argc) * ms);
  check(t.from == from && t.to == 0 && t.ms == static_cast<std::uint32_t>(150 + argc) && t.step != nullptr);
  check(t.step != slide_in.step);  // y, not x
}

}  // namespace

int main(int argc, char**) {
  lv_init();
  lv_display_t* display = lv_test_display_create(320, 240);
  lv_display_set_theme(display, nullptr);
  lv_display_add_event_cb(display, &lv::detail::drain_deferred, LV_EVENT_REFR_START, nullptr);
  auto screen = lv::screen_active();

  slides_between_pages(screen);
  comes_back_while_it_leaves(screen);
  a_third_page_cuts_the_leaving_one(screen);
  a_leaving_page_takes_no_clicks(screen);
  the_peak_holds_two_pages(screen);
  pages_of_other_files(screen);
  no_room_for_the_slots(screen);
  a_transition_at_run_time(argc);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
