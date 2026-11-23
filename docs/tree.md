# The tree and its widgets

A screen is one constant: a tree of widgets that the compiler reads and `lv::mount` builds.

Header: `<lv/lv.hpp>`, the core.

## A tree

This is the screen of the [quick start](quick-start.md):

<!-- snippet: examples/quick_start/main.cpp tree -->
```cpp
constexpr auto ui = lv::column(
  {
    .flex_align = {"center", "center", "center"},
    .size       = {320 * px, 240 * px},
  },
  lv::label({.text = "Speed"}),
  lv::arc(
    {
      .range = {0, 240},
      .size  = {120 * px, 120 * px},
      .value = lv::bind(speed),
    },
    lv::label({.align = "center", .text = lv::bind(speed)})
  ),
  lv::button({.on_clicked = [] { speed = 0 * km / h; }}, lv::label({.text = "Reset"}))
);
```

Each widget takes its properties in braces first, then its children as further arguments. A widget without properties takes its children alone: `lv::button(lv::label({.text = "OK"}))`.

Each child is an argument, so a formatter lays the tree out as nested blocks. The older form `lv::group({...})[child, child]` builds the same node and keeps compiling.

`lv::mount<ui>(parent)` builds the widgets and returns the object that owns them. [Where a tree lives](mounting.md) says where that object can be.

## Properties come from LVGL

`lv::props<"arc">` is a struct that C++26 reflection builds from LVGL's own C API. It has one member for each `lv_arc_set_*` and `lv_obj_set_*` function, and one for each event, such as `on_clicked`. Nothing is written by hand, and nothing is generated offline.

`lv::widget<"spinbox">({...})` reaches any widget that has no short name.

Which properties a widget has, and what each takes, is in the reference data: [lvgl-v9.6.0.json](https://github.com/filipsajdak/lvgl26/blob/main/docs/reference/lvgl-v9.6.0.json). What a property takes is typed: [Values and units](values.md).

## The order of the properties

The members of a props struct come in four groups: the id, the properties, the styles, the handlers. Each group is in alphabetical order, so a list reads `.id`, `.range`, `.value`, `.style`, `.on_clicked`.

C++ wants designated initializers in the order of the members, so a list follows the same order. `python3 tools/order_properties.py <files>` moves the properties of existing code into it.

## What the compiler checks

A wrong value says what the property takes and what you gave:

<!-- message: props_needs_unit -->
```text
error: static assertion failed: lv: wrong value for this property. It takes a width or x (120 * lv::px, 50 * lv::pct or lv::size_content, not a vertical length). You gave int.
```

- A wrong id lists the ids of the tree: `the tree has no id 'titel'. Its ids: title` (test `unknown_id`).
- A property out of order is named. GCC says `designator order for field ... does not match declaration order` (test `props_wrong_order`).
- A misspelled property is not named. GCC 16.2 says `no match for call to '(const lv::detail::factory<...>) (<brace-enclosed initializer list>)'` and points at the line. The [editor stubs](editor-support.md) mark the name itself.

## What it costs

`lv::mount<ui>` reads the tree at compile time. An optimized build holds only the LVGL calls for the properties that are set.
