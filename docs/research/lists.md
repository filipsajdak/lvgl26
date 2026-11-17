# Lists: rows that change, on a small device

Research and design for lists in lvgl26, written 2026-11-17.

Marks follow `docs/research/README.md`. Every mark is dated 2026-11-17 unless its row says otherwise.

- `source`: LVGL's code, read in the local checkout of v9.6.0 and the v10 pin, or our own code.
- `measured`: compiled or measured in this session. The method is under "Measured here".

The owner decided the scope: four slices, rows named by stable keys, a fixed-capacity model, a pull model, and structural nodes over an outlet. `docs/research/components.md`, section 4 and part A, holds what other toolkits do.

## Short answers

| Question | Answer | Evidence |
|---|---|---|
| How is a list written? | `lv::each(tracks, &track::id)[row template]`, over a `constinit lv::list<track, 20>` | `tests/lists.cpp` |
| What does one change touch? | The widgets of its own row. Insert at the front of 19 rows deletes and sets no other row | "Measured here" |
| How does a row show its values? | `lv::bind(&track::title)` in the row template. Text is shown from the row itself, not copied | `tests/lists.cpp` |
| What does a row cost? | The widgets of its template. lv::each adds nothing per row, and one block per view | "Measured here" |
| How is a row selected? | `.selected = lv::bind(current)`: a state of the key's type. The selected row is `checked` | `tests/lists_selection.cpp` |
| What if the list is full? | `insert()` returns `list_error::full` and changes nothing | `tests/lists.cpp` |

## Measured here

GCC 16.2 on macOS arm64, LVGL v9.6.0, the `host-v96` build. Heap is LVGL's, read with `lv_mem_monitor` after one warm-up of each way. Pointers are 8 bytes here, so a Cortex-M4 needs about two thirds of each figure.

| What | Result | How |
|---|---|---|
| Heap of a row of two labels, mounted as a tree for each row | 1094 bytes | `measures_the_heap_per_row` in `tests/lists.cpp` |
| The same row through lv::each | 960 bytes | same |
| Heap of a printer queue card, mounted again for each job as the port does | 6345 bytes | `tests/lists_printer.cpp` |
| The same card through lv::each, with its tap task | 6145 bytes | same |
| What lv::each keeps for one view of a list of 20, without rows | 200 bytes | `heap_of_view` in `tests/lists.cpp` |
| Cards deleted when the first job gains a percent, of 6 | 6 remounted, 0 with lv::each | `tests/lists_printer.cpp` |
| Cards deleted when a job is added | 6 remounted, 0 with lv::each | same |
| Cards deleted when the second job is cancelled | 7 remounted, 1 with lv::each | same |
| Code of one row mounted once | 1040 bytes of `__text` | `size -m`, `-Os`, one file per case |
| Code of one list with lv::each and five changes | 4260 bytes | same |
| Code of a second list in the same file | 1384 more | same |
| Compile cost of `tests/large_tree.cpp`, before | 89.60 to 89.76 G instructions; 5,768,554 constexpr operations | `cc1plus` under `/usr/bin/time -l`; bisecting `-fconstexpr-ops-limit` with `-fsyntax-only` |
| The same, after | 90.04 to 90.17 G instructions, about 0.4 percent more; 5,768,554 constexpr operations | same |
| Parsing `lv/lv.hpp`, which now includes `lv/list.hpp` | 33.31 G instructions with it and without it | an empty file that includes it |
| `lv/list.hpp` with `<ranges>`, `<iterator>` and `<algorithm>`, as first written | 0.55 G instructions more in every file | same |

What the numbers mean:

- A row costs what its widgets cost. The 134 bytes saved per row are the copied texts and the owner of each tree.
- The large saving is in work, not in bytes: a change rebuilds no other row, so animations, tasks and the focus go on.
- The code of a view is in one class that is no template. Each further list adds its table and its row template only.
- A list costs no compile time where it is not used. The standard headers it first included cost half a second in every file, so it uses none of them.

## Findings in LVGL and GCC

