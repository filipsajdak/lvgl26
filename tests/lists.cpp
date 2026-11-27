// lv::list and lv::each: a change touches the widgets of its own row only.

#include <lv/list.hpp>
#include <lv/lv.hpp>

#include "check.hpp"
#include "full_heap.hpp"

#include <array>
#include <concepts>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace {

using namespace lv::units;

//! [model]
struct track_id {
  int value;
};

struct track {
  track_id id;
  const char* title;
  lv::counter plays;
};

constinit lv::list<track, 20> tracks;  // room for 20 rows
//! [model]

//! [row]
constexpr auto track_row =
    lv::row({.size = {100 * pct, lv::size_content}})[
        lv::label({.id = "title", .text = lv::bind(&track::title)}),
        lv::label({.id = "plays", .text = lv::bind(&track::plays, "{} plays")})];

constexpr auto playlist = lv::column({.size = {320 * px, 240 * px}})[
    lv::each(tracks, &track::id)[track_row]];
//! [row]

std::size_t free_bytes() {
  lv_mem_monitor_t m;
  lv_mem_monitor(&m);
  return m.free_size;
}

const char* const titles[] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j",
                              "k", "l", "m", "n", "o", "p", "q", "r", "s", "t"};

track numbered(std::size_t i) { return {{static_cast<int>(i + 1)}, titles[i], lv::counter{0}}; }

void fill(std::size_t n) {
  tracks.clear();
  for (std::size_t i = 0; i < n; ++i) check(tracks.push_back(numbered(i)).has_value());
}

lv_obj_t* row_at(lv::obj rows, std::size_t i) { return lv_obj_get_child(rows.raw(), static_cast<std::int32_t>(i)); }

const char* text_of(lv_obj_t* row, std::int32_t child) { return lv_label_get_text(lv_obj_get_child(row, child)); }

bool shows(lv_obj_t* row, const char* title) { return std::strcmp(text_of(row, 0), title) == 0; }

// Writes "x" into a row's labels behind the list's back: a row that its view sets again
// shows its values once more, a row that it leaves alone still shows "x".
void taint(lv_obj_t* row) {
  lv_label_set_text(lv_obj_get_child(row, 0), "x");
  lv_label_set_text(lv_obj_get_child(row, 1), "x");
}

// The widgets of every row now shown, each watched for its deletion, and tainted.
struct watched_rows {
  std::vector<lv_obj_t*> roots;
  std::vector<lv::weak<lv::obj>> alive;

  explicit watched_rows(lv::obj rows) {
    for (std::uint32_t i = 0; i < lv_obj_get_child_count(rows.raw()); ++i) {
      lv_obj_t* r = lv_obj_get_child(rows.raw(), static_cast<std::int32_t>(i));
      roots.push_back(r);
      alive.emplace_back(lv::obj{r});
      taint(r);
    }
  }

  std::size_t deleted() const {
    std::size_t n = 0;
    for (const auto& w : alive) n += w.expired() ? std::size_t{1} : std::size_t{0};
    return n;
  }
};

void builds_a_row_for_each_track(lv::obj screen) {
  fill(3);
  auto root = lv::mount<playlist>(screen);
  check(root.has_value());
  if (!root) return;
  const lv::obj rows = root->root();
  check(lv_obj_get_child_count(rows.raw()) == 3);
  check(shows(row_at(rows, 0), "a") && shows(row_at(rows, 2), "c"));
  check(std::strcmp(text_of(row_at(rows, 1), 1), "0 plays") == 0);
  // The text of a row is not copied: the label shows the row's own text.
  check(text_of(row_at(rows, 0), 0) == titles[0]);
}

// Animates a property that the layout leaves alone.
void fade(lv_obj_t* row) {
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, row);
  lv_anim_set_values(&a, 0, 255);
  lv_anim_set_duration(&a, 1000);
  lv_anim_set_exec_cb(&a, [](void* o, std::int32_t v) {
    lv_obj_set_style_opa(static_cast<lv_obj_t*>(o), static_cast<lv_opa_t>(v), LV_PART_MAIN);  // C API
  });
  lv_anim_start(&a);
}

lv_opa_t opa_of(lv_obj_t* row) { return lv_obj_get_style_opa(row, LV_PART_MAIN); }

