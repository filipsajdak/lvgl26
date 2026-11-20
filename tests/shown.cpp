// lv::shown: a node that follows a state of an enum or a bool and shows the tree of its value,
// in the storage of the tree that holds it; and lv::when for a bool.
#include <lv/lv.hpp>
#include <lv/one_of.hpp>
#include <lv/repeat.hpp>
#include <lv/shown.hpp>

#include "check.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string_view>

namespace {

using namespace lv::units;

enum class page { home, settings, ink, none };

constinit lv::state<page> current{page::home};
constinit lv::state<bool> online{false};

// The pages: each a constexpr tree of its own, with its own ids.
constexpr auto home_ui = lv::column({.size = {200 * px, 100 * px}})[
    lv::label({.id = "title", .text = "Home"}), lv::button({.id = "go", .size = {60 * px, 30 * px}})];
constexpr auto settings_ui = lv::column({.size = {200 * px, 100 * px}})[
    lv::label({.id = "title", .text = "Settings"}), lv::slider({.id = "level"}), lv::button({.id = "back"})];
constexpr auto online_ui = lv::label({.id = "status", .text = "online"});

// How many times each .on_shown ran, and the title it read through its handle.
int home_shown = 0;
std::string_view last_title;
lv::held<home_ui> kept_home;

// A task of the home page: it waits for a click on "go", switches the page, and looks at its own
// page right after. The switch waits for the refresh, so the page is still there.
bool task_saw_its_page = false;
bool task_ran = false;
lv::task<> switch_on_click(lv::held<home_ui> page) {
  co_await page->get<"go">().next().clicked;
  current = page::settings;
  task_ran = true;
  task_saw_its_page = page.has_value() && lv_obj_is_valid(page->get<"title">().raw());
}

void start_home(lv::held<home_ui> page) {
  lv::spawn(switch_on_click(page), page->root());
  ++home_shown;
  last_title = lv_label_get_text(page->get<"title">().raw());
  kept_home = page;
}

// Two large pages, for the peak of a switch.
constexpr int many[80] = {};
constexpr auto big_a = lv::column({.size = {200 * px, 100 * px}})[lv::for_each<many>([]<int I> { return lv::label({.text = "a"}); })];
constexpr auto big_b = lv::column({.size = {200 * px, 100 * px}})[lv::for_each<many>([]<int I> { return lv::button(); })];
enum class big { a, b };
constinit lv::state<big> current_big{big::a};
constexpr auto big_window = lv::group({.size = {240 * px, 160 * px}})[
    lv::shown(current_big)[lv::is<big::a>(big_a), lv::is<big::b>(big_b)]];

// What the holder holds when an arm is shown: the old tree must be gone by then.
std::uint32_t children_when_shown = 0;
void count_children(lv::held<settings_ui> page) {
  children_when_shown = lv_obj_get_child_count(lv_obj_get_parent(page->root().raw()));
}

constexpr auto window = lv::group({.id = "window", .size = {240 * px, 160 * px}})[
    lv::shown(current)[
        lv::is<page::home>(home_ui, {.on_shown = &start_home}),
        lv::is<page::settings, page::ink>(settings_ui, {.on_shown = &count_children}),
        lv::is<page::none>()]];

constexpr auto banner = lv::group({.size = {240 * px, 40 * px}})[lv::when(online)[online_ui]];

// A shown node in a tree inside a shown node's arm.
enum class tab { a, b };
constinit lv::state<tab> current_tab{tab::a};
constexpr auto tab_a = lv::label({.text = "a"});
constexpr auto tab_b = lv::label({.text = "b"});
constexpr auto tabs = lv::group({.size = {100 * px, 40 * px}})[lv::shown(current_tab)[lv::is<tab::a>(tab_a), lv::is<tab::b>(tab_b)]];
enum class outer_page { first, second };
constinit lv::state<outer_page> outer{outer_page::first};
constexpr auto nested = lv::group({.size = {240 * px, 100 * px}})[
    lv::shown(outer)[lv::is<outer_page::first>(tabs), lv::otherwise()]];

std::size_t heap_used() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.total_size - m.free_size;
}

// The widget that holds the shown tree, and the text of the first label of what it shows.
lv_obj_t* holder_of(const lv::mounted<window>& w) { return w.get<"window">().raw(); }

const char* shown_title(const lv::mounted<window>& w) {
  lv_obj_t* h = holder_of(w);
  if (lv_obj_get_child_count(h) == 0) return "";
  return lv_label_get_text(lv_obj_get_child(lv_obj_get_child(h, 0), 0));
}

