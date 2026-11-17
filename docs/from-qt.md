# Coming from Qt Quick

You write QML or use Qt Quick for MCUs, and want the same kind of screen in C++.
This page maps what you know to what lvgl26 has today.

A screen is a tree again, but it is one `constexpr` C++ value, not a document that a runtime reads.
There is no second language, no code generator and no engine: the C++ compiler checks the screen.

## Concepts

| Qt Quick | lvgl26 |
|---|---|
| A `.qml` file with an `Item` tree | A `constexpr` tree: `lv::column({...})[children]`. Properties go in `()`, children in `[]` |
| `Column`, `Row` | `lv::column`, `lv::row`. `lv::group` is a plain box with no padding of its own |
| `anchors.centerIn: parent`, `x`, `y` | `.align = "center"`, `.x`, `.y` with a unit: `.x = 12 * px` |
| `Text { text: ... }` | `lv::label({.text = ...})` |
| `Button { onClicked: ... }` | `lv::button({.on_clicked = &handler})` |
| A property binding `text: speed` | `.text = lv::bind(speed)` |
| A binding with an expression | `lv::computed`, bound the same way |
| A `Q_PROPERTY` of a model object | An `lv::state`. It has a type, and a measure has a unit |
| A `Repeater` over a model | `lv::each(tracks, &track::id)[row template]` over an `lv::list`. See [Lists](lists.md) |
| `Timer { onTriggered: ... }` | A coroutine: `co_await lv::sleep(100 * ms)` in an `lv::task` |
| `Connections { onSomething: ... }` | A coroutine that waits: `co_await button.next().clicked` |
| `NumberAnimation`, `Behavior`, easing | `lv::animate(widget, {.value = {0, 100}}, 300 * ms, lv::ease::in_quad)` |
| `State` and `Transition` | `lv::match` on a state, and animations in a task |
| `Image { source: "a.png" }` | `lv::image_file<a_png>`. The compiler decodes the PNG, and the device needs no decoder |
| `FontLoader` and a font family | `lv::typeface`. The compiler makes the font from a TrueType file |
| `Loader`, `StackView` | `lv::mount<tree>` creates a page and its owner deletes it. See the printer port for a page switcher |
| A Qt Quick Controls style | A theme: `lv::theme({...})` gives each kind of widget its look |
| Colors and sizes kept in one design file | A W3C Design Tokens file, read at compile time. A style takes tokens only |
| A property of the wrong type | A compile error that says what the property takes |
| Work on another thread | `lv::ui_scheduler::post()` on hosted systems |

## What you get that QML does not give

- **Units in the type.** `120 * km / h` is not `120 * px`, and a speed in metres per second that does not fit an integer is refused.
- **Errors at build time.** A wrong property, a misspelled enumerator name or a missing unit stops the build, where QML shows a warning at run time.
- **No runtime.** A tree is built from constants, so the device holds no QML engine and no parser.
- **One licence.** lvgl26 and LVGL are MIT. Nothing in the build is sold.

## Things that are the same

- A screen has a tree, and you change it by changing values.
- Values follow state: change the state and the widgets that are bound to it update.
- A theme sets the look of every widget of a kind, and a node still has the last word.

## How to port, step by step

These steps are for a person or an agent.
The [walkthrough of the printer panel](port-slint-printer.md) ports a Slint demo, and the steps are the same.