// Insert at the front of 19 rows: no other row is built again or set again, and an
// animation on one of them goes on.
void insert_in_front_keeps_the_others(lv::obj screen) {
  fill(19);
  auto root = lv::mount<playlist>(screen);
  if (!root) return check(false);
  const lv::obj rows = root->root();
  watched_rows before{rows};
  fade(before.roots[5]);
  lv_test_wait(200);
  const lv_opa_t faded = opa_of(before.roots[5]);

  check(tracks.insert(0, {{100}, "new", lv::counter{1}}).has_value());
  check(lv_obj_get_child_count(rows.raw()) == 20);
  check(before.deleted() == 0);
  for (std::size_t i = 0; i < before.roots.size(); ++i) check(row_at(rows, i + 1) == before.roots[i]);
  for (std::size_t i = 1; i < 20; ++i) check(shows(row_at(rows, i), "x"));
  check(shows(row_at(rows, 0), "new"));

  check(lv_anim_get(before.roots[5], nullptr) != nullptr);
  lv_test_wait(200);
  check(opa_of(before.roots[5]) > faded);
  lv_anim_delete(before.roots[5], nullptr);
}

// Each change touches its own row: a change of one field, an erase, a move.
void a_change_of_one_field_sets_one_row(lv::obj screen) {
  fill(6);
  auto root = lv::mount<playlist>(screen);
  if (!root) return check(false);
  const lv::obj rows = root->root();
  watched_rows w{rows};
  tracks.update(2, [](track& t) { t.plays = lv::counter{7}; });
  check(std::strcmp(text_of(row_at(rows, 2), 1), "7 plays") == 0 && shows(row_at(rows, 2), "c"));
  for (std::size_t i : {std::size_t{0}, std::size_t{1}, std::size_t{3}, std::size_t{4}, std::size_t{5}})
    check(shows(row_at(rows, i), "x"));
  check(w.deleted() == 0);
}

void an_erase_deletes_one_row(lv::obj screen) {
  fill(6);
  auto root = lv::mount<playlist>(screen);
  if (!root) return check(false);
  const lv::obj rows = root->root();
  watched_rows w{rows};
  tracks.erase(1);  // b
  check(w.deleted() == 1 && w.alive[1].expired() && lv_obj_get_child_count(rows.raw()) == 5);
  check(row_at(rows, 1) == w.roots[2]);
}

void a_move_moves_one_row(lv::obj screen) {
  fill(5);
  auto root = lv::mount<playlist>(screen);
  if (!root) return check(false);
  const lv::obj rows = root->root();
  watched_rows w{rows};
  tracks.move(0, 3);  // a goes after d
  check(w.deleted() == 0 && tracks[3].title == titles[0]);
  check(row_at(rows, 3) == w.roots[0] && row_at(rows, 0) == w.roots[1] && row_at(rows, 2) == w.roots[3]);
  tracks.move(4, 0);  // e to the front
  check(row_at(rows, 0) == w.roots[4] && row_at(rows, 4) == w.roots[0]);
  for (std::size_t i = 0; i < 5; ++i) check(shows(row_at(rows, i), "x"));
}

// A full list is a result: the row is not added, and nothing changes.
void a_full_list_says_so(lv::obj screen) {
  fill(20);
  auto root = lv::mount<playlist>(screen);
  if (!root) return check(false);
  const auto r = tracks.insert(3, {{99}, "late", lv::counter{0}});
  check(!r.has_value() && r.error() == lv::list_error::full);
  check(tracks.size() == 20 && lv_obj_get_child_count(root->root().raw()) == 20);
  check(!tracks.push_back({{98}, "later", lv::counter{0}}).has_value());
  const track more[21] = {};
  const auto all = tracks.assign(more);
  check(!all.has_value() && all.error() == lv::list_error::full && tracks.size() == 20);
}

// A list runs from left to right in a row, and rows differ in size.
constinit lv::list<track, 4> shelf;
constexpr auto shelf_ui = lv::row({.size = {320 * px, 100 * px}})[
    lv::each(shelf, &track::id)[lv::label({.text = lv::bind(&track::title)})]];