void shows_the_arm_of_the_value(lv::obj screen) {
  current = page::home;
  home_shown = 0;
  auto w = lv::mount<window>(screen);
  check(w.has_value());
  check(std::string_view{shown_title(w)} == "Home");
  check(home_shown == 1 && last_title == "Home" && kept_home.has_value());

  current = page::settings;
  check(std::string_view{shown_title(w)} == "Home");  // the switch waits for the refresh
  check(kept_home.has_value());
  lv::settle();
  check(std::string_view{shown_title(w)} == "Settings");
  check(!kept_home.has_value());  // the handle sees that home is gone

  // Two values of one arm keep the tree.
  lv_obj_t* settings_root = lv_obj_get_child(holder_of(w), 0);
  current = page::ink;
  lv::settle();
  check(lv_obj_get_child(holder_of(w), 0) == settings_root);

  // An arm without a tree shows nothing.
  current = page::none;
  lv::settle();
  check(lv_obj_get_child_count(holder_of(w)) == 0);

  current = page::home;
  lv::settle();
  check(std::string_view{shown_title(w)} == "Home" && home_shown == 2 && kept_home.has_value());
  kept_home = {};  // a handle must not outlive its tree
}

void several_changes_build_once(lv::obj screen) {
  current = page::home;
  home_shown = 0;
  auto w = lv::mount<window>(screen);
  current = page::settings;
  current = page::none;
  current = page::home;
  lv::settle();
  check(home_shown == 1);  // back where it was: nothing was rebuilt
  current = page::settings;
  current = page::none;
  lv::settle();
  check(lv_obj_get_child_count(holder_of(w)) == 0);
  kept_home = {};  // a handle must not outlive its tree
}

void the_refresh_switches(lv::obj screen) {
  current = page::home;
  auto w = lv::mount<window>(screen);
  current = page::settings;
  lv_refr_now(nullptr);
  check(std::string_view{shown_title(w)} == "Settings");
  kept_home = {};  // a handle must not outlive its tree
}

void when_shows_while_true(lv::obj screen) {
  online = false;
  auto b = lv::mount<banner>(screen);
  check(b.has_value() && lv_obj_get_child_count(b.root().raw()) == 0);
  online = true;
  lv::settle();
  check(lv_obj_get_child_count(b.root().raw()) == 1);
  online = false;
  lv::settle();
  check(lv_obj_get_child_count(b.root().raw()) == 0);
}

void nests(lv::obj screen) {
  outer = outer_page::first;
  current_tab = tab::a;
  auto n = lv::mount<nested>(screen);
  check(n.has_value());
  lv_obj_t* tabs_root = lv_obj_get_child(n.root().raw(), 0);
  check(std::string_view{lv_label_get_text(lv_obj_get_child(tabs_root, 0))} == "a");
  current_tab = tab::b;
  lv::settle();
  check(std::string_view{lv_label_get_text(lv_obj_get_child(tabs_root, 0))} == "b");
  outer = outer_page::second;
  current_tab = tab::a;  // the inner node is gone with its arm: nothing follows it now
  lv::settle();
  check(lv_obj_get_child_count(n.root().raw()) == 0);
  outer = outer_page::first;
  lv::settle();
  check(lv_obj_get_child_count(n.root().raw()) == 1);
}

// The old tree goes before the new one is built: the peak of a switch is the larger tree, not
// both. Measured as the heap after each switch, which must come back to the same number.
void switches_without_growth(lv::obj screen) {
  current = page::home;
  auto w = lv::mount<window>(screen);
  for (int i = 0; i < 100; ++i) {
    current = page::settings;
    lv::settle();
    current = page::home;
    lv::settle();
  }
  const std::size_t after_100 = heap_used();
  for (int i = 0; i < 2000; ++i) {
    current = page::settings;
    lv::settle();
    current = page::home;
    lv::settle();
  }
  check(heap_used() == after_100);
}

std::size_t heap_peak() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.max_used;
  kept_home = {};  // a handle must not outlive its tree
}

