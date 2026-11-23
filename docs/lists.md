# Lists

A list shows a row template once for each row of an `lv::list`, and a change touches only the widgets of its own row.

Header: `<lv/list.hpp>`

![A playlist of four tracks on a dark screen; Northern Lights is selected in blue](media/lists.png)

## The smallest example

The rows are data: a struct for one row, and a list with room for a fixed number of them.

<!-- snippet: examples/lists/main.cpp model -->
```cpp
struct track_id {
  int value;
};

struct track {
  track_id id;  // names the row
  const char* title;
  lv::counter plays;
};

constinit lv::list<track, 8> tracks;       // room for eight rows
constinit lv::state current{track_id{0}};  // the selected row; no track has the id 0
```

`lv::each(tracks, &track::id, track_row)` puts one `track_row` into the tree for each row. The rows go where `lv::each` stands: here in a column, from top to bottom. The braces before the row hold its options, such as the selected row: `lv::each(tracks, &track::id, {.selected = lv::bind(current)}, track_row)`.

<!-- snippet: examples/lists/main.cpp tree -->
```cpp
constinit lv::computed has_tracks{
  [](lv::counter n) {
    return n.value > 0;
  },
  tracks.count(),
};

// Shown while the list is empty.
constexpr auto empty_note = lv::label({
  .hidden = lv::bind(has_tracks),
  .text   = "No tracks",
  .style  = {.text_color = tok.color.quiet},
});

// One row per track; the selected one is checked.
constexpr auto track_list = lv::group(
  {
    .flex_flow = "column",
    .size      = {100 * pct, lv::size_content},
    .style     = {.pad_gap = tok.space.gap},
  },
  lv::each(tracks, &track::id, {.selected = lv::bind(current)}, track_row)
);

// Groups: boxes without the theme's look, laid out as columns.
constexpr auto ui = lv::group(
  {
    .flex_flow = "column",
    .size      = {320 * px, 240 * px},
    .style = {
      .bg_color = tok.color.screen,
      .bg_opa   = tok.opacity.opaque,
      .pad_all  = tok.space.edge,
      .pad_gap  = tok.space.gap,
    },
  },
  empty_note, track_list
);
```

Once the tree is mounted, a change of the list builds the widgets. `assign` fills it from one constant array of rows.

<!-- snippet: examples/lists/main.cpp fill -->
```cpp
  if (!tracks.assign(playlist)) return 2;  // builds a row for each track: full, or no memory
```

## How it works

An `lv::list<T, N>` holds up to N rows of type T, in memory of its own. N is fixed at compile time, so the list never takes heap.

A row keeps its address from the moment it is added until it is erased. A move or an insert shifts small numbers, never the rows.

A key names each row. It is a field of the row: a struct with one member, or an enum. Two rows never have the same key.

`lv::each` is a node of the constant tree that builds no widget of its own. Its rows become children of the widget that holds it.

The list tells each view what changed: a row inserted, erased, moved or changed, or all rows replaced. The view then builds, deletes, moves or sets the widgets of that one row.

A row template is a constant tree like any other, so all rows share its styles from read-only memory.

## What you can do

### Show the fields of a row

`lv::bind(&track::title)` makes a property follow a field of the row. A text field is shown without a copy, since the row stays at one address. A number field needs a format, as a state does.

<!-- snippet: examples/lists/main.cpp row -->
```cpp
// One row: a button that is checked while it is the selected one. state_trickle hands that
// state to the labels, which style themselves for it.
constexpr auto track_row = lv::button(
  {
    .flex_flow     = "row",
    .size          = {100 * pct, lv::size_content},
    .state_trickle = true,
    .style = {
      .bg_color = tok.color.row,
      .pad_all  = tok.space.edge,
      .radius   = tok.radius.row,
    },
    .style_checked = {.bg_color = tok.color.selected},
  },
  lv::label({
    .flex_grow     = 1,
    .text          = lv::bind(&track::title),
    .style         = {.text_color = tok.color.text},
    .style_checked = {.text_color = tok.color.on_selected},
  }),
  lv::label({
    .text          = lv::bind(&track::plays, "{} plays"),
    .style         = {.text_color = tok.color.quiet},
    .style_checked = {.text_color = tok.color.on_selected},
  })
);
```

