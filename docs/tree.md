# The tree and its widgets

A screen is one constant: a tree of widgets that the compiler checks and `lv::mount` builds.

Header: `<lv/lv.hpp>`, the core.

## The smallest example

This is the screen of the [quick start](quick-start.md):

<!-- snippet: examples/quick_start/main.cpp tree -->
```cpp
constexpr auto ui = lv::column(
  {.flex_align = {"center", "center", "center"}, .size = {320 * px, 240 * px}},
  lv::label({.text = "Speed"}),
  lv::arc(
    {
      .range = {0, 240},
      .size  = {120 * px, 120 * px},
      .value = lv::bind(speed),
    },
    lv::label({.align = "center", .text = lv::bind(speed)})
  ),
  lv::button({.on_clicked = &reset}, lv::label({.text = "Reset"}))
);
```

Each widget takes its properties in braces first, then its children as further arguments. The label inside the arc follows `speed`, and the button's handler `reset` sets it.

## How it works

`lv::props<"arc">` is a struct that C++26 reflection builds from LVGL's own C API. It has one member for each `lv_arc_set_*` and `lv_obj_set_*` function, and one for each event, such as `on_clicked`. Nothing is written by hand, and nothing is generated offline.

A tree is a `constexpr` value. `lv::mount<ui>(parent)` reads it at compile time, builds the widgets and returns the object that owns them. An optimized build holds only the LVGL calls for the properties that are set.

The members of a props struct come in four groups: the id, the properties, the styles, the handlers. Each group is in alphabetical order, so a list reads `.id`, `.range`, `.value`, `.style`, `.on_clicked`. C++ wants designated initializers in the order of the members, so a list is written in that order.

A node calls its setters in the same order, except where LVGL needs another. A button matrix's map runs first, and a text area's cursor position last; one table in the library says which, and why.

[Where a tree lives](mounting.md) says where the mounted object can be.

## What you can do

### Nest widgets, and name the pieces

A screen lists named pieces: a constant for a widget, or a function that returns a piece of tree. Here `cups_text` is a constant, and `stepper<cups>` a part that takes it.

<!-- snippet: examples/state/main.cpp tree -->
```cpp
constexpr auto ui = lv::column(
  {.flex_align = {"center", "center", "center"}, .size = {320 * px, 240 * px}},
  lv::dropdown({
    .options  = lv::options_of<drink>,
    .selected = lv::bind(chosen),
    .width    = 140 * px,
  }),
  stepper<cups>(cups_text),
  lv::slider({
    .range = {1, 5},
    .value = lv::bind(bold),
    .width = 200 * px,
  }),                                                    // follows, and writes back
  lv::label({.text = lv::bind(dose)}),                   // a quantity shows its unit: "18 g"
  lv::label({.text = lv::bind(needed, "{} g in all")}),  // a format shows the number only
  lv::label({.hidden = lv::bind(enough), .text = "Not enough beans"})
);
```

Keep each piece small, at most about three levels deep, and let the screen list them.

### Reach a widget that has no short name

`lv::widget<"buttonmatrix">({...})` reaches any widget of LVGL by its name. `lv::place<ui>(screen)` builds a tree that keeps nothing at run time, and leaves its widgets to the screen.

<!-- snippet: examples/lvgl/widgets/buttonmatrix_map.cpp -->
```cpp
#include "widgets.hpp"

namespace {

using namespace widgets;
using namespace lv::units;

// A 3 by 3 keypad: the "\n" entries end a row, and the last entry is null.
// The widget keeps a pointer to the map, not a copy.
constexpr const char* const keypad[] = {
  "1", "2", "3", "\n", "4", "5", "6", "\n", "7", "8", "9", nullptr
};

constexpr auto ui = lv::widget<"buttonmatrix">({
  .align = "center",
  .map   = keypad,
  .size  = {90 * pct, 200 * px},
});

}  // namespace

void widgets::buttonmatrix_map(lv::obj screen) { lv::place<ui>(screen); }
```

### Lay the tree out

`python3 tools/format_ui.py <files>` runs clang-format with the repository's `.clang-format`, then a pass that changes white space only. A property list that ends with a comma gets one property per line; one without it stays on one line while it fits.

### Put the properties in order

`python3 tools/order_properties.py <files>` moves the properties of existing code into the order of the props. It skips a list with a comment between its properties, and says so.