void runs_across_with_rows_of_any_size(lv::obj screen) {
  check(shelf.push_back({{1}, "one", {}}).has_value());
  check(shelf.push_back({{2}, "two\nlines", {}}).has_value());
  check(shelf.push_back({{3}, "three", {}}).has_value());
  auto root = lv::mount<shelf_ui>(screen);
  if (!root) return check(false);
  lv_obj_update_layout(root->root().raw());
  const lv::obj rows = root->root();
  check(lv_obj_get_x(row_at(rows, 0)) < lv_obj_get_x(row_at(rows, 1)));
  check(lv_obj_get_x(row_at(rows, 1)) < lv_obj_get_x(row_at(rows, 2)));
  check(lv_obj_get_height(row_at(rows, 1)) > lv_obj_get_height(row_at(rows, 0)));
  check(shelf.insert(0, {{4}, "zero", {}}).has_value());
  lv_obj_update_layout(rows.raw());
  check(lv_obj_get_x(row_at(rows, 0)) < lv_obj_get_x(row_at(rows, 1)));
  shelf.clear();
}

// assign() keeps the widgets of each key: in reverse order, every row moves and none is
// built again. A list that went by place would show "e" in the widgets of "a".
void assign_keeps_rows_by_key(lv::obj screen) {
  fill(5);
  auto root = lv::mount<playlist>(screen);
  if (!root) return check(false);
  const lv::obj rows = root->root();
  watched_rows w{rows};
  const track reversed[] = {numbered(4), numbered(3), numbered(2), numbered(1), numbered(0)};
  check(tracks.assign(reversed).has_value());
  check(w.deleted() == 0);
  for (std::size_t i = 0; i < 5; ++i) {
    check(row_at(rows, i) == w.roots[4 - i]);
    check(shows(row_at(rows, i), titles[4 - i]));  // set again with its own values
  }
  // A key that goes deletes its row; a new key builds one.
  const track other[] = {numbered(2), {{50}, "fifty", {}}, numbered(4)};
  check(tracks.assign(other).has_value());
  check(w.deleted() == 3 && row_at(rows, 0) == w.roots[2] && row_at(rows, 2) == w.roots[4]);
  check(shows(row_at(rows, 1), "fifty"));
}

// Keys should be unique. A key given to every row is a mistake, and still safe: a reset that
// brings unique keys shows each row once.
void a_key_given_twice_stays_safe(lv::obj screen) {
  tracks.clear();
  auto root = lv::mount<playlist>(screen);
  if (!root) return check(false);
  for (std::size_t i = 0; i < 20; ++i) check(tracks.push_back({{1}, titles[i], {}}).has_value());
  track unique[20];
  for (std::size_t i = 0; i < 20; ++i) unique[i] = numbered(i);
  check(tracks.assign(unique).has_value());
  check(lv_obj_get_child_count(root->root().raw()) == 20);
  for (std::size_t i = 0; i < 20; ++i) check(shows(row_at(root->root(), i), titles[i]));
}

// A view that had no memory for a reset shows nothing, then the list again at the next
// change. An erase is not that change: the row is still in the list while the view hears of it.
void a_view_comes_back_after_running_out_of_memory(lv::obj screen) {
  fill(1);
  auto root = lv::mount<playlist>(screen);
  if (!root) return check(false);
  track ten[10];
  for (std::size_t i = 0; i < 10; ++i) ten[i] = numbered(i + 5);
  {
    full_heap no_room{64};
    const auto r = tracks.assign(ten);
    check(!r.has_value() && r.error() == lv::list_error::out_of_memory);
  }
  check(tracks.size() == 10 && lv_obj_get_child_count(root->root().raw()) == 0);
  tracks.erase(0);
  check(tracks.size() == 9 && lv_obj_get_child_count(root->root().raw()) == 0);
  check(tracks.push_back(numbered(0)).has_value());  // an insert is a complete change
  check(lv_obj_get_child_count(root->root().raw()) == 10 && shows(row_at(root->root(), 0), titles[6]));
}

// A null text field shows nothing, and not the label's old text.
void a_null_text_shows_nothing(lv::obj screen) {
  fill(1);
  auto root = lv::mount<playlist>(screen);
  if (!root) return check(false);
  check(tracks.push_back({{50}, nullptr, {}}).has_value());
  check(shows(row_at(root->root(), 1), ""));
  tracks.update(0, [](track& t) { t.title = nullptr; });
  check(shows(row_at(root->root(), 0), ""));
}

