#pragma once

// Lists: rows that the program adds, removes, moves and changes, each shown by a row
// template, a constant tree like any other.
//
//   struct track_id { int value; };
//   struct track {
//     track_id id;
//     const char* title;
//     lv::counter plays;
//   };
//
//   constinit lv::list<track, 20> tracks;            // room for 20 rows, chosen at compile time
//
//   constexpr auto ui = lv::column[
//       lv::each(tracks, &track::id)[                // a row for each track, known by its id
//           lv::row[lv::label({.text = lv::bind(&track::title)}),
//                   lv::label({.text = lv::bind(&track::plays, "{} plays")})]]];
//
//   auto root = lv::mount<ui>(display.screen());
//   if (!tracks.push_back({{1}, "Intro"})) { ... }   // builds one row; full or out of memory
//   tracks.insert(0, {{2}, "Outro"});                // builds one more, in front
//   tracks.update(1, [](track& t) { ++t.plays; });   // sets that row's bound properties again
//   tracks.move(0, 1);                               // moves the widgets of one row
//   tracks.erase(0);                                 // deletes the widgets of one row
//
// A change touches the widgets of its own row only. The widgets of the other rows stay, with
// their running animations, their tasks and the focus.
//
// The rows
//
//   An lv::list holds its rows in place: a row keeps its address from insert to erase, so a
//   label shows a text field of it without a copy. move() and insert() shift small numbers,
//   never the rows. Read a row with tracks[i]; change it with update(), which tells the views.
//   A change that does not fit is a result, not an error: insert() and push_back() return
//   list_error::full when the list is full, and change nothing.
//
// The views
//
//   lv::each(tracks, &track::id)[row] builds one row template for each row, in the widget that
//   holds the lv::each: lv::column for a list from top to bottom, lv::row for one from left to
//   right. A widget holds at most one lv::each; it may hold other children before and after it.
//   The key names a row. assign() replaces all rows, and a row whose key stays keeps its
//   widgets. A row whose key changes in update() is built anew. Keys are unique.
//
//   lv::bind(&track::title) in the row template follows a field of the row. It takes what a
//   binding to a state takes: a text (const char*, std::string or a char array), a
//   quantity, a named value, an enum, a bool, a color or a pointer. lv::bind(&track::plays,
//   "{} plays") formats a number. A state bound in a row template works as anywhere else.
//
//   .selected = lv::bind(current) makes a state of the key's type follow the selected row: a
//   click or the focus selects a row, and the selected row is `checked`, so the row template
//   styles it with .style_checked. .on_row = &start is called with each row that is built.
//
// What happens to a row when it goes
//
//   Its widgets are deleted. LVGL removes their event handlers, observers and animations with
//   them, and a task started with lv::spawn(work, row) ends. Its ids need no memory: a row is
//   reached through lv::row_view<template>{row}.get<"id">().
//
// Like all of LVGL, a list must only be changed from the UI thread, and not while one of its
// views follows a change (from an .on_row function).

#include <lv/computed.hpp>
#include <lv/state.hpp>
#include <lv/tree.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <new>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace lv {

// Why a change to an lv::list did not happen, or is not shown.
enum class list_error {
  full,           // the list holds as many rows as it can; nothing changed
  out_of_memory,  // LVGL had no memory for a row's widgets; see the function
};

template <class T, std::size_t N>
class list;

namespace detail {

// ---- the model and its views -----------------------------------------------

enum class change_kind : std::uint8_t {
  inserted,  // a row was added at `at`
  erased,    // the row at `at` is about to go
  moved,     // the row at `at` moved to `to`
  changed,   // the row at `at` has new values
  reset,     // all rows may be new: assign() and clear()
  gone,      // the list itself goes
};

struct list_change {
  change_kind kind;
  std::size_t at;
  std::size_t to;
};

// Something that shows a list: lv::each's block, kept on the widget that holds the rows.
struct list_watcher {
  list_watcher* next;
  // Follows one change. False only when it could not show an inserted row, and then it
  // shows the list as it was before.
  bool (*follow)(list_watcher* self, const list_change& change);
};

// The watchers of one list, linked through the watchers themselves: no memory of its own.
class watcher_chain {
  list_watcher* first_ = nullptr;
  bool telling_ = false;

 public:
  void link(list_watcher* w) {
    w->next = first_;
    first_ = w;
  }

  void unlink(const list_watcher* w) {
    for (list_watcher** p = &first_; *p != nullptr; p = &(*p)->next) {
      if (*p == w) {
        *p = w->next;
        return;
      }
    }
  }

  bool telling() const { return telling_; }

  // Tells the watchers, in order, until one cannot follow. Returns that one, or null.
  list_watcher* tell(const list_change& c) {
    telling_ = true;
    list_watcher* failed = nullptr;
    for (list_watcher* w = first_; w != nullptr && failed == nullptr;) {
      list_watcher* next = w->next;
      if (!w->follow(w, c)) failed = w;
      w = next;
    }
    telling_ = false;
    return failed;
  }

  // Tells every watcher, whatever each answers: a reset cannot be undone, so each view must
  // follow it or show nothing. True if every one could follow.
  bool tell_all(const list_change& c) {
    telling_ = true;
    bool all = true;
    for (list_watcher* w = first_; w != nullptr;) {
      list_watcher* next = w->next;
      all = w->follow(w, c) && all;
      w = next;
    }
    telling_ = false;
    return all;
  }