| Finding | Link | Mark |
|---|---|---|
| `LV_OBJ_FLAG_STATE_TRICKLE` hands a state to the children of a widget. `lv_obj_set_state_trickle` sets it, in v9.6 and the v10 pin | [lv_obj.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/core/lv_obj.c) | `source` |
| A child passes a trickled state on only if it has the flag itself | same, `lv_obj_children_add_state` | `source` |
| `lv_obj_set_checked(obj, bool)` exists in both versions, so `.checked` is a property with one value | `include/lvgl/core/lv_obj.h` | `source` |
| `lv_obj_move_to_index` moves one child and shifts the others; no child is created | [lv_obj_tree.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/core/lv_obj_tree.c) | `source` |
| `LV_EVENT_DELETE` reaches a widget before its children are deleted | same, `obj_delete_core` | `source` |
| An event handler removed while its list is traversed is marked and freed later. It still shows in `lv_obj_get_event_dsc` | [lv_event.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/misc/lv_event.c) | `source` |
| A bubbled event runs the parent's class handler too. A bubbled `FOCUSED` would mark the parent focused | [lv_obj_event.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/core/lv_obj_event.c) | `source` |
| `lv_subject_t` is opaque in the public headers of both versions. A subject cannot live on the stack | `include/lvgl/lv_types.h` | `source` |
| `lv_label_set_text_static` only marks the label; its size is computed at the next refresh | [lv_label.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/widgets/label/lv_label.c) | `source` |
| The first widget added to an empty group takes the focus while it is created | `tests/lists_selection.cpp` | `measured` |
| Under `-fsanitize=undefined`, GCC 16.2 does not compare two different function addresses in a constant expression | scratch file, `static_assert` | `measured` |
| The same holds for the address of an inline variable or a template parameter object against null | same | `measured` |
| Comparing `std::meta::reflect_constant` of the two values works in every build | same | `measured` |
| Of two `-fconstexpr-ops-limit` options, GCC takes the last. `large_tree` gets 9,000,000, then the library's 1,000,000,000 | scratch file; `compile_commands.json` | `measured` |

The last row means the guard of `tests/large_tree.cpp` does not stop a build today. Only a bisect shows its cost.

## The design

### The model: `lv::list<T, N>`

- A fixed number of cells, chosen at compile time, and a permutation of small numbers that orders them.
- A row keeps its address from insert to erase. `move()` and `insert()` shift the numbers, never the rows.
- So a label shows a text field of a row without a copy, through `lv_label_set_text_static`.
- A change reports one of: inserted, erased, moved, changed, reset (from `assign()` and `clear()`).
- A full list is data, not a bug: a Start button can fill a print queue. `insert()` returns `std::expected` with `list_error::full`.
- An index out of range, or a change while views follow one, is a bug: a `pre`.
- `count()` is the number of rows as an `lv::state<lv::counter>`. A computed of it hides a "No tracks" label.

### The view: `lv::each`

- A structural node of the constant tree. It builds no widget of its own: its rows are children of the widget that holds it.
- `lv::column` holds a list from top to bottom, `lv::row` one from left to right. The flex layout gives each row its own size.
- A widget holds one `lv::each` at most, with any other children before and after it. The rows start at a fixed child number.
- The view keeps a block on LVGL's heap: the key of each row and a link into the list's watchers. The holder's deletion frees it.
- Insert builds one row, erase deletes one, move calls `lv_obj_move_to_index`, a change sets the bound properties of one row again.
- `assign()` keeps the widgets of every key it finds again, and moves them into place.
- A key that changes in `update()` makes a new row: a key names the row.
- Without memory for a row, `insert()` undoes itself and returns `list_error::out_of_memory`.

### Bindings to the fields of a row

- `lv::bind(&track::title)` makes a constant that holds the member pointer and a reader. It fills the pointers that a state binding fills, and `follows` says it is a row field.
- So `lv::arg` keeps its size, and a large tree pays nothing for it.
- A node outside a row template refuses such a binding at compile time. `view.set()` at run time breaks a contract.
- A field follows the rules of a state: a quantity, a named value, an enum, a bool, a color, a pointer, or a text.

### Styling rows, and the selected row

- A row template is a constant tree, so every style of a node works, from read-only memory, shared by all rows.
- The selected row gets LVGL's `checked` state. `.style_checked` on any node of the template styles it.
- `.state_trickle = true` hands the state to the children of a node, so the label of a selected row restyles too.
- `.checked = lv::bind(&job::printing)` gives a row the same state from a field: one template for two looks.
- A click on a row, or the focus moving to it, selects it. A button row joins the default group, so keys move the selection.
- Once `lv::on` (style selectors) lands, it can name the checked state of a row instead of `.style_checked`.

### What happens to a row when it goes

- Its widgets are deleted. LVGL removes their handlers, observers and animations with them.
- A task started with `lv::spawn(work, row)` ends: `.on_row` is the place to start one.
- Its ids need no memory: `lv::row_view<card>{row}.get<"title">()` follows the template from the row's root.
- A state bound in the row template stops following with the widget, as LVGL's observers do.

### What the other parts needed