// A row whose key changes is a new row.
void a_new_key_is_a_new_row(lv::obj screen) {
  fill(3);
  auto root = lv::mount<playlist>(screen);
  if (!root) return check(false);
  watched_rows w{root->root()};
  tracks.update(1, [](track& t) { t.id = track_id{77}; });
  check(w.deleted() == 1 && w.alive[1].expired());
  check(shows(row_at(root->root(), 1), "b") && lv_obj_get_child_count(root->root().raw()) == 3);
}

// Without memory for a row's widgets, insert() changes nothing and says so.
void out_of_memory_changes_nothing(lv::obj screen) {
  fill(2);
  auto root = lv::mount<playlist>(screen);
  if (!root) return check(false);
  {
    full_heap no_room{64};
    const auto r = tracks.insert(1, {{9}, "none", {}});
    check(!r.has_value() && r.error() == lv::list_error::out_of_memory);
  }
  check(tracks.size() == 2 && lv_obj_get_child_count(root->root().raw()) == 2);
  check(shows(row_at(root->root(), 1), "b"));
  check(tracks.insert(1, {{9}, "room", {}}).has_value() && shows(row_at(root->root(), 1), "room"));
}

// Two views of one list follow it both.
void two_views_follow_one_list(lv::obj screen) {
  fill(2);
  std::optional<lv::mounted<playlist>> left{std::in_place, screen};
  auto right = lv::mount<playlist>(screen);
  if (!*left || !right) return check(false);
  check(tracks.push_back(numbered(2)).has_value());
  check(lv_obj_get_child_count(left->root().raw()) == 3 && lv_obj_get_child_count(right->root().raw()) == 3);
  left.reset();  // one view goes; the other still follows
  tracks.erase(0);
  check(lv_obj_get_child_count(right->root().raw()) == 2 && shows(row_at(right->root(), 0), "b"));
}

// A reset reaches every view, also when one of them had no memory for its rows: each view
// shows the list, or nothing until the next complete change, and never rows the list lost.
void a_reset_reaches_every_view(lv::obj screen) {
  fill(1);
  auto left = lv::mount<playlist>(screen);
  auto right = lv::mount<playlist>(screen);
  if (!left || !right) return check(false);
  track ten[10];
  for (std::size_t i = 0; i < 10; ++i) ten[i] = numbered(i + 5);
  {
    full_heap no_room{64};
    check(!tracks.assign(ten).has_value());
  }
  check(lv_obj_get_child_count(left->root().raw()) == 0 && lv_obj_get_child_count(right->root().raw()) == 0);
  check(tracks.push_back(numbered(0)).has_value());
  check(lv_obj_get_child_count(left->root().raw()) == 11 && lv_obj_get_child_count(right->root().raw()) == 11);
  check(shows(row_at(left->root(), 0), titles[5]) && shows(row_at(right->root(), 0), titles[5]));
}

// The heap is back where it was once the view and the rows are gone.
void the_heap_returns(lv::obj screen) {
  tracks.clear();
  const std::size_t before = free_bytes();
  {
    auto root = lv::mount<playlist>(screen);
    for (std::size_t i = 0; i < 20; ++i) check(tracks.push_back(numbered(i)).has_value());
    tracks.erase(4);
    tracks.move(0, 10);
  }
  tracks.clear();
  check(free_bytes() == before);
  {
    auto root = lv::mount<playlist>(screen);
    fill(20);
    tracks.clear();  // the rows go first, the view later
  }
  check(free_bytes() == before);
}

// The list goes first: the views delete their rows and forget it.
void the_list_can_go_first(lv::obj screen) {
  fill(3);
  auto root = lv::mount<playlist>(screen);
  if (!root) return check(false);
  std::destroy_at(&tracks);
  check(lv_obj_get_child_count(root->root().raw()) == 0);
  std::construct_at(&tracks);
  check(tracks.push_back(numbered(0)).has_value());
  check(lv_obj_get_child_count(root->root().raw()) == 0);  // that view no longer follows
}