  // Tells the watchers before `stop`: they undo what `stop` could not follow.
  void tell_until(const list_change& c, const list_watcher* stop) {
    telling_ = true;
    for (list_watcher* w = first_; w != stop && w != nullptr;) {
      list_watcher* next = w->next;
      w->follow(w, c);
      w = next;
    }
    telling_ = false;
  }

  // The list goes: every watcher forgets it.
  void end() {
    for (list_watcher* w = first_; w != nullptr;) {
      list_watcher* next = w->next;
      w->follow(w, {change_kind::gone, 0, 0});
      w = next;
    }
    first_ = nullptr;
  }
};

// Reaches the watchers of a list for lv::each, which holds a pointer to a const list.
struct list_access {
  template <class L>
  static watcher_chain& watchers(const L& l) {
    return l.watchers_;
  }
};

// What lv::each shows: something with rows of one type, a capacity, and watchers.
template <class S>
concept row_source = requires(const S& s, std::size_t i) {
  typename S::value_type;
  { s.size() } -> std::same_as<std::size_t>;
  { s[i] } -> std::same_as<const typename S::value_type&>;
  { S::capacity() } -> std::same_as<std::size_t>;
  { list_access::watchers(s) } -> std::same_as<watcher_chain&>;
};

// Moves entry `from` of `a` to place `to`; the entries between shift by one.
template <class E>
constexpr void shift(E* a, std::size_t from, std::size_t to) {
  const E moved = a[from];
  for (; from < to; ++from) a[from] = a[from + 1];
  for (; from > to; --from) a[from] = a[from - 1];
  a[to] = moved;
}

// The smallest unsigned type that numbers N cells.
template <std::size_t N>
using cell_index = std::conditional_t<(N <= 0xff), std::uint8_t, std::uint16_t>;

}  // namespace detail

// A list of up to N rows of type T: the model of lv::each. See the top of this file.
// It is not copied or moved: views hold its address. Make it a constant-initialized global
// (`constinit`), as a state is.
template <class T, std::size_t N>
class list {
  static_assert(N > 0, "lv: a list holds at least one row: lv::list<track, 8>.");
  static_assert(N <= 0xffff, "lv: a list holds at most 65535 rows.");

  using index = detail::cell_index<N>;
  // Named through T: a state names lv::cfg, which a project may set after this header.
  using count_state = state<counter, typename detail::settings<T>::tag>;

  // A cell holds a row, or nothing. `none` is what a constant-initialized list holds.
  union cell {
    char none;
    T value;
    constexpr cell() : none() {}
    constexpr ~cell() {}
  };

  cell cells_[N];
  index order_[N]{};  // order_[0..size) are the cells of the rows, in order; the rest are free
  std::size_t size_ = 0;
  count_state count_{counter{0}};
  mutable detail::watcher_chain watchers_;
  friend detail::list_access;

 public:
  using value_type = T;

  constexpr list() {
    for (std::size_t i = 0; i < N; ++i) order_[i] = static_cast<index>(i);
  }
  list(const list&) = delete;
  list& operator=(const list&) = delete;

  // Deletes the rows of every view while LVGL runs, then the rows themselves.
  ~list() {
    if (lv_is_initialized()) clear();
    watchers_.end();
    for (std::size_t i = 0; i < size_; ++i) std::destroy_at(&cells_[order_[i]].value);
  }