- `tree.hpp`: a trait for structural nodes, `operator[]` accepting them, `build` handing them over, and ids not collected below them.
- `tree.hpp`: `apply_node` skips a row field and refuses one outside a row template.
- `values.hpp`: a type for a row binding, and `follows` of `lv::arg` as a number: nothing, a state, or a row field.
- `props.hpp`: `from_number`, the conversion a subject used, now shared with row fields.
- `state.hpp` and `owned.hpp`: unchanged. The block of a view does not live in `mounted`, so the new storage of `mounted` can take it later.

## The API of the four slices

Slices 1 and 2 are built. The code below is from the tests, which build and run it.

<!-- snippet: tests/lists.cpp model -->
```cpp
struct track_id {
  int value;
};

struct track {
  track_id id;
  const char* title;
  lv::counter plays;
};

constinit lv::list<track, 20> tracks;  // room for 20 rows
```

<!-- snippet: tests/lists.cpp row -->
```cpp
constexpr auto track_row =
    lv::row({.size = {100 * pct, lv::size_content}})[
        lv::label({.id = "title", .text = lv::bind(&track::title)}),
        lv::label({.id = "plays", .text = lv::bind(&track::plays, "{} plays")})];

constexpr auto playlist = lv::column({.size = {320 * px, 240 * px}})[
    lv::each(tracks, &track::id)[track_row]];
```

Slice 2, the selected row:

<!-- snippet: tests/lists_selection.cpp selection -->
```cpp
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
```

Slices 3 and 4 are sketches. They compiled with `-fsyntax-only` against the headers of slices 1 and 2, with prototypes that do nothing (`measured`, the file is not in the repository).

Slice 3, the pull model: a count and a function that gives row i. A recycling view keeps the rows that fit.

```cpp
const song& song_at(std::size_t i) { return catalog[i]; }
constinit lv::rows<song> library{&song_at, 500};

constexpr auto browser = lv::column({.size = {320 * px, 240 * px}})[
    lv::each_visible(library, &song::id, {.row_size = 40 * px})[song_row]];

library.resize(480);   // tells the views
library.changed(12);   // a view refills row 12 if it shows it
```

- A view keeps a pool of rows, as many as fit plus two, and places each one itself. Scrolling refills a row with `apply`.
- The primitive is the one slice 1 already has: set the bound fields of a row from a value.
- Rows of different sizes take `.size_of = &height_of` in place of `.row_size`. The view sums the sizes of the rows it passes.
- A row of a pull model has no fixed address. Its text is copied, unless the function returns a reference that lasts.

Slice 4, views that sort and filter a list without copying a row:

```cpp
constinit lv::sorted by_title{tracks, [](const track& a, const track& b) { return std::strcmp(a.title, b.title) < 0; }};
constinit lv::filtered unplayed{tracks, [](const track& t) { return t.plays.value == 0; }};

constexpr auto both = lv::row[
    lv::column[lv::each(by_title, &track::id)[track_row]],
    lv::column[lv::each(unplayed, &track::id)[track_row]]];
```

- A view holds the numbers of the list's rows in its own order: one or two bytes a row, at the list's capacity.
- It watches the list and reports its own changes. A changed row moves in a sorted view, and enters or leaves a filtered one.
- `lv::each` takes it unchanged: the sketch satisfies the same `row_source` concept as `lv::list`.

## Open choices, with a recommendation

| Choice | Options | Recommendation |
|---|---|---|
| Name of the model | `lv::list`, `lv::rows`, `lv::fixed_list` | `lv::list`: the owner's sketch, and LVGL's list widget is gone on master |
| Name of the view | `lv::each`, `lv::for_each`, `lv::repeat` | `lv::each`: short, and Svelte and Solid use it |
| Name of the row handle | `lv::row_view`, `lv::row_of`, `lv::entry` | `lv::row_view`; `lv::row` is the flex row already |
| The selected state | `checked`, a user state, a state chosen per list | `checked`: LVGL's list example uses it, and `.style_checked` exists |
| Selection and a `.checked` field in one list | refuse, allow | refuse at compile time in a later change; today both write one state |
| Plain integer keys | refuse, allow | refuse, as for a state; `lv::config::raw_integer_state` allows them |
| Where the list lives | a global now, a member of a component later | a global now; `lv::each(&playlist::tracks, ...)` needs the instance that `lv::use` brings |
| `lv::when` | build it now, later | later: a hidden flag bound to `count()` covers "No tracks" without a new node |
| `assign()` without memory | the view shows nothing until the next change, or undoes | show nothing: undoing a whole assign needs a second copy of the rows |
| Pool size of a recycling view | from the size at mount, fixed at compile time | at mount, with `.pool` to fix it |
| Two lists in one widget | refuse, allow | refuse: each list's rows start at a fixed child number |
| Focus order after an insert or a move | keep LVGL's order of building, or reorder the group | reorder later: `lv_group_swap_obj` also moves the focus, which a selection follows |