// A row erases itself from its own click handler.
constinit lv::list<track, 4> chores;
void done(lv::event e) {
  lv_obj_t* row = lv_obj_get_parent(e.target<lv::obj>().raw());
  chores.erase(static_cast<std::size_t>(lv_obj_get_index(row)));
}
constexpr auto chores_ui = lv::column[lv::each(chores, &track::id)[
    lv::row({.size = {100 * pct, lv::size_content}})[
        lv::label({.text = lv::bind(&track::title)}),
        lv::button({.on_clicked = &done})[lv::label({.text = "Done"})]]]];

void a_row_erases_itself(lv::obj screen) {
  check(chores.push_back({{1}, "wash", {}}).has_value());
  check(chores.push_back({{2}, "dry", {}}).has_value());
  auto root = lv::mount<chores_ui>(screen);
  if (!root) return check(false);
  lv_obj_t* button = lv_obj_get_child(row_at(root->root(), 0), 1);
  lv_obj_send_event(button, LV_EVENT_CLICKED, nullptr);
  check(chores.size() == 1 && lv_obj_get_child_count(root->root().raw()) == 1);
  check(shows(row_at(root->root(), 0), "dry"));
  chores.clear();
}

// The ids of a row, found from the row's root; and work that belongs to a row.
int started = 0;
bool task_ended = false;
struct on_end {
  bool* flag;
  ~on_end() { *flag = true; }
};
lv::task<> blink(lv::obj row) {
  on_end end{&task_ended};
  for (;;) co_await row.next().clicked;
}
void start(lv::obj row, const track& t) {
  started += t.id.value;
  lv::spawn(blink(row), row);
}
constinit lv::list<track, 4> queue;
constexpr auto queue_ui = lv::column[lv::each(queue, &track::id, {.on_row = &start})[track_row]];

// The call forms build the node of the subscript forms: with options and a row, with a row alone, with
// a row written in the call, and with options alone.
template <class T>
using node_of = std::remove_const_t<T>;
static_assert(std::same_as<node_of<decltype(lv::each(queue, &track::id, {.on_row = &start}, track_row))>,
                           node_of<decltype(lv::each(queue, &track::id, {.on_row = &start})[track_row])>>);
static_assert(std::same_as<node_of<decltype(lv::each(queue, &track::id, track_row))>,
                           node_of<decltype(lv::each(queue, &track::id)[track_row])>>);
static_assert(std::same_as<node_of<decltype(lv::each(queue, &track::id, {}, track_row))>,
                           node_of<decltype(lv::each(queue, &track::id)[track_row])>>);
static_assert(std::same_as<node_of<decltype(lv::each(queue, &track::id, lv::label({.text = "x"})))>,
                           node_of<decltype(lv::each(queue, &track::id)[lv::label({.text = "x"})])>>);
static_assert(requires { lv::each(queue, &track::id, {.on_row = &start}); });
// The key is a named value or an enum; a plain integer or a number is not one, without the setting.
enum class shelf_key { top };
static_assert(lv::detail::row_key<track_id> && lv::detail::row_key<shelf_key> && !lv::detail::row_key<int> &&
              !lv::detail::row_key<double>);

void a_row_has_ids_and_tasks(lv::obj screen) {
  started = 0;
  check(queue.push_back({{5}, "five", lv::counter{2}}).has_value());
  auto root = lv::mount<queue_ui>(screen);
  if (!root) return check(false);
  check(started == 5);
  const lv::row_view<track_row> row{lv::obj{row_at(root->root(), 0)}};
  static_assert(std::is_same_v<decltype(row.get<"plays">()), lv::view<"label">>);
  check(std::strcmp(row.get<"plays">().get<"text">(), "2 plays") == 0);
  check(row.root() == lv::obj{row_at(root->root(), 0)});
  check(!task_ended);
  queue.erase(0);
  check(task_ended);
}

// "No tracks" is a label that the number of rows hides.
constinit lv::computed has_tracks{[](lv::counter n) { return n.value > 0; }, tracks.count()};
constexpr auto with_empty = lv::column[
    lv::label({.hidden = lv::bind(has_tracks), .text = "No tracks"}),
    lv::column({.size = {100 * pct, lv::size_content}})[lv::each(tracks, &track::id)[track_row]]];