  static constexpr std::size_t capacity() { return N; }
  std::size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }

  // The number of rows as a state, to bind to or compute from:
  //   lv::computed has_rows{[](lv::counter n) { return n.value > 0; }, tracks.count()};
  constexpr const count_state& count() const { return count_; }

  const T& operator[](std::size_t i) const
    pre(i < size())
  {
    return cells_[order_[i]].value;
  }

  class iterator {
    const list* l_ = nullptr;
    std::size_t i_ = 0;

   public:
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    iterator() = default;
    iterator(const list* l, std::size_t i) : l_(l), i_(i) {}
    const T& operator*() const { return (*l_)[i_]; }
    iterator& operator++() {
      ++i_;
      return *this;
    }
    iterator operator++(int) {
      iterator was = *this;
      ++i_;
      return was;
    }
    friend bool operator==(const iterator&, const iterator&) = default;
  };

  iterator begin() const { return {this, 0}; }
  iterator end() const { return {this, size_}; }

  // Adds a row before row `at`; at == size() adds it at the end. Every view builds its widgets.
  // list_error::full: the list is full. list_error::out_of_memory: LVGL had no memory for the
  // widgets of the row. Either way the list is as it was.
  [[nodiscard]] std::expected<void, list_error> insert(std::size_t at, T value)
    pre(at <= size())
    pre(!telling())
  {
    if (size_ == N) return std::unexpected(list_error::full);
    const index c = order_[size_];
    std::construct_at(&cells_[c].value, std::move(value));
    detail::shift(order_, size_, at);
    ++size_;
    if (const auto* failed = watchers_.tell({detail::change_kind::inserted, at, 0})) {
      watchers_.tell_until({detail::change_kind::erased, at, 0}, failed);
      remove(at);
      return std::unexpected(list_error::out_of_memory);
    }
    count_ = counter{static_cast<std::int32_t>(size_)};
    return {};
  }

  [[nodiscard]] std::expected<void, list_error> push_back(T value)
    pre(!telling())
  {
    return insert(size_, std::move(value));
  }

  // Removes row `at`. Every view deletes its widgets.
  void erase(std::size_t at)
    pre(at < size())
    pre(!telling())
  {
    watchers_.tell({detail::change_kind::erased, at, 0});
    remove(at);
    count_ = counter{static_cast<std::int32_t>(size_)};
  }

  // Moves row `from` to place `to`; the rows between shift by one. Every view moves its widgets.
  void move(std::size_t from, std::size_t to)
    pre(from < size() && to < size())
    pre(!telling())
  {
    if (from == to) return;
    detail::shift(order_, from, to);
    watchers_.tell({detail::change_kind::moved, from, to});
  }

  // Changes row `at` with `f(row)`, then every view sets the bound properties of that row again.
  // A row whose key `f` changes is a new row: its widgets are built anew.
  template <class F>
    requires std::invocable<F&, T&>
  void update(std::size_t at, F f)
    pre(at < size())
    pre(!telling())
  {
    f(cells_[order_[at]].value);
    watchers_.tell({detail::change_kind::changed, at, 0});
  }

  // Replaces all rows with copies of `rows`. A view keeps the widgets of each row whose key it
  // still finds, and moves them into place. list_error::full: `rows` has more than N rows, and
  // nothing changed. list_error::out_of_memory: a view could not build a row; it shows no
  // rows until the next change.
  template <class R>
    requires requires(const R& r) {
      { r.size() } -> std::convertible_to<std::size_t>;
      { *r.begin() } -> std::convertible_to<const T&>;
      r.end();
    }
  [[nodiscard]] std::expected<void, list_error> assign(const R& rows)
    pre(!telling())
  {
    return assign_from(rows, rows.size());
  }

  template <std::size_t M>
  [[nodiscard]] std::expected<void, list_error> assign(const T (&rows)[M])
    pre(!telling())
  {
    return assign_from(rows, M);
  }

  // Removes all rows. Every view deletes its widgets first.
  void clear()
    pre(!telling())
  {
    const std::size_t had = size_;
    size_ = 0;
    watchers_.tell_all({detail::change_kind::reset, 0, 0});
    size_ = had;
    destroy_rows();
    count_ = counter{0};
  }

  // True while the views follow a change: the list must not change then.
  bool telling() const { return watchers_.telling(); }

 private:
  template <class R>
  std::expected<void, list_error> assign_from(const R& rows, std::size_t count) {
    if (count > N) return std::unexpected(list_error::full);
    destroy_rows();
    for (const T& r : rows) std::construct_at(&cells_[order_[size_++]].value, r);
    const bool shown = watchers_.tell_all({detail::change_kind::reset, 0, 0});
    count_ = counter{static_cast<std::int32_t>(size_)};
    if (!shown) return std::unexpected(list_error::out_of_memory);
    return {};
  }

  // Takes row `at` out and destroys it; its cell becomes free.
  void remove(std::size_t at) {
    std::destroy_at(&cells_[order_[at]].value);
    detail::shift(order_, at, size_ - 1);
    --size_;
  }

  void destroy_rows() {
    for (std::size_t i = 0; i < size_; ++i) std::destroy_at(&cells_[order_[i]].value);
    size_ = 0;
  }
};

// ---- keys -------------------------------------------------------------------

namespace detail {

// Whether a pointer constant is null. Compared by reflection: under a sanitizer GCC 16.2
// does not compare the address of a function or of a static object with null in a
// constant expression.
template <class P>
consteval bool is_null(P p) {
  return std::meta::reflect_constant(p) == std::meta::reflect_constant(P{});
}

// What names one row. Like the value of a state: a named value or an enum. A plain integer
// only where lv::config::raw_integer_state allows one.
template <class K>
concept row_key =
    strong_value<K> || std::is_enum_v<K> || (integer<K> && cfg<typename settings<K>::tag>.raw_integer_state);

// A key as the whole number a state's subject holds for it.
template <class K>
constexpr std::int32_t key_number(const K& k) {
  if constexpr (strong_value<K>)
    return static_cast<std::int32_t>(k.[:strong_member<K>():]);
  else
    return static_cast<std::int32_t>(k);
}

// ---- bindings to the fields of a row -----------------------------------------

// The value of a field, in the form the property takes it from.
struct row_value {
  std::int32_t number;  // a quantity, a named value, an enum, a bool or an integer
  const void* pointer;  // a text or a pointer
  lv_color_t color;
};

inline row_value field_value(const char* text) { return {.number = 0, .pointer = text, .color = {}}; }
inline row_value field_value(const std::string& text) { return field_value(text.c_str()); }
template <std::size_t M>
row_value field_value(const char (&text)[M]) {
  return field_value(&text[0]);
}
inline row_value field_value(lv_color_t c) { return {.number = 0, .pointer = nullptr, .color = c}; }
template <class P>
  requires std::is_pointer_v<P>
row_value field_value(P p) {
  return {.number = 0, .pointer = p, .color = {}};
}
template <integer_quantity Q>
row_value field_value(const Q& q) {
  return {.number = static_cast<std::int32_t>(q.numerical_value_in(Q::unit)), .pointer = nullptr, .color = {}};
}
template <strong_value S>
row_value field_value(const S& s) {
  return {.number = key_number(s), .pointer = nullptr, .color = {}};
}
template <plain_state_value V>
row_value field_value(V v) {
  return {.number = static_cast<std::int32_t>(v), .pointer = nullptr, .color = {}};
}

inline lv_color_t from_row(const row_value& v, as<lv_color_t>) { return v.color; }
template <class P>
  requires std::is_pointer_v<P>
P from_row(const row_value& v, as<P>) {
  return static_cast<P>(const_cast<void*>(v.pointer));
}
template <class V>
V from_row(const row_value& v, as<V>) {
  return from_number(v.number, as<V>{});
}

// A field that a text property shows as it is.
template <class F>
concept text_field = std::same_as<F, const char*> || std::same_as<F, std::string> ||
                     (std::is_array_v<F> && std::same_as<std::remove_extent_t<F>, char>);

// What a binding to a field of type F hands the property: a text, or F itself.
template <class F>
struct field_type {
  using type = F;
};
template <text_field F>
struct field_type<F> {
  using type = const char*;
};

// One bound field: a constant that the binding points to, made by lv::bind(&T::field).
struct row_field {
  row_value (*read)(const row_field* self, const void* row);
};

template <class C, class F>
struct row_field_of : row_field {
  F C::* member;

