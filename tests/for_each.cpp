// lv::for_each and lv::for_each_field: the compiler writes the repeated widgets.

#include <lv/list.hpp>
#include <lv/lv.hpp>
#include <lv/repeat.hpp>

#include "check.hpp"

#include <array>
#include <cstring>

namespace {

using namespace lv::units;

enum class subpage { none, print, scan, copy, usb };
constinit lv::state<subpage> active{subpage::none};

struct icons {
  static constexpr char print[] = "P";
  static constexpr char scan[] = "S";
  static constexpr char copy[] = "C";
  static constexpr char usb[] = "U";
};

//! [tile]
constexpr auto tile = []<subpage Page> {
  return lv::button({.on_clicked = [] { active = Page; }})[
      lv::label({.text = lv::like<Page, icons>}), lv::label({.text = lv::name_of<Page>})];
};
constexpr auto home = lv::row[lv::for_each<subpage, lv::but<subpage::none>>(tile)];
//! [tile]

constexpr auto framed = lv::column[lv::label({.text = "header"}),
                                   lv::for_each<subpage, lv::but<subpage::none, subpage::usb>>(tile),
                                   lv::label({.text = "footer"})];

struct job {
  int ignored;
  [[=lv::caption{"queue.owner"}]] const char* owner;
  [[=lv::caption{"queue.pages"}]] lv::counter pages;
};
constexpr job sample{0, "info@example.com", lv::counter{6}};

constexpr auto lines = lv::column[lv::for_each_field<job>([]<lv::field F> {
  return lv::row[lv::label({.text = F.caption}), lv::label({.text = lv::name_of<subpage::scan>})];
})];

struct tube {
  const char* name;
  int level;
};
constexpr tube tubes[] = {{"yellow", 90}, {"cyan", 50}, {"black", 10}};
constexpr auto gauges = lv::row[lv::for_each<tubes>([]<const auto& T> {
  return lv::label({.text = T.name});
})];

// A structural record is a template argument of its own, in a std::array too.
struct level {
  int percent;
};
constexpr std::array<level, 2> levels{{{90}, {50}}};
constexpr auto bars = lv::row[lv::for_each<levels>([]<level L> {
  return lv::bar({.range = {0, 100}, .value = {L.percent, false}});
})];

// The lines of a row of lv::each: bindings to the row's fields.
struct line_id {
  int value;
};
struct entry {
  line_id id;
  [[=lv::caption{"queue.owner"}]] const char* owner;
  [[=lv::caption{"queue.pages"}]] lv::counter pages;
};
constinit lv::list<entry, 2> entries;
constexpr auto entry_lines = lv::column[lv::each(entries, &entry::id)[
    lv::column[lv::for_each_field<entry>([]<lv::field F> {
      return lv::row[lv::label({.text = F.caption}), lv::label({.text = F.text()})];
    })]]];

static_assert(std::string_view{lv::name_of<subpage::print>} == "print");
static_assert(std::string_view{lv::like<subpage::copy, icons>} == "C");

std::uint32_t count(lv::obj o) { return lv_obj_get_child_count(o.raw()); }
lv_obj_t* kid(lv::obj o, std::int32_t i) { return lv_obj_get_child(o.raw(), i); }

void tiles_are_children_of_the_row(lv::obj screen) {
  auto m = lv::mount<home>(screen);
  check(m.has_value());
  check(count(m->root()) == 4);
  check(std::strcmp(lv_label_get_text(lv_obj_get_child(kid(m->root(), 0), 0)), "P") == 0);
  check(std::strcmp(lv_label_get_text(lv_obj_get_child(kid(m->root(), 3), 1)), "usb") == 0);
}

void a_click_sets_the_tiles_enumerator(lv::obj screen) {
  auto m = lv::mount<home>(screen);
  const subpage order[] = {subpage::print, subpage::scan, subpage::copy, subpage::usb};
  for (std::int32_t i = 0; i < 4; ++i) {
    lv_obj_send_event(kid(m->root(), i), LV_EVENT_CLICKED, nullptr);
    check(active.get() == order[i]);
  }
}

void siblings_keep_their_places(lv::obj screen) {
  auto m = lv::mount<framed>(screen);
  check(count(m->root()) == 5);
  check(std::strcmp(lv_label_get_text(kid(m->root(), 0)), "header") == 0);
  check(std::strcmp(lv_label_get_text(kid(m->root(), 4)), "footer") == 0);
  check(std::strcmp(lv_label_get_text(lv_obj_get_child(kid(m->root(), 3), 1)), "copy") == 0);
}

void captioned_members_only(lv::obj screen) {
  auto m = lv::mount<lines>(screen);
  check(count(m->root()) == 2);
  check(std::strcmp(lv_label_get_text(lv_obj_get_child(kid(m->root(), 0), 0)), "queue.owner") == 0);
  check(std::strcmp(lv_label_get_text(lv_obj_get_child(kid(m->root(), 1), 0)), "queue.pages") == 0);
}

void records_give_values(lv::obj screen) {
  auto m = lv::mount<gauges>(screen);
  check(count(m->root()) == 3);
  check(std::strcmp(lv_label_get_text(kid(m->root(), 0)), "yellow") == 0);
  check(std::strcmp(lv_label_get_text(kid(m->root(), 2)), "black") == 0);
}

void structural_records_and_bindings_in_rows(lv::obj screen) {
  auto b = lv::mount<bars>(screen);
  check(count(b->root()) == 2 && lv_bar_get_value(kid(b->root(), 1)) == 50);
  auto m = lv::mount<entry_lines>(screen);
  check(entries.push_back({{1}, "info@example.com", lv::counter{6}}).has_value());
  lv_obj_t* details = kid(m->root(), 0);
  check(lv_obj_get_child_count(details) == 2);
  check(std::strcmp(lv_label_get_text(lv_obj_get_child(lv_obj_get_child(details, 0), 1)), "info@example.com") == 0);
  check(std::strcmp(lv_label_get_text(lv_obj_get_child(lv_obj_get_child(details, 1), 1)), "6") == 0);
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);
  auto screen = lv::screen_active();
  tiles_are_children_of_the_row(screen);
  a_click_sets_the_tiles_enumerator(screen);
  siblings_keep_their_places(screen);
  captioned_members_only(screen);
  records_give_values(screen);
  structural_records_and_bindings_in_rows(screen);
  return failures;
}
