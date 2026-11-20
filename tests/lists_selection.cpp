// The selected row of an lv::each: a state of the key's type, set by a click or the focus,
// shown as the checked state of the row.

#include <lv/list.hpp>
#include <lv/lv.hpp>

#include "check.hpp"

#include <cstring>

namespace {

using namespace lv::units;

constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

//! [selection]
struct song_id {
  int value;
};

struct song {
  song_id id;
  const char* title;
};

constinit lv::list<song, 8> songs;
constinit lv::state<song_id> current{song_id{0}};  // no song has the id 0

// A row is checked while it is the selected one. state_trickle hands the state to its
// children, so that the label styles itself for it too.
constexpr auto song_row = lv::button({.size = {100 * pct, lv::size_content}, .state_trickle = true,
                                      .style = {.bg_color = tok.color.card},
                                      .style_checked = {.bg_color = tok.color.primary}})[
    lv::label({.style = {.text_color = tok.color.text}, .style_checked = {.text_color = tok.color.surface},
               .text = lv::bind(&song::title)})];

constexpr auto picker = lv::column({.size = {320 * px, 240 * px}})[
    lv::each(songs, &song::id, {.selected = lv::bind(current)})[song_row]];
//! [selection]

std::size_t free_bytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.free_size;
}

void fill() {
  songs.clear();
  for (int i = 1; i <= 4; ++i) {
    static const char* const names[] = {"one", "two", "three", "four"};
    check(songs.push_back({{i}, names[i - 1]}).has_value());
  }
}

lv_obj_t* row_at(lv::obj rows, std::int32_t i) { return lv_obj_get_child(rows.raw(), i); }

bool is_checked(lv_obj_t* o) { return lv_obj_has_state(o, LV_STATE_CHECKED); }

// How many rows are checked.
int checked_rows(lv::obj rows) {
  int n = 0;
  for (std::uint32_t i = 0; i < lv_obj_get_child_count(rows.raw()); ++i)
    n += is_checked(row_at(rows, static_cast<std::int32_t>(i))) ? 1 : 0;
  return n;
}

void a_click_selects(lv::obj screen) {
  fill();
  current = song_id{0};
  auto root = lv::mount<picker>(screen);
  if (!root) return check(false);
  const lv::obj rows = root->root();
  check(checked_rows(rows) == 0);
  lv_obj_send_event(row_at(rows, 2), LV_EVENT_CLICKED, nullptr);
  check(current.get().value == 3);
  check(is_checked(row_at(rows, 2)) && checked_rows(rows) == 1);
  // The state reaches the label, which styles itself for it.
  lv_obj_t* label = lv_obj_get_child(row_at(rows, 2), 0);
  check(is_checked(label));
  check(lv_color_eq(lv_obj_get_style_text_color(label, LV_PART_MAIN), *tok.color.surface));
  lv_obj_send_event(row_at(rows, 0), LV_EVENT_CLICKED, nullptr);
  check(current.get().value == 1 && is_checked(row_at(rows, 0)) && !is_checked(row_at(rows, 2)));
  check(!is_checked(lv_obj_get_child(row_at(rows, 2), 0)));
}

// Only a click and the focus select: a press, or any other event of a row, does not.
void other_events_do_not_select(lv::obj screen) {
  fill();
  current = song_id{1};
  auto root = lv::mount<picker>(screen);
  if (!root) return check(false);
  const lv::obj rows = root->root();
  lv_obj_send_event(row_at(rows, 2), LV_EVENT_PRESSED, nullptr);
  lv_obj_send_event(row_at(rows, 2), LV_EVENT_VALUE_CHANGED, nullptr);
  check(current.get().value == 1 && is_checked(row_at(rows, 0)) && checked_rows(rows) == 1);
}

void code_selects(lv::obj screen) {
  fill();
  auto root = lv::mount<picker>(screen);
  if (!root) return check(false);
  const lv::obj rows = root->root();
  current = song_id{4};
  check(is_checked(row_at(rows, 3)) && checked_rows(rows) == 1);
  current = song_id{42};  // no such row: none is checked
  check(checked_rows(rows) == 0);
}