void the_number_of_rows_is_a_state(lv::obj screen) {
  tracks.clear();
  auto root = lv::mount<with_empty>(screen);
  if (!root) return check(false);
  lv_obj_t* empty = lv_obj_get_child(root->root().raw(), 0);
  check(!lv_obj_is_hidden(empty));
  check(tracks.push_back(numbered(0)).has_value());
  check(lv_obj_is_hidden(empty) && tracks.count().get().value == 1);
  tracks.clear();
  check(!lv_obj_is_hidden(empty));
  has_tracks.reset();
}

// Rows go after the widgets before the lv::each, and before those after it.
constexpr auto framed = lv::column[
    lv::label({.text = "Head"}), lv::each(tracks, &track::id)[track_row], lv::label({.text = "Foot"})];

void rows_sit_between_their_siblings(lv::obj screen) {
  fill(2);
  auto root = lv::mount<framed>(screen);
  if (!root) return check(false);
  const lv::obj c = root->root();
  check(tracks.insert(0, numbered(5)).has_value());
  check(tracks.push_back(numbered(6)).has_value());
  check(lv_obj_get_child_count(c.raw()) == 6);
  check(std::strcmp(lv_label_get_text(row_at(c, 0)), "Head") == 0);
  check(shows(row_at(c, 1), "f") && shows(row_at(c, 4), "g"));
  check(std::strcmp(lv_label_get_text(row_at(c, 5)), "Foot") == 0);
}

// The heap of a row: the widgets of its template, and nothing of lv::each's. Mounting a tree
// for each row and setting its texts, as a program without lv::each does, costs more: LVGL
// copies each text, and each tree needs an owner.
constexpr auto plain_row = lv::row({.size = {100 * pct, lv::size_content}})[
    lv::label({.id = "title", .text = ""}), lv::label({.id = "plays", .text = ""})];
constexpr auto bare_column = lv::column();
constexpr auto empty_view = lv::column[lv::each(tracks, &track::id)[track_row]];

std::size_t heap_of_view(lv::obj screen) {
  tracks.clear();
  const std::size_t start = free_bytes();
  auto bare = lv::mount<bare_column>(screen);
  const std::size_t column_only = start - free_bytes();
  auto view = lv::mount<empty_view>(screen);
  return start - free_bytes() - 2 * column_only;
}

std::size_t heap_per_row_mounted(lv::obj screen, std::size_t n) {
  auto column = lv::mount<bare_column>(screen);
  std::vector<lv::owned<lv::obj>> rows;
  const std::size_t before = free_bytes();
  for (std::size_t i = 0; i < n; ++i) {
    auto r = lv::mount<plain_row>(column->root());
    r->get<"title">().set({.text = titles[i]});
    r->get<"plays">().set({.text = "0 plays"});
    rows.emplace_back(r->detach());
  }
  return (before - free_bytes()) / n;
}

std::size_t heap_per_row_listed(lv::obj screen, std::size_t n) {
  tracks.clear();
  auto root = lv::mount<playlist>(screen);
  const std::size_t before = free_bytes();
  for (std::size_t i = 0; i < n; ++i) check(tracks.push_back(numbered(i)).has_value());
  const std::size_t per_row = (before - free_bytes()) / n;
  tracks.clear();
  return per_row;
}

void measures_the_heap_per_row(lv::obj screen) {
  heap_per_row_mounted(screen, 4);  // once first: the first mount makes what later ones share
  heap_per_row_listed(screen, 4);
  const std::size_t mounted = heap_per_row_mounted(screen, 20);
  const std::size_t listed = heap_per_row_listed(screen, 20);
  std::printf("heap per row of two labels: mounted %zu, lv::each %zu bytes\n", mounted, listed);
  check(listed < mounted);
  // What lv::each keeps for a view, apart from the rows: its block on the column.
  std::printf("a view of a list of 20, without rows: %zu bytes\n", heap_of_view(screen));
}

// A number field through a format: the text of the format around the digits, with a sign.
static_assert(lv::detail::format_room("%d plays") == 20);  // the format, and room for an int32