// The old tree is deleted before the new one is built, so a switch between two large pages
// never holds both: the heap's peak stays below the two together.
void deletes_before_it_builds(lv::obj screen) {
  current = page::home;
  {
    auto w = lv::mount<window>(screen);
    current = page::settings;
    lv::settle();
    check(children_when_shown == 1);
  }
  current_big = big::b;
  std::size_t with_b = 0;
  {
    auto w = lv::mount<big_window>(screen);
    with_b = heap_used();
  }
  current_big = big::a;
  const std::size_t base = heap_used();
  auto w = lv::mount<big_window>(screen);
  const std::size_t with_a = heap_used();
  const std::size_t peak_before = heap_peak();
  current_big = big::b;
  lv::settle();
  const std::size_t one_page = std::max(with_a, with_b) - base;
  const std::size_t both_pages = (with_a - base) + (with_b - base);
  const std::size_t peak = std::max(heap_peak(), peak_before) - base;
  check(heap_peak() <= std::max(peak_before, base + one_page + one_page / 4));
  check(peak < both_pages);

  kept_home = {};  // a handle must not outlive its tree
}

// A real click on a button of the home page, whose task switches to settings.
void a_click_switches(lv::obj screen) {
  lv_test_indev_create_all();
  current = page::home;
  auto w = lv::mount<window>(screen);
  lv_refr_now(nullptr);
  lv_area_t at;
  lv_obj_get_coords(lv_obj_get_child(lv_obj_get_child(holder_of(w), 0), 1), &at);
  lv_test_mouse_click_at((at.x1 + at.x2) / 2, (at.y1 + at.y2) / 2);
  check(task_ran && task_saw_its_page);
  lv_refr_now(nullptr);
  check(std::string_view{shown_title(w)} == "Settings");
  kept_home = {};  // a handle must not outlive its tree
}

// A tree with lv::shown in an lv::one_of slot.
constexpr auto other = lv::label({.text = "other"});
constinit lv::one_of<window, other> slot;

void in_a_one_of(lv::obj screen) {
  current = page::home;
  check(slot.mount<window>(screen).has_value());
  current = page::settings;
  lv::settle();
  check(slot.visit([](lv::held<window> w) { return std::string_view{shown_title(*w)}; },
                   [](lv::held<other>) { return std::string_view{}; }) == std::optional<std::string_view>{"Settings"});
  check(slot.mount<other>(screen).has_value());
  current = page::home;
  lv::settle();
  slot.unmount();
}

void ends_with_its_tree(lv::obj screen) {
  current = page::home;
  {
    auto w = lv::mount<window>(screen);
    check(kept_home.has_value());
    current = page::settings;  // queued, then the tree goes before the refresh
    kept_home = {};            // a handle must not outlive its tree's lv::mounted
  }
  lv::settle();  // the queue no longer holds the ended node
  check(lv_obj_get_child_count(screen.raw()) == 0);
}

void the_root_deleted_from_outside(lv::obj screen) {
  current = page::home;
  auto w = lv::mount<window>(screen);
  lv_obj_delete(w.root().raw());
  check(!w.has_value() && !kept_home.has_value());
  current = page::settings;
  lv::settle();
  check(lv_obj_get_child_count(screen.raw()) == 0);
  kept_home = {};  // a handle must not outlive its tree
}

void focus_moves_to_the_new_page(lv::obj screen) {
  lv_group_t* g = lv_group_create();
  lv_group_set_default(g);
  current = page::home;
  auto w = lv::mount<window>(screen);
  lv_group_focus_obj(lv_obj_get_child(lv_obj_get_child(holder_of(w), 0), 1));  // home's button
  current = page::settings;
  lv::settle();
  lv_obj_t* settings_root = lv_obj_get_child(holder_of(w), 0);
  lv_obj_t* focused = lv_group_get_focused(g);
  check(focused != nullptr && lv_obj_get_parent(focused) == settings_root);
  check(focused == lv_obj_get_child(settings_root, 1));  // the slider: the first of the page in the group
  lv_group_set_default(nullptr);
  lv_group_delete(g);
}

}  // namespace

int main() {
  lv_init();
  lv_display_t* display = lv_test_display_create(320, 240);
  lv_display_set_theme(display, nullptr);
  lv_display_add_event_cb(display, &lv::detail::drain_deferred, LV_EVENT_REFR_START, nullptr);
  auto screen = lv::screen_active();

  shows_the_arm_of_the_value(screen);
  several_changes_build_once(screen);
  the_refresh_switches(screen);
  when_shows_while_true(screen);
  nests(screen);
  switches_without_growth(screen);
  deletes_before_it_builds(screen);
  a_click_switches(screen);
  in_a_one_of(screen);
  ends_with_its_tree(screen);
  the_root_deleted_from_outside(screen);
  focus_moves_to_the_new_page(screen);

  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
  kept_home = {};  // a handle must not outlive its tree
}