A field holds what a state holds. That is a text (`const char*`, `std::string` or a character array), a quantity, a named value, an enum, a bool, a color or a pointer.

### Fill the list

The rows to start with are one constant array. `assign` replaces all rows with copies of them.

<!-- snippet: examples/lists/main.cpp rows -->
```cpp
// The playlist to start with, in one constant.
constexpr track playlist[] = {
  {{4}, "Overture", lv::counter{0}},
  {{1}, "Intro", lv::counter{3}},
  {{2}, "Northern Lights", lv::counter{1}},
  {{3}, "Harbour", lv::counter{12}}
};
```

`assign`, `insert` and `push_back` return `std::expected`. `lv::list_error::full` says the list has no room; then nothing changed. `lv::list_error::out_of_memory` says LVGL had no memory for the widgets of a row.

### Change one row

<!-- snippet: examples/lists/main.cpp changes -->
```cpp
void play(std::size_t row) {
  tracks.update(row, [](track& t) {
    ++t.plays;
  });  // that row's labels are set again
}
```

<!-- snippet: examples/lists/main.cpp move -->
```cpp
  play(2);
  tracks.move(3, 1);      // Harbour moves up, with its widgets
  current = track_id{2};  // selects Northern Lights
```

| Call | What the views do |
|---|---|
| `insert(i, row)`, `push_back(row)` | Build the widgets of the new row, at its place |
| `erase(i)` | Delete the widgets of that row |
| `move(from, to)` | Move the widgets of that row |
| `update(i, f)` | Set the bound properties of that row again; a new key makes a new row |
| `assign(rows)` | Keep the widgets of every key found again, and move them into place |
| `clear()` | Delete the widgets of all rows |

An index out of range breaks a precondition. So does a change of the list while its views follow another change.

### Count the rows

`tracks.count()` is the number of rows as an `lv::state<lv::counter>`. In the tree above, a computed of it hides the label "No tracks".

### Select a row

`.selected = lv::bind(current)` makes a state of the key's type follow the selected row. A click on a row selects it, and so does the focus. Setting the state from code selects too.

The selected row has LVGL's `checked` state, so `.style_checked` styles it. `.state_trickle = true` hands that state to the children of a node.

A row that is a button joins the default group, so the keys move the selection. A row that goes leaves the selection as it was.

### Start work for a row

`.on_row = &start` in the options of `lv::each` is called with each row that is built, and its value. A task started there with `lv::spawn(work, row)` ends when the row goes. `lv::row_view<track_row>{row}.get<"title">()` finds a widget of the row by an id of its template, with no memory of its own. No example shows this yet.

## What the compiler checks

A key must say what it is. A plain `int` is refused:

<!-- snippet: tests/fail/each_plain_key.cpp -->
```cpp
#include <lv/list.hpp>
#include <lv/lv.hpp>
struct track {
  int id;
  const char* title;
};
constinit lv::list<track, 4> tracks;
constexpr auto ui = lv::column[lv::each(tracks, &track::id)[lv::label()]];
```

<!-- message: each_plain_key -->
```text
error: static assertion failed: lv: a key names one row, and is compared as a state compares its value. Give it a named type, `struct track_id { int value; };`, or an enum. To allow plain integers, set rules.raw_integer_state in the settings.
```

A widget holds one `lv::each` at most:

<!-- snippet: tests/fail/each_twice_in_one_widget.cpp -->
```cpp
#include <lv/list.hpp>
#include <lv/lv.hpp>
struct track_id {
  int value;
};
struct track {
  track_id id;
};
constinit lv::list<track, 4> tracks;
constexpr auto ui = lv::column[lv::each(tracks, &track::id)[lv::label()], lv::each(tracks, &track::id)[lv::label()]];
```

<!-- message: each_twice_in_one_widget -->
```text
error: static assertion failed: lv: a widget holds at most one lv::each. Put each one in a widget of its own: lv::column[lv::each(a, &row::id)[...]], lv::column[lv::each(b, &row::id)[...]].
```

A field binding belongs in a row template:

<!-- snippet: tests/fail/row_field_outside_each.cpp -->
```cpp
#include <lv/list.hpp>
#include <lv/lv.hpp>
struct track {
  const char* title;
};
constexpr auto ui = lv::column[lv::label({.text = lv::bind(&track::title)})];
void f(lv::obj screen) { lv::mount<ui>(screen); }
```