  static row_value read_member(const row_field* self, const void* row) {
    const auto* me = static_cast<const row_field_of*>(self);
    return field_value(static_cast<const C*>(row)->*(me->member));
  }
};

template <class F>
consteval void check_field() {
  static_assert(!std::same_as<F, std::string_view>,
                "lv: a std::string_view field has no ending zero, which LVGL needs. Make the field "
                "a const char*, a std::string or a char array.");
  static_assert(text_field<F> || std::same_as<F, std::string_view> || state_value<F>,
                "lv: a field of a row follows like a state: a text (const char*, std::string or a "
                "char array), a quantity, a named value, an enum, a bool, an lv_color_t or a "
                "pointer.");
  static_assert(!integer<F> || cfg<typename settings<F>::tag>.raw_integer_state,
                "lv: a field of a plain integer does not say what the number is. If it is a "
                "measure, give it a unit with mp-units. If it counts things, use lv::counter. For "
                "anything else, name it: `struct pages { int value; };`. To allow plain integers, "
                "set lv::config::raw_integer_state.");
}

// The room a formatted number needs: the format, and an int32 with its sign.
consteval std::size_t format_room(const char* format) {
  return std::char_traits<char>::length(format) + 12;
}

// Writes the decimal digits of n at `out`. Returns the end.
inline char* write_decimal(char* out, std::int32_t n) {
  std::int64_t v = n;
  if (v < 0) {
    *out++ = '-';
    v = -v;
  }
  char digits[12];
  std::size_t count = 0;
  do digits[count++] = static_cast<char>('0' + v % 10);
  while ((v /= 10) > 0);
  while (count > 0) *out++ = digits[--count];
  return out;
}

// Writes `format` with its %d replaced by n, and each %% as %. A text binding's format has
// exactly one %d (see lv::text_format).
inline void format_number(char* out, const char* format, std::int32_t n) {
  for (const char* f = format; *f != '\0'; ++f) {
    if (f[0] == '%' && f[1] == 'd') {
      out = write_decimal(out, n);
      ++f;
    } else {
      *out++ = *f;
      if (f[0] == '%' && f[1] == '%') ++f;
    }
  }
  *out = '\0';
}

}  // namespace detail

// Binds a property of a row template to a field of the row: .text = lv::bind(&track::title).
// Only in the row template of lv::each.
template <class C, class F>
  requires(!std::is_function_v<F>)
consteval detail::row_binding<typename detail::field_type<F>::type> bind(F C::* field) {
  detail::check_field<F>();
  const detail::row_field* f =
      std::define_static_object(detail::row_field_of<C, F>{{&detail::row_field_of<C, F>::read_member}, field});
  return {f, &detail::row_field_marker};
}

// Binds a text property of a row template to a number field through a format:
// .text = lv::bind(&track::plays, "{} plays").
template <class C, class F>
  requires(!std::is_function_v<F>)
consteval detail::row_formatted_binding<F> bind(F C::* field, text_format format) {
  detail::check_field<F>();
  const detail::row_field* f =
      std::define_static_object(detail::row_field_of<C, F>{{&detail::row_field_of<C, F>::read_member}, field});
  return {f, &detail::row_field_marker, format.printf};
}

template <class C, class F>
void bind(F C::*) = delete(
    "lv: lv::bind(&T::member) follows a data member of a row. A member function has no value "
    "to show: keep the value in a field of the row.");

