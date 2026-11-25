# Views and ids

A view is a typed handle to one widget: it reads and changes the widget, and never deletes it.

Header: `<lv/lv.hpp>`, the core.

## The smallest example

A node with `.id = "message"` is reached through the mounted tree, and changed with `set`:

<!-- snippet: examples/mounting/main.cpp local -->
```cpp
  auto shown = lv::mount<toast>(display.screen());  // built in this variable; it never moves
  if (!shown.has_value()) return shown.error() == lv::mount_error::out_of_memory ? 2 : 3;
  shown->get<"message">().set({.text = "Saved at 12:00"});
```

`shown->get<"message">()` is an `lv::view<"label">`. Its `set` takes the label's properties, written in place as in a tree.

## How it works

A view is one pointer to an LVGL widget, typed by the widget's kind. Copying it copies the pointer, and dropping it leaves the widget alone.

`get<"id">()` finds the id when the program is compiled, and returns the view of that widget's kind. No search runs while the program runs.

`set({...})` calls the widget's setters for the properties given, in the same order as a tree does. A style in it is a named style, a look, or one made with `lv::style({...})`: `set` holds it by reference, so that its argument stays small on the stack.

Only `lv::mount` and `lv::owned<T>` own widgets. `lv::weak<T>` empties itself when LVGL deletes the widget.

## What you can do

### Reach a widget of a tree that lives in a global

A tree in a `constinit` global is mounted once the display exists. Its widgets are reached as in a variable:

<!-- snippet: examples/mounting/main.cpp mount-global -->
```cpp
  if (!home_page.mount(display.screen())) return 1;
  home_page->get<"title">().set({.text = "Ready"});
```

### Keep a widget made while the program runs

`lv::owned<lv::view<"label">>` owns a widget that no tree holds, and deletes it when the owner goes. It is safe when the parent deletes the widget first. `lv::weak<V>` points at a widget without owning it, and becomes empty when LVGL deletes it.

### Reach what has no typed form

`view.set<"name">(...)` and `view.get<"name">()` reach any `lv_*_set_*` and `lv_*_get_*` function of the widget by its name. A name the widget does not have stops the build and lists the names it has.

One thing is typed there. A getter of an angle returns an `lv::angle` or an `lv::decidegrees`, so `image.get<"rotation">() == 90 * deg` holds whatever LVGL's number is ([Values and units](values.md)).

`raw()` gives the widget's C pointer: the one door to LVGL's C API.

## What the compiler checks

An id the tree does not have lists the ids it has:

<!-- message: unknown_id -->
```text
error: constexpr message: lv: the tree has no id 'titel'. Its ids: title.
```

A style written in place in `set` says how to write it:

<!-- snippet: tests/fail/set_inline_style.cpp -->
```cpp
#include <lv/lv.hpp>

using namespace lv::units;

// set() holds a style by reference, so a style written in place is made with lv::style({...}).
void f(lv::view<"obj"> box) { box.set({.style = {.radius = lv::raw(2 * px)}}); }
```

<!-- message: set_inline_style -->
```text
error: use of deleted function 'Self lv::view<W>::set(this Self, lv::detail::lazy_props<W, D>&&, None ...) [...]': lv: a style in set() is made with lv::style({...}): view.set({.style = lv::style({.radius = ...})}).
```

These stop the build too, each with a test of its own in `tests/fail/`:

- a name that the widget has no setter for, in `set<"name">` (`unknown_property`);
- a set of properties prepared in a variable, where `set` takes them in place (`set_named_props`, `set_constexpr_props`);
- an `lv::owned` or an `lv::weak` of something that is no view (`owned_not_a_view`, `weak_not_a_view`).

## What it costs

| Cost | What we know | Where it comes from |
|---|---|---|
| Processor time | not measured yet | not measured yet |
| RAM | A view is one pointer. An `lv::weak` adds a handler on its widget, which LVGL keeps on its heap. | [owned.hpp](https://github.com/filipsajdak/lvgl26/blob/main/include/lv/owned.hpp), from the code |
| ROM | not measured yet | not measured yet |
| Stack | One `set` stays within a budget of 4 KB of stack on a Cortex-M4, checked every night. | [stack_budget.py](https://github.com/filipsajdak/lvgl26/blob/main/tools/stack_budget.py), the budget |
| Compile time | An id costs nothing measurable: ten labels took 55.8 G instructions, with and without an id each. | [compile-diagnosis.md](https://github.com/filipsajdak/lvgl26/blob/main/docs/research/compile-diagnosis.md), measured 2026-11-19 |

## Rules

<!-- rules: checked-9 -->
- [`checked-9`](rules.md#checked-9): Reach a widget of a mounted tree by its id, `root->get<"title">()`; never keep a pointer to it or search for it at run time.
<!-- rules: end -->

## Limits

- A view is valid while its widget lives. Keep an `lv::weak` where the widget may go first.
- The widgets that `lv::for_each` repeats have no id, so they are not reached by `get`.
- A widget of a row of `lv::each` is reached through `lv::row_view` ([Lists](lists.md)).

What is planned is on the [status page](https://filipsajdak.github.io/lvgl26/status.html).

## Names

`<lv/lv.hpp>` declares `lv::view`, `lv::owned`, `lv::weak` and `lv::mounted`, as [headers.json](https://github.com/filipsajdak/lvgl26/blob/main/docs/reference/headers.json) lists them. The getters and setters each widget has are in the reference data: [lvgl-v9.6.0.json](https://github.com/filipsajdak/lvgl26/blob/main/docs/reference/lvgl-v9.6.0.json).