<!-- message: row_field_outside_each -->
```text
error: static assertion failed: lv: lv::bind(&T::field) follows a field of a row, and this node is not in a row. Use it in the row template of lv::each: lv::each(tracks, &track::id)[lv::label({.text = lv::bind(&track::title)})].
```

These mistakes stop the build too, each with a test of its own in `tests/fail/`:

- two row templates in one `lv::each` (`each_two_rows`), or none (`each_without_row`);
- a key that is a member of another row type (`each_key_of_another_row`);
- a field of a plain integer (`row_field_plain_int`), or a `std::string_view` field (`row_field_string_view`);
- a member function in place of a field (`row_field_function`);
- a field that the property cannot show (`row_field_wrong_property`);
- a `.selected` state of another type than the key (`each_selected_wrong_type`);
- an id that the row template does not have (`row_view_unknown_id`).

## What it costs

| Cost | What we know | Where it comes from |
|---|---|---|
| Processor time | A change builds, moves or deletes the widgets of one row. When a job was added to the printer's queue of six, the port's own loop built all six cards again and `lv::each` built none. | [lists.md](https://github.com/filipsajdak/lvgl26/blob/main/docs/research/lists.md), measured 2026-11-17 |
| RAM | The list is a global of N rows and takes no heap. A row takes the heap of its widgets: 960 bytes for two labels on a 64-bit host. A view of a list of 20 keeps 200 bytes. | [lists.md](https://github.com/filipsajdak/lvgl26/blob/main/docs/research/lists.md), measured 2026-11-17 |
| ROM | One list with `lv::each` and five kinds of change: 4260 bytes of code at `-Os` on a 64-bit host. A second list in the same file: 1384 bytes more. | [lists.md](https://github.com/filipsajdak/lvgl26/blob/main/docs/research/lists.md), measured 2026-11-17 |
| Stack | not measured yet | not measured yet |
| Compile time | `<lv/list.hpp>` adds 0.17 G instructions to a file, against 9.01 G for the core alone. A row template costs what the same widgets cost written out. | [header-structure.md](https://github.com/filipsajdak/lvgl26/blob/main/docs/research/header-structure.md), measured 2026-11-20 |

## Rules

<!-- rules: no-boilerplate-4 fast-run-1 fast-run-2 fast-run-3 checked-5 fast-build-1 -->
- [`no-boilerplate-4`](rules.md#no-boilerplate-4): Show rows that change with `lv::each` over an `lv::list`, and change the list, never the widgets.
- [`fast-run-1`](rules.md#fast-run-1): Keep states, lists and mounted trees in `constinit` globals or in variables: lvgl26 takes no heap for them.
- [`fast-run-2`](rules.md#fast-run-2): Change one row with `insert`, `erase`, `move` or `update`; never clear and fill a list again to change it.
- [`fast-run-3`](rules.md#fast-run-3): Show a text field of a row through `lv::bind`; do not copy it into a buffer of your own.
- [`checked-5`](rules.md#checked-5): Handle the `std::expected` that `lv::mount`, `insert` and `assign` return: a full list or a heap without room is a value, not a crash.
- [`fast-build-1`](rules.md#fast-build-1): Include `<lv/lv.hpp>` and the header of each feature the file uses; include `<lv/all.hpp>` only in a first program.
<!-- rules: end -->

## Limits

- A list holds at most N rows, and builds the widgets of every row, also of rows scrolled out of sight.
- A view cannot sort or filter a list: it shows the rows in the list's order.
- A row template cannot hold a node that keeps run-time storage, such as an `lv::one_of` (`each_row_storage`).
- The keys move the focus in the order the rows were built, which an insert or a move does not change.
- A row template that follows `.selected` cannot set `.checked` from a field too (`each_selected_and_checked`).

What is planned for lists is on the [status page](https://filipsajdak.github.io/lvgl26/status.html).

## Names

`<lv/list.hpp>` declares `lv::list`, `lv::list_error`, `lv::each`, `lv::row_view` and the `lv::bind` of a field, as [headers.json](https://github.com/filipsajdak/lvgl26/blob/main/docs/reference/headers.json) lists them. The properties a row template can set, and the names in quotes they take, are in the reference data: [lvgl-v9.6.0.json](https://github.com/filipsajdak/lvgl26/blob/main/docs/reference/lvgl-v9.6.0.json).