namespace detail {

// ---- applying the fields of a row ----------------------------------------------

// The child of `w` built from kid number K of its node: LVGL adds a widget's own children
// (the label of a text area) before the tree's.
template <std::size_t K, std::size_t Kids>
lv_obj_t* tree_child_of(lv_obj_t* w) {
  const auto own = static_cast<std::int32_t>(lv_obj_get_child_count(w)) - static_cast<std::int32_t>(Kids);
  return lv_obj_get_child(w, own + static_cast<std::int32_t>(K));
}

// Sets one property of `w`, member I of the node at Path, from the field of `row` it follows.
template <const auto& Tree, std::size_t I, std::size_t... Path>
void apply_field(lv_obj_t* w, const void* row) {
  constexpr const auto& N = descend<Path...>(Tree);
  constexpr auto W = std::remove_cvref_t<decltype(N)>::kind;
  constexpr auto a = N.p.[:members_of_kept<W>[I]:];
  using A = std::remove_cvref_t<decltype(a)>;
  static_assert(std::tuple_size_v<typename A::types> == 1,
                "lv: this property takes more than one value and cannot follow a field of a row.");
  using V = std::tuple_element_t<0, typename A::types>;
  constexpr auto setter = targets<W>[I];

  const auto* field = static_cast<const row_field*>(a.bound);
  const row_value got = field->read(field, row);
  if constexpr (std::same_as<V, const char*> && !is_null(a.bound_format)) {
    char text[format_room(a.bound_format)];
    format_number(text, a.bound_format, got.number);
    set_property<W, setter, false>(w, static_cast<const char*>(text));
  } else if constexpr (std::same_as<V, const char*>) {
    // The text lives in the row, which outlives its widgets and tells them when it changes.
    // LVGL reads a null text as "show the old one again": a null field shows nothing.
    const auto* text = static_cast<const char*>(got.pointer);
    set_property<W, setter, true>(w, text != nullptr ? text : "");
  } else {
    [:setter:](w, lower(from_row(got, as<V>{})));
  }
}

// Whether member I of a kept property set follows a field of a row.
template <const auto& Tree, std::size_t I, std::size_t... Path>
consteval bool follows_field() {
  constexpr const auto& N = descend<Path...>(Tree);
  constexpr auto W = std::remove_cvref_t<decltype(N)>::kind;
  constexpr auto m = members_of_kept<W>[I];
  if constexpr (is_maybe<std::remove_cvref_t<decltype(N.p.[:m:])>>)
    return false;
  else
    return N.p.[:m:].follows == follows_row_field;
}

// Whether the node at Path, or one below it, follows a field of a row.
template <const auto& Tree, std::size_t... Path>
consteval bool has_fields() {
  using node_type = std::remove_cvref_t<decltype(descend<Path...>(Tree))>;
  if constexpr (is_structural<node_type>) {
    return false;
  } else {
    bool any = false;
    template for (constexpr auto i : set_members<Tree, Path...>) any = any || follows_field<Tree, i, Path...>();
    template for (constexpr auto k : indices<kid_count<node_type>>) any = any || has_fields<Tree, Path..., k>();
    return any;
  }
}

// Sets every property of the row whose root is `w` that follows a field of `row`. The node at
// Path is the root of the row template. The same step builds a row and changes one.
template <const auto& Tree, std::size_t... Path>
void apply_row(lv_obj_t* w, const void* row) {
  using node_type = std::remove_cvref_t<decltype(descend<Path...>(Tree))>;
  template for (constexpr auto i : set_members<Tree, Path...>) {
    if constexpr (follows_field<Tree, i, Path...>()) apply_field<Tree, i, Path...>(w, row);
  }
  template for (constexpr auto k : indices<kid_count<node_type>>) {
    if constexpr (has_fields<Tree, Path..., k>())
      apply_row<Tree, Path..., k>(tree_child_of<k, kid_count<node_type>>(w), row);
  }
}

// ---- the ids of a row ------------------------------------------------------------

// The ids of a row template, checked to be unique.
template <class Node>
consteval std::vector<id_entry> ids_in(const Node& n) {
  std::vector<id_entry> out;
  collect(out, n);
  for (std::size_t i = 0; i < out.size(); ++i)
    for (std::size_t j = i + 1; j < out.size(); ++j)
      if (out[i].name == out[j].name)
        fail("lv: the id '" + str(out[i].name) + "' is used twice in one row template. Ids must be unique.");
  return out;
}

// The kid numbers that lead from `n` to the node with the id `name`. Empty if it is `n`.
template <fixed W, class... Kids>
consteval bool find_id(const node<W, Kids...>& n, std::string_view name, std::vector<std::size_t>& path) {
  if (n.id() == name) return true;
  bool found = false;
  template for (constexpr auto k : indices<sizeof...(Kids)>) {
    if (!found) {
      path.push_back(k);
      found = find_id(kid<k>(n.kids), name, path);
      if (!found) path.pop_back();
    }
  }
  return found;
}
template <class S>
  requires is_structural<S>
consteval bool find_id(const S&, std::string_view, std::vector<std::size_t>&) {
  return false;
}

template <const auto& Template>
consteval std::meta::info row_id_view(std::string_view name) {
  const auto ids = ids_in(Template);
  std::string valid;
  for (const auto& id : ids) {
    if (id.name == name) return id.view;
    valid += (valid.empty() ? "" : ", ") + str(id.name);
  }
  fail("lv: the row template has no id '" + str(name) + "'. Its ids: " +
       (valid.empty() ? str("none") : valid) + ".");
}

template <const auto& Template, fixed Name>
inline constexpr auto row_id_path = [] {
  std::vector<std::size_t> path;
  find_id(Template, Name.sv(), path);
  return std::define_static_array(path);
}();

// The widget with the id Name in a row built from `Template`, whose root is `w`: the kid
// numbers of its path, followed one level at a time. Taken... are the levels done so far.
template <const auto& Template, fixed Name, std::size_t... Taken>
lv_obj_t* child_on_path(lv_obj_t* w) {
  constexpr auto path = row_id_path<Template, Name>;
  if constexpr (sizeof...(Taken) == path.size()) {
    return w;
  } else {
    using node_type = std::remove_cvref_t<decltype(descend<Taken...>(Template))>;
    constexpr std::size_t k = path[sizeof...(Taken)];
    return child_on_path<Template, Name, Taken..., k>(tree_child_of<k, kid_count<node_type>>(w));
  }
}

}  // namespace detail

// The widgets of one row, by the ids of its row template: lv::row_view<card>{row}.get<"title">().
// `card` is the constexpr row template given to lv::each. A row needs no memory for its ids:
// get<>() follows the template from the row's root.
template <const auto& Template>
  requires tree_node<decltype(Template)>
class row_view {
  lv_obj_t* root_;

 public:
  constexpr explicit row_view(obj root) : root_(root.raw()) {}

  obj root() const { return obj{root_}; }