1. **Inventory the source UI.** List the QML files and their components, and the C++ objects they read. List the models behind `ListView` and `Repeater`, the `State` and `Transition` items, the `qsTr` texts, the fonts, the images and the styles. Check each against the concept table above and the list of what is not there yet.
2. **Make the decisions below.** Do it before you write code. Translations, lists and images change the shape of the port.
3. **Make the token file.** Copy the colors, sizes, radii, durations and easing curves from where your QML keeps them into a W3C Design Tokens file. Add each font as a type style. A style takes tokens only.
4. **Port page by page, one file for each page.** A page is one constant tree and one function that mounts it. Start with the page that needs the fewest other parts. A QML component becomes a C++ function that returns a piece of tree.
5. **Bind state.** A value that a C++ object gives to QML becomes an `lv::state`. A binding with an expression becomes an `lv::computed`. Keep a page's own state in the page's file, and shared state in one header.
6. **Add behaviour.** An `onClicked` handler becomes a function, named as `.on_clicked = &handler`. A `Timer` becomes a coroutine that sleeps. A `NumberAnimation` or a `Behavior` becomes `lv::animate` in a task.
7. **Build and run.** Build a page as soon as it is written. Write each page as a PNG file with `lv::write_png`, so that you and an agent can look at the result without a display.
8. **Compare a screenshot.** Take a screenshot of the Qt app at the same size, and put it beside the port. Keep the port's picture as a reference, and let a test compare every page on each push, as `tests/ports_printer.cpp` does.
9. **Check the memory.** Mount each page on the target and read `app.heap()`. Compare it with the heap you can spare.
10. **Record the gaps.** Write down each thing you left out or changed, and why, as the port's README does under "What differs from the original". Do not hide a gap behind a workaround that has no name.

## Decisions you must make

| Decision | Options | Our advice |
|---|---|---|
| Reusable parts | C++ functions now, or wait for components with state and slots | Write functions, and pass the state in as an argument. A part that takes children has no counterpart yet |
| Lists | Fixed children, `lv::each` over an `lv::list`, or wait for a list that recycles its rows | `lv::each` for a list of up to a few dozen rows that changes. A long scrolling list that recycles its rows is a gap: decide before you start |
| Translations | One language, one build for each language, or a switch at run time | `qsTr` has no counterpart. Keep the texts of a build in one header. A switch at run time is a gap |
| Fonts, sizes and glyphs | Compile-time fonts with `lv::typeface`, or `lv::scaled_font` for a size known at run time | List the sizes the design uses, and the characters it shows. Each size costs compile time and flash: see [Fonts](fonts.md). `lv::scaled_font` needs `LVGL26_TTF` |
| Images | Compile-time pictures with `lv::image_file`, or files decoded on the device with `lv::encoded_image` | Icons and small art at compile time. Photos stay files, and a decoder is a CMake option: `LVGL26_PNG` or `LVGL26_JPEG`. Convert SVG to PNG at the size shown |
| Memory budget | Measure with `app.heap()` after each mount | Measure on the target, and keep headroom. `app.heap()` reads zero when LVGL uses another allocator. A label or a button takes about 200 bytes of heap on a Cortex-M4 |
| Page switching | Mount a page when shown and delete it when it goes, or mount all pages and hide them | Mount and delete: a page that is not shown has no widgets. A `Loader` that unloads its item does the same |
| Themes | One theme, or several that switch at run time | One theme. A theme cannot change at run time yet |
| Assets with an unclear licence | Use them, replace them, or leave them out | Replace them, and list each asset and its licence in a NOTICE file. A font's licence must allow its glyphs in a device. Assets of a Qt example keep that example's licence: Qt for MCUs examples are commercial, and some Qt 6 examples are BSD-3-Clause |
| Something is missing | Change the design, leave it out, use `lv::raw(...)`, or wait | Look it up first. Then change the design, or leave it out and record it. Use `lv::raw(...)` for one reasoned exception |

Numbers for the memory row come from `docs/research/memory.md`.

## Not there yet

- **Components.** A repeated part is a C++ function that returns a piece of tree. Components with their own state, parameters and slots are planned.
- **A `ListView` over a long model.** `lv::each` over an `lv::list` builds a row for each row, up to the list's capacity. A list that recycles its rows while it scrolls is planned.
- **A style for one part in one state.** The knob of a slider while pressed has no form yet. Themes set a part or a state, not both.
- **Translations.** `qsTr` has no counterpart yet.
- **Qt Design Studio.** There is no visual editor. The workflow is text and a screenshot: `lv::write_png` and the agent tools.

## Next

- [Quick start](quick-start.md) builds a screen in one file.
- [Themes](themes.md) shows how a look is made once and used by every widget.
- [Port walkthrough: the Slint printer panel](port-slint-printer.md) shows a whole port, with its decisions.
- [examples/ports](../examples/README.md) lists the examples that other frameworks inspired.