void formats_numbers() {
  char out[lv::detail::format_room("%d plays")];
  volatile std::int32_t twelve = 12, lowest = -2147483647 - 1, zero = 0;
  lv::detail::format_number(out, "%d plays", twelve);
  check(std::strcmp(out, "12 plays") == 0);
  lv::detail::format_number(out, "%d plays", lowest);
  check(std::strcmp(out, "-2147483648 plays") == 0);
  lv::detail::format_number(out, "100%% of %d", zero);
  check(std::strcmp(out, "100% of 0") == 0);
  lv::detail::format_number(out, "%d dB", twelve);  // a space before a d is text
  check(std::strcmp(out, "12 dB") == 0);
  lv::detail::format_number(out, "%d played", twelve);  // and so is a letter
  check(std::strcmp(out, "12 played") == 0);
  // A bool property takes any number that is not 0 as true, also one that C code wrote.
  volatile std::int32_t minus_one = -1;
  check(lv::detail::from_number(minus_one, lv::detail::as<lv::strict<bool>>{}).raw());
  check(!lv::detail::from_number(zero, lv::detail::as<lv::strict<bool>>{}).raw());
}

// Reads one row a time, as a program walks a list.
void reads_like_a_range() {
  fill(3);
  int sum = 0;
  for (const track& t : tracks) sum += t.id.value;
  check(sum == 6 && tracks.size() == 3 && !tracks.empty() && tracks.capacity() == 20);
}

// A list changed while its views follow a change, from .on_row, breaks a contract.
constinit lv::list<track, 4> busy;
void grow(lv::obj, const track&) { static_cast<void>(busy.push_back({{9}, "more", {}})); }
constexpr auto busy_ui = lv::column[lv::each(busy, &track::id, {.on_row = &grow})[track_row]];

// A value that breaks a contract at run time ends the program with a contract violation.
// Each case is run as a test of its own, which expects that.
struct violation {
  const char* name;
  int (*run)(lv::obj screen);
};

volatile std::size_t far = 25;

constexpr violation violations[] = {
    {"index", [](lv::obj) { return tracks[far].id.value; }},
    {"erase", [](lv::obj) { return tracks.erase(far), 0; }},
    {"move", [](lv::obj) { return tracks.move(0, far), 0; }},
    {"move_from_end", [](lv::obj) { return tracks.move(tracks.size(), 0), 0; }},
    {"move_from_far", [](lv::obj) { return tracks.move(far, 0), 0; }},
    {"move_to_end", [](lv::obj) { return tracks.move(0, tracks.size()), 0; }},
    {"insert", [](lv::obj) { return tracks.insert(far, numbered(4)).has_value() ? 1 : 0; }},
    {"set", [](lv::obj screen) { return screen.add<"label">().set({.text = lv::bind(&track::title)}) ? 1 : 0; }},
    {"telling",
     [](lv::obj screen) {
       auto root = lv::mount<busy_ui>(screen);
       return busy.push_back({{1}, "first", {}}).has_value() ? 1 : 0;
     }},
};

int violates(const char* which, lv::obj screen) {
  fill(3);
  for (const violation& v : violations)
    if (std::strcmp(which, v.name) == 0) return v.run(screen);
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  lv_init();
  lv_test_display_create(320, 240);
  auto screen = lv::screen_active();
  if (argc == 2) {
    std::printf("no contract violation: %d\n", violates(argv[1], screen));
    return 0;
  }

  builds_a_row_for_each_track(screen);
  insert_in_front_keeps_the_others(screen);
  a_change_of_one_field_sets_one_row(screen);
  an_erase_deletes_one_row(screen);
  a_move_moves_one_row(screen);
  a_full_list_says_so(screen);
  runs_across_with_rows_of_any_size(screen);
  assign_keeps_rows_by_key(screen);
  a_new_key_is_a_new_row(screen);
  a_key_given_twice_stays_safe(screen);
  a_null_text_shows_nothing(screen);
  a_view_comes_back_after_running_out_of_memory(screen);
  out_of_memory_changes_nothing(screen);
  two_views_follow_one_list(screen);
  a_reset_reaches_every_view(screen);
  the_heap_returns(screen);
  the_list_can_go_first(screen);
  a_row_erases_itself(screen);
  a_row_has_ids_and_tasks(screen);
  the_number_of_rows_is_a_state(screen);
  rows_sit_between_their_siblings(screen);
  reads_like_a_range();
  formats_numbers();
  measures_the_heap_per_row(screen);

  tracks.clear();
  queue.clear();
  tracks.count();
  lv_deinit();
  if (failures == 0) std::printf("ok\n");
  return failures == 0 ? 0 : 1;
}