  // A typed view of the widget with this id in the row.
  template <detail::fixed Name>
  auto get() const {
    // A check in the body, not pre(): GCC 16.2 crashes on pre() with a deduced return type.
    contract_assert(root_ != nullptr);
    using view_type = typename[:detail::row_id_view<Template>(Name.sv()):];
    return view_type{detail::child_on_path<Template, Name>(root_)};
  }
};

namespace detail {

// ---- lv::each ------------------------------------------------------------------

// The selected row's key, as a binding to a state of the key's type: lv::bind(current).
template <class K>
struct selection {
  binding<K> state{};

  constexpr selection() = default;
  template <class U>
  constexpr selection(binding<U> b) : state{b.source, b.subject} {
    static_assert(std::same_as<U, K>,
                  "lv: .selected follows the key of the selected row. Bind a state of the key's "
                  "type: `constinit lv::state<track_id> current{};`, then .selected = lv::bind(current).");
  }
};

// What lv::each takes after the key, all optional: {.on_row = &start, .selected = lv::bind(current)}.
template <class T, class K>
struct each_options {
  void (*on_row)(obj row, const T& value) = nullptr;  // called with each row that is built
  selection<K> selected{};                            // the state that follows the selected row
};

template <class S, class T, class K, class... Row>
struct each_node;

}  // namespace detail

template <class S, class T, class K, class... Row>
inline constexpr bool detail::is_structural<detail::each_node<S, T, K, Row...>> = true;

namespace detail {

class each_view;
template <const auto& Tree, std::size_t... Path>
struct each_parts;

// Whether the root of a row template sets .checked: the state that .selected gives.
template <class Node>
consteval bool sets_checked(const Node& n) {
  if constexpr (requires { n.p.checked; })
    return n.p.checked.set;
  else
    return false;
}

// What a view needs of the list it shows and of its row template: one table for each lv::each
// of a program, made at compile time. The logic is in each_view, which is no template, so
// a program has its code once, however many lists it shows.
struct each_kind {
  std::size_t capacity;
  std::size_t (*size)(const void* rows);
  const void* (*at)(const void* rows, std::size_t i);  // row i of the list
  std::int32_t (*key)(const void* row);
  watcher_chain& (*watchers)(const void* rows);
  // The widgets of `row`, after the other children of `outlet`, with its fields set; null if
  // LVGL had no memory, and then nothing of them is left.
  lv_obj_t* (*build)(lv_obj_t* outlet, const void* row);
  void (*apply)(lv_obj_t* widgets, const void* row);  // sets the fields of a row again
  void (*on_row)(lv_obj_t* widgets, const void* row);  // .on_row, or null
  lv_subject_t* (*selection)();                        // the state of .selected, or null
};

// The block that lv::each keeps for one view: it follows the list and keeps the key of each
// row it shows, so that a reset finds the widgets of a key again. It lives on LVGL's heap,
// as long as the widget that holds the rows: start() makes it, that widget's deletion frees
// it. Row i is that widget's child first_ + i. The keys follow the block in the same allocation.
class each_view : list_watcher {
  const each_kind* kind_;
  const void* rows_;  // null once the list is gone
  lv_obj_t* outlet_;
  std::size_t first_;
  std::size_t shown_ = 0;
  bool broken_ = false;           // LVGL had no memory to follow a reset: no rows are shown
  lv_obj_t* selected_ = nullptr;  // the row in the checked state
  bool quiet_ = false;            // the view deletes a row: the focus that LVGL moves selects nothing
  std::int32_t* keys_;            // capacity + 1: a row with a new key is built before the old goes

  each_view(const each_kind& kind, const void* rows, lv_obj_t* outlet, std::int32_t* keys)
      : list_watcher{nullptr, &each_view::follow},
        kind_(&kind),
        rows_(rows),
        outlet_(outlet),
        first_(static_cast<std::size_t>(lv_obj_get_child_count(outlet))),
        keys_(keys) {}

 public:
  // Builds the rows in `outlet`, the widget that holds the lv::each, and follows the list from
  // now on. False if LVGL had no memory; deleting `outlet` then cleans up.
  static bool start(lv_obj_t* outlet, const each_kind& kind, const void* rows) {
    const std::size_t keys_size = (kind.capacity + 1) * sizeof(std::int32_t);
    void* memory = lv_malloc(sizeof(each_view) + keys_size);
    if (memory == nullptr) return false;
    auto* keys = reinterpret_cast<std::int32_t*>(static_cast<unsigned char*>(memory) + sizeof(each_view));
    auto* self = ::new (memory) each_view(kind, rows, outlet, keys);
    if (lv_obj_add_event_cb(outlet, &each_view::on_outlet_delete, LV_EVENT_DELETE, self) == nullptr) {
      self->~each_view();
      lv_free(memory);
      return false;
    }
    kind.watchers(rows).link(self);
    return self->follow_selection() && self->fill();
  }

 private:
  std::size_t size() const { return kind_->size(rows_); }
  const void* value_at(std::size_t i) const { return kind_->at(rows_, i); }
  lv_obj_t* row(std::size_t i) const { return lv_obj_get_child(outlet_, static_cast<std::int32_t>(first_ + i)); }

  static void on_outlet_delete(lv_event_t* e) {
    auto* self = static_cast<each_view*>(lv_event_get_user_data(e));
    self->close();
    self->~each_view();
    lv_free(self);
  }