// The selection follows the key, not the place: rows move, the same widgets stay checked.
void the_selection_follows_the_row(lv::obj screen) {
  fill();
  auto root = lv::mount<picker>(screen);
  if (!root) return check(false);
  const lv::obj rows = root->root();
  current = song_id{2};
  lv_obj_t* two = row_at(rows, 1);
  check(songs.insert(0, {{9}, "nine"}).has_value());
  songs.move(2, 4);
  check(row_at(rows, 4) == two && is_checked(two) && checked_rows(rows) == 1);
  songs.erase(4);  // the selected row goes; its key stays selected, and no row has it
  check(checked_rows(rows) == 0 && current.get().value == 2);
  check(songs.push_back({{2}, "two again"}).has_value());  // a row with that key comes back
  check(is_checked(row_at(rows, 4)) && checked_rows(rows) == 1);
}

// Keys move the focus, and the focus selects: a button row joins the default group.
void the_focus_selects(lv::obj screen) {
  fill();
  current = song_id{0};
  lv_group_t* keys = lv_group_create();
  lv_group_set_default(keys);
  {
    auto root = lv::mount<picker>(screen);
    if (!root) return check(false);
    const lv::obj rows = root->root();
    // The first row took the focus when it joined the group, before it could select.
    check(lv_group_get_focused(keys) == row_at(rows, 0) && current.get().value == 0);
    lv_group_focus_next(keys);
    check(current.get().value == 2 && is_checked(row_at(rows, 1)) && checked_rows(rows) == 1);
    lv_group_focus_next(keys);
    check(current.get().value == 3);
    lv_group_focus_prev(keys);
    lv_group_focus_prev(keys);
    check(current.get().value == 1 && is_checked(row_at(rows, 0)) && checked_rows(rows) == 1);
  }
  lv_group_set_default(nullptr);
  lv_group_delete(keys);
}

// A row that goes leaves the selection as it was, also when it had the focus: LVGL then
// focuses another row, and that does not select it.
void a_row_that_goes_keeps_the_selection(lv::obj screen) {
  fill();
  lv_group_t* keys = lv_group_create();
  lv_group_set_default(keys);
  {
    auto root = lv::mount<picker>(screen);
    if (!root) return check(false);
    const lv::obj rows = root->root();
    lv_group_focus_obj(row_at(rows, 1));
    check(current.get().value == 2);
    songs.erase(1);
    check(current.get().value == 2 && checked_rows(rows) == 0);
    // Select another row by the focus, then delete them all: the selection stays on it.
    lv_obj_t* other = lv_group_get_focused(keys) == row_at(rows, 2) ? row_at(rows, 0) : row_at(rows, 2);
    lv_group_focus_obj(other);
    const int picked = current.get().value;
    check(picked != 2);
    songs.clear();
    check(current.get().value == picked);
  }
  lv_group_set_default(nullptr);
  lv_group_delete(keys);
}

// A view and its rows give the heap back. The selection costs one event handler for each row.
void the_heap_returns(lv::obj screen) {
  songs.clear();
  current.raw();  // the state's subject lives as long as the state
  const std::size_t before = free_bytes();
  {
    auto root = lv::mount<picker>(screen);
    fill();
    current = song_id{2};
    lv_obj_send_event(row_at(root->root(), 3), LV_EVENT_CLICKED, nullptr);
    songs.erase(1);
  }
  songs.clear();
  check(free_bytes() == before);
}

// A selected row that is deleted by deleting the whole view: the focus moves through the rows
// as LVGL deletes them, and lv::each has already let go.
void the_view_goes_while_a_row_has_the_focus(lv::obj screen) {
  fill();
  lv_group_t* keys = lv_group_create();
  lv_group_set_default(keys);
  {
    auto root = lv::mount<picker>(screen);
    if (!root) return check(false);
    lv_group_focus_obj(row_at(root->root(), 1));
  }
  check(current.get().value == 2);
  lv_group_set_default(nullptr);
  lv_group_delete(keys);
  songs.clear();
}

}  // namespace

int main() {
  lv_init();
  lv_test_display_create(320, 240);
  auto screen = lv::screen_active();

  a_click_selects(screen);
  other_events_do_not_select(screen);
  code_selects(screen);
  the_selection_follows_the_row(screen);
  the_focus_selects(screen);
  a_row_that_goes_keeps_the_selection(screen);
  the_heap_returns(screen);
  the_view_goes_while_a_row_has_the_focus(screen);

  songs.clear();
  current.reset();
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