## What the compiler checks

A property out of order is named:

<!-- snippet: tests/fail/props_wrong_order.cpp -->
```cpp
#include <lv/lv.hpp>
// The id comes first: a property before it is out of order, and GCC says so.
constexpr auto ui = lv::label({.text = "x", .id = "title"});  // order on purpose
```

<!-- message: props_wrong_order -->
```text
tests/fail/props_wrong_order.cpp:3:30: error: designator order for field 'lv::detail::props_of<lv::detail::fixed<6>{"label"}, void>::type::id' does not match declaration order in 'lv::detail::lazy_props<lv::detail::fixed<6>{"label"}, void>' {aka 'const lv::detail::props_of<lv::detail::fixed<6>{"label"}, void>::type'}
```

A wrong value says what the property takes and what you gave:

<!-- message: props_needs_unit -->
```text
error: static assertion failed: lv: wrong value for this property. It takes a width or x (120 * lv::px, 50 * lv::pct or lv::size_content, not a vertical length). You gave int.
```

A misspelled name of an LVGL value lists the right ones:

<!-- message: enumerator_unknown_name -->
```text
error: constexpr message: lv: align has no value 'centre'. Its values: default, top_left, top_mid, top_right, bottom_left, bottom_mid, bottom_right, left_mid, right_mid, center, out_top_left, out_top_mid, out_top_right, out_bottom_left, out_bottom_mid, out_bottom_right, out_left_top, out_left_mid, out_left_bottom, out_right_top, out_right_mid, out_right_bottom.
```

A widget that does not exist stops the build too (`unknown_widget`), and so does an id the tree does not have (`unknown_id`).

A misspelled property is not named. GCC 16.2 says `no match for call to '(const lv::detail::factory<...>) (<brace-enclosed initializer list>)'` and points at the line. The [editor stubs](editor-support.md) mark the name itself.

## What it costs

| Cost | What we know | Where it comes from |
|---|---|---|
| Processor time | not measured yet | not measured yet |
| RAM | A tree is a constant. At run time lvgl26 keeps only what the mounted object holds, which the compiler sizes. | [mounted_storage.cpp](https://github.com/filipsajdak/lvgl26/blob/main/tests/mounted_storage.cpp), from the test |
| ROM | not measured yet | not measured yet |
| Stack | not measured yet | not measured yet |
| Compile time | A widget kind costs a file about 5.6 G instructions the first time it appears, and each further node about 0.2 G. | [compile-diagnosis.md](https://github.com/filipsajdak/lvgl26/blob/main/docs/research/compile-diagnosis.md), measured 2026-11-19 |

## Rules

<!-- rules: no-boilerplate-1 checked-2 expressive-6 expressive-7 -->
- [`no-boilerplate-1`](rules.md#no-boilerplate-1): Write a screen as one constant tree, `constexpr auto ui = lv::column[...]`, and mount it; never create its widgets one by one.
- [`checked-2`](rules.md#checked-2): Write an LVGL enumerator as its name in quotes, `.align = "center"`; a misspelled name stops the build and lists the right ones.
- [`expressive-6`](rules.md#expressive-6): Build a screen from named pieces: small constants, each at most about three levels deep, that the screen lists.
- [`expressive-7`](rules.md#expressive-7): Write a tree in the call form and let clang-format lay it out: the children as arguments, two spaces, and a trailing comma after a property list when it should have one property per line.
<!-- rules: end -->

## Limits

- A node sets each property once. A setter that LVGL calls several times, such as one for a table's cells, has one value per node.
- A child that LVGL makes with an `add_` function, such as a tab or a chart series, has no node yet.
- A tree has one root; `lv::place` puts several trees on one screen.

What is planned for the tree is on the [status page](https://filipsajdak.github.io/lvgl26/status.html).

## Names

`<lv/lv.hpp>` declares `lv::group`, `lv::column`, `lv::row`, `lv::widget`, `lv::props`, `lv::mount` and `lv::place`, as [headers.json](https://github.com/filipsajdak/lvgl26/blob/main/docs/reference/headers.json) lists them. Which properties a widget has, and what each takes, is in the reference data: [lvgl-v9.6.0.json](https://github.com/filipsajdak/lvgl26/blob/main/docs/reference/lvgl-v9.6.0.json).