  // Lets go of the list, and of the rows, which LVGL deletes after this.
  void close() {
    if (rows_ != nullptr) kind_->watchers(rows_).unlink(this);
    if (kind_->selection == nullptr) return;
    for (std::size_t i = 0; i < shown_; ++i) lv_obj_remove_event_cb_with_user_data(row(i), &each_view::on_row_event, this);
  }

  static bool follow(list_watcher* w, const list_change& c) {
    auto* self = static_cast<each_view*>(w);
    if (c.kind == change_kind::gone) {
      self->rows_ = nullptr;
      return true;
    }
    if (self->broken_) {
      // An erased row is still in the list while the views hear of it: fill after the change.
      if (c.kind != change_kind::erased) self->broken_ = !self->fill();
      return true;
    }
    return self->follow_change(c);
  }

  bool follow_change(const list_change& c) {
    switch (c.kind) {
      case change_kind::inserted:
        return build_row(c.at);
      case change_kind::erased:
        erase_row(c.at);
        return true;
      case change_kind::moved:
        move_row(c.at, c.to);
        return true;
      case change_kind::changed:
        change_row(c.at);
        return true;
      default:
        return reset();
    }
  }

  // Builds the rows of the whole list. On failure no rows are shown.
  bool fill() {
    for (std::size_t i = 0; i < size(); ++i) {
      if (!build_row(i)) {
        clear_rows();
        return false;
      }
    }
    return true;
  }

  void clear_rows() {
    while (shown_ > 0) erase_row(shown_ - 1);
  }

  // Builds the widgets of list row `at`, at its place. False if LVGL had no memory; then
  // nothing is left of them.
  bool build_row(std::size_t at) {
    const void* value = value_at(at);
    lv_obj_t* w = kind_->build(outlet_, value);
    if (w == nullptr) return false;
    if (!hook(w)) {
      lv_obj_delete(w);
      return false;
    }
    lv_obj_move_to_index(w, static_cast<std::int32_t>(first_ + at));
    shift(keys_, shown_, at);
    keys_[at] = kind_->key(value);
    ++shown_;
    mark(w, at);
    if (kind_->on_row != nullptr) kind_->on_row(w, value);
    return true;
  }

  void erase_row(std::size_t at) {
    lv_obj_t* w = row(at);
    if (w == selected_) selected_ = nullptr;
    quiet_ = true;
    lv_obj_delete(w);
    quiet_ = false;
    shift(keys_, at, shown_ - 1);
    --shown_;
  }

  void move_row(std::size_t from, std::size_t to) {
    lv_obj_move_to_index(row(from), static_cast<std::int32_t>(first_ + to));
    shift(keys_, from, to);
  }

  // The same key: the bound properties are set again. A new key: a new row, unless LVGL has no
  // memory for it; then the old widgets show the new values.
  void change_row(std::size_t at) {
    const void* value = value_at(at);
    const std::int32_t k = kind_->key(value);
    if (k != keys_[at] && build_row(at)) {
      erase_row(at + 1);
      return;
    }
    keys_[at] = k;
    kind_->apply(row(at), value);
  }

  // All rows may be new. The rows whose key went are deleted; each other key keeps its
  // widgets, which move into place; a new key gets a new row.
  bool reset() {
    drop_missing();
    for (std::size_t i = 0; i < size(); ++i) {
      if (!place_row(i)) {
        clear_rows();
        broken_ = true;
        return false;
      }
    }
    while (shown_ > size()) erase_row(shown_ - 1);  // the rows of a key given twice
    return true;
  }

  bool has_key(std::int32_t k) const {
    for (std::size_t i = 0; i < size(); ++i)
      if (kind_->key(value_at(i)) == k) return true;
    return false;
  }

  // Deletes the rows whose key the list no longer has, and the later rows of a key shown twice:
  // then no more rows are shown than the list has, and the keys fit their room.
  void drop_missing() {
    for (std::size_t i = shown_; i-- > 0;)
      if (!has_key(keys_[i]) || shown_before(i)) erase_row(i);
  }

  bool shown_before(std::size_t i) const {
    for (std::size_t j = 0; j < i; ++j)
      if (keys_[j] == keys_[i]) return true;
    return false;
  }

  bool place_row(std::size_t i) {
    const std::int32_t k = kind_->key(value_at(i));
    std::size_t j = i;
    while (j < shown_ && keys_[j] != k) ++j;
    if (j == shown_) return build_row(i);
    if (j != i) move_row(j, i);
    kind_->apply(row(i), value_at(i));
    return true;
  }

  // ---- selection ----

  // Follows the state of .selected, if there is one.
  bool follow_selection() {
    if (kind_->selection == nullptr) return true;
    lv_subject_t* s = kind_->selection();
    return s != nullptr && lv_subject_add_observer_obj(s, &each_view::on_selected, outlet_, this) != nullptr;
  }

  // Lets a click or the focus select the row `w`.
  bool hook(lv_obj_t* w) {
    if (kind_->selection == nullptr) return true;
    return lv_obj_add_event_cb(w, &each_view::on_row_event, LV_EVENT_ALL, this) != nullptr;
  }

  // Gives row `w`, at `at`, the checked state if its key is the selected one.
  void mark(lv_obj_t* w, std::size_t at) {
    if (kind_->selection == nullptr) return;
    lv_subject_t* s = kind_->selection();
    if (s != nullptr && lv_subject_get_int(s) == keys_[at]) select_row(w);
  }

  void select_row(lv_obj_t* w) {
    if (w == selected_) return;
    if (selected_ != nullptr) lv_obj_remove_state(selected_, LV_STATE_CHECKED);
    if (w != nullptr) lv_obj_add_state(w, LV_STATE_CHECKED);
    selected_ = w;
  }

