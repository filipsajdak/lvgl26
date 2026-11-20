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
| `for item in model` | `lv::each(tracks, &track::id)[row template]` over an `lv::list`. See [Lists](lists.md) |
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

## How to port, step by step

These steps are for a person or an agent.
The [walkthrough of the printer panel](port-slint-printer.md) follows them on a whole application.

1. **Inventory the source UI.** List the components, the properties and callbacks, the `for` models, the `states` and `animate` blocks, the `@tr` texts, the fonts, the images and the themes. Check each against the concept table above and the list of what is not there yet.
2. **Make the decisions below.** Do it before you write code. Translations, lists and themes change the shape of the port.
3. **Make the token file.** Copy the colors, sizes, radii, durations and easing curves of your global (the printer demo keeps them in `DemoPalette`) into a W3C Design Tokens file. Add each font as a type style. A style takes tokens only.
4. **Port page by page, one file for each page.** A page is one constant tree and one function that mounts it. Start with the page that needs the fewest other parts. A `component` becomes a C++ function that returns a piece of tree.
5. **Bind state.** A property that several places read becomes an `lv::state`. A property made from others becomes an `lv::computed`. Keep a page's own state in the page's file, and shared state in one header.
6. **Add behaviour.** A `clicked => { ... }` handler becomes a function, named as `.on_clicked = &handler`. A `Timer` becomes a coroutine that sleeps. An `animate` block or a transition becomes `lv::animate` in a task.
7. **Build and run.** Build a page as soon as it is written. Write each page as a PNG file with `lv::write_png`, so that you and an agent can look at the result without a display.
8. **Compare a screenshot.** Take a screenshot of the Slint app at the same size, and put it beside the port. Keep the port's picture as a reference, and let a test compare every page on each push, as `tests/ports_printer.cpp` does.
9. **Check the memory.** Mount each page on the target and read `app.heap()`. Compare it with the heap you can spare.
10. **Record the gaps.** Write down each thing you left out or changed, and why, as the port's README does under "What differs from the original". Do not hide a gap behind a workaround that has no name.

## Decisions you must make

| Decision | Options | Our advice |
|---|---|---|
| Reusable parts | C++ functions now, or wait for components with state and slots | Write functions, and pass the state in as an argument. A part that takes children has no counterpart yet |
| Lists | Fixed children, `lv::each` over an `lv::list`, or wait for a list that recycles its rows | `lv::each` for a list of up to a few dozen rows that changes. A long scrolling list that recycles its rows is a gap: decide before you start |
| Translations | One language, one build for each language, or a switch at run time | `@tr` has no counterpart. Keep the texts of a build in one header. A switch at run time is a gap |
| Fonts, sizes and glyphs | Compile-time fonts with `lv::typeface`, or `lv::scaled_font` for a size known at run time | List the sizes the design uses, and the characters it shows. Each size costs compile time and flash: see [Fonts](fonts.md). `lv::scaled_font` needs `LVGL26_TTF` |
| Images | Compile-time pictures with `lv::image_file`, or files decoded on the device with `lv::encoded_image` | Icons and small art at compile time. Photos stay files, and a decoder is a CMake option: `LVGL26_PNG` or `LVGL26_JPEG`. Convert SVG to PNG at the size shown |
| Memory budget | Measure with `app.heap()` after each mount | Measure on the target, and keep headroom. `app.heap()` reads zero when LVGL uses another allocator. A label or a button takes about 200 bytes of heap on a Cortex-M4 |
| Page switching | Mount a page when shown and delete it when it goes, or mount all pages and hide them | Mount and delete: a page that is not shown has no widgets. Slint's `if` does this. `lv::one_of` does it for a root, in one memory: see [Where a tree lives](mounting.md). A page moved off screen keeps its widgets on the heap |
| Themes | One theme, or several that switch at run time | One theme. A theme cannot change at run time yet, so Slint's other printer themes are left out |
| Assets with an unclear licence | Use them, replace them, or leave them out | Replace them, and list each asset and its licence in a NOTICE file. A font's licence must allow its glyphs in a device |
| Something is missing | Change the design, leave it out, use `lv::raw(...)`, or wait | Look it up first. Then change the design, or leave it out and record it. Use `lv::raw(...)` for one reasoned exception |

Numbers for the memory row come from `docs/research/memory.md`.

## Not there yet

- **A `ListView` over a long model.** `lv::each` builds a row for each row of an `lv::list`, up to its capacity. A list that recycles its rows while it scrolls is planned. The printer port shows its queue with `lv::each`: a change of a job touches the widgets of that job only.
- **Components with their own state and slots.** `@children` has no counterpart. They are planned.
- **A style that follows a color state.** A theme cannot change at run time yet, so Slint's four printer themes are not ported.
- **Gradients.** A gradient is vertical or horizontal. Diagonal and multi-stop gradients are missing.
- **Translations.** `@tr` has no counterpart yet.

## Next

- [Quick start](quick-start.md) builds a screen in one file.
- [Themes](themes.md) covers tokens and the look of widgets.
- [Port walkthrough: the Slint printer panel](port-slint-printer.md) shows pages that slide in, a queue and bound controls, with the decisions made.
- The code of that port is `examples/ports/slint/printer`.
