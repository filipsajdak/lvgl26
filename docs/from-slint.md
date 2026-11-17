# Coming from Slint

You write `.slint` files and want the same kind of screen in C++.
This page maps what you know to what lvgl26 has today.

Slint compiles its own language to code. In lvgl26 the screen is C++ already: a `constexpr` tree that the C++ compiler checks.
The printer demo of Slint is ported as an example: `examples/ports/slint/printer`.

## Concepts

| Slint | lvgl26 |
|---|---|
| `export component App inherits Window { ... }` | A `constexpr` tree: `lv::column({...})[children]` |
| `VerticalLayout`, `HorizontalLayout` | `lv::column`, `lv::row` |
| `Rectangle` | `lv::box`, or `lv::group` for a box with no padding of its own |
| `Text { text: ...; }` | `lv::label({.text = ...})` |
| `Button { clicked => { ... } }` | `lv::button({.on_clicked = &handler})` |
| `in-out property <int> speed` | An `lv::state`. A measure has a unit in its type |
| `text: root.speed` | `.text = lv::bind(speed)` |
| `value <=> root.speed` (two-way) | `.value = lv::bind(speed)` on an arc or a slider: the widget follows the state and writes back |
| `property <bool> over: speed > limit` | An `lv::computed`, bound like a state |
| `callback clicked()` | A function pointer: `.on_clicked = &handler`, or a coroutine that waits for the event |
| `animate x { duration: 200ms; easing: ease-in; }` | `lv::animate(widget, {.x = {0 * px, 80 * px}}, 200 * ms, lv::ease::in_quad)` |
| `states [ ... ]` and `transitions` | `lv::match` on a state, and animations in a task |
| `Timer { interval: 1s; }` | `co_await lv::sleep(1 * sec)` in an `lv::task` |
| `@image-url("a.png")` | `lv::image_file<a_png>`, decoded by the compiler. A photo may stay a file: `lv::encoded_image` |
| `Palette`, a global `Theme` | A theme, `lv::theme({...})`, and a W3C Design Tokens file |
| A font in a `Text` | A `lv::typeface` that the compiler makes from a TrueType file |
| `for item in model` | Not there yet. See below |
| `@tr("...")` | Not there yet |
| The Slint viewer and live preview | Screenshots: `lv::write_png`, and the agent tools |

## What you get that Slint does not give

- **One language.** There is no `.slint` compiler step and no generated code to read.
- **Units in the type.** A length is not a time, and a speed is not a length.
- **A small runtime.** The tree is built from constants, and a constant text stays in flash.
- **MIT licence.** lvgl26 and LVGL carry no terms that depend on the device or the number of units.

## Things that are the same

- Properties are typed, and a binding keeps a property in step with a value.
- A component is a unit of reuse. In lvgl26 it is a C++ function that returns a piece of tree.
- Colors, sizes and durations come from one place, so a design change touches one file.

## Not there yet

- **Models and lists.** A `for` over a model has no counterpart. The printer port mounts a card for each job again when the queue changes.
- **Components with their own state and slots.** `@children` has no counterpart. They are planned.
- **A style that follows a color state.** A theme cannot change at run time yet, so Slint's four printer themes are not ported.
- **Gradients.** A gradient is vertical or horizontal. Diagonal and multi-stop gradients are missing.
- **Translations.** `@tr` has no counterpart yet.

## Next

- [Quick start](quick-start.md) builds a screen in one file.
- [Themes](themes.md) covers tokens and the look of widgets.
- The printer port, `examples/ports/slint/printer`, shows pages that slide in, a queue and bound controls.