  // The selected key changed: the checked state moves to its row, or to none.
  static void on_selected(lv_observer_t* o, lv_subject_t* s) {
    auto* self = static_cast<each_view*>(lv_observer_get_user_data(o));
    const std::int32_t k = lv_subject_get_int(s);
    lv_obj_t* w = nullptr;
    for (std::size_t i = 0; i < self->shown_ && w == nullptr; ++i)
      if (self->keys_[i] == k) w = self->row(i);
    self->select_row(w);
  }

  // A click on a row, or the focus moving to it, selects it.
  static void on_row_event(lv_event_t* e) {
    const lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED && code != LV_EVENT_FOCUSED) return;
    auto* self = static_cast<each_view*>(lv_event_get_user_data(e));
    if (self->quiet_) return;
    lv_obj_t* w = lv_event_get_current_target_obj(e);
    const auto at = static_cast<std::size_t>(lv_obj_get_index(w)) - self->first_;
    if (lv_subject_t* s = self->kind_->selection()) lv_subject_set_int(s, self->keys_[at]);
  }
};

// The table of one lv::each: the node at Path of `Tree`.
template <const auto& Tree, std::size_t... Path>
struct each_parts {
  static constexpr const auto& node = descend<Path...>(Tree);
  using source = std::remove_cvref_t<decltype(*node.rows)>;
  using row_type = typename source::value_type;
  static constexpr std::size_t row_ids = ids_in(descend<Path..., 0>(Tree)).size();

  static std::size_t size(const void* rows) { return static_cast<const source*>(rows)->size(); }
  static const void* at(const void* rows, std::size_t i) { return &(*static_cast<const source*>(rows))[i]; }
  static std::int32_t key(const void* row) { return key_number(static_cast<const row_type*>(row)->*node.key); }
  static watcher_chain& watchers(const void* rows) { return list_access::watchers(*static_cast<const source*>(rows)); }

  static lv_obj_t* build_row(lv_obj_t* outlet, const void* row) {
    std::array<lv_obj_t*, row_ids + 1> ids{};  // a row keeps no ids: see lv::row_view
    build_state st{ids.data()};
    lv_obj_t* w = build<Tree, Path..., 0>(outlet, st);
    if (w != nullptr && !st.ok) {
      lv_obj_delete(w);
      return nullptr;
    }
    if (w != nullptr) apply_row<Tree, Path..., 0>(w, row);
    return w;
  }

  static void apply(lv_obj_t* w, const void* row) { apply_row<Tree, Path..., 0>(w, row); }
  static void on_row(lv_obj_t* w, const void* row) { node.options.on_row(obj{w}, *static_cast<const row_type*>(row)); }

  static_assert(is_null(node.options.selected.state.source) || !sets_checked(descend<Path..., 0>(Tree)),
                "lv: .selected gives the selected row the checked state, and this row template sets "
                ".checked itself. Keep one of them.");

  static lv_subject_t* selection() {
    const auto& b = node.options.selected.state;
    return b.subject(b.source);
  }

  static constexpr each_kind kind{source::capacity(), &size, &at, &key, &watchers, &build_row, &apply,
                                  is_null(node.options.on_row) ? nullptr : &on_row,
                                  is_null(node.options.selected.state.source) ? nullptr : &selection};
};

template <class S, class T, class K, class... Row>
struct each_node {
  const S* rows;
  K T::* key;
  each_options<T, K> options;
  detail::kids<Row...> kids{};

  // The row template: lv::each(tracks, &track::id)[lv::label(...)]. Variadic, so that two
  // rows are not read as one comma expression.
  template <tree_node... R>
    requires(sizeof...(Row) == 0)
  constexpr each_node<S, T, K, R...> operator[](const R&... row) const {
    static_assert(sizeof...(R) == 1,
                  "lv: lv::each takes one row template. Put several widgets in one: lv::each(...)[lv::row[a, b]].");
    return {rows, key, options, {row...}};
  }

  template <const auto& Tree, std::size_t... Path>
  static lv_obj_t* build(lv_obj_t* parent, build_state& st) {
    static_assert(sizeof...(Row) == 1,
                  "lv: lv::each needs a row template: lv::each(tracks, &track::id)[lv::label(...)].");
    if (!each_view::start(parent, each_parts<Tree, Path...>::kind, descend<Path...>(Tree).rows)) st.ok = false;
    return parent;
  }
};

}  // namespace detail

// A row of the row template for each row of `rows`, named by the key `key`. See the top of this file.
//   lv::each(tracks, &track::id)[row template]
//   lv::each(tracks, &track::id, {.on_row = &start, .selected = lv::bind(current)})[row template]
template <detail::row_source S, class T, class K>
  requires std::same_as<typename S::value_type, T>
constexpr detail::each_node<S, T, K> each(const S& rows, K T::* key, detail::each_options<T, K> options = {}) {
  static_assert(detail::row_key<K>,
                "lv: a key names one row, and is compared as a state compares its value. Give it a "
                "named type, `struct track_id { int value; };`, or an enum. To allow plain integers, "
                "set lv::config::raw_integer_state.");
  return {&rows, key, options, {}};
}

template <class S, class M, class... O>
void each(const S&, M, O&&...) = delete(
    "lv: lv::each takes an lv::list and the member of its rows that holds the key: "
    "lv::each(tracks, &track::id)[...].");

}  // namespace lv
