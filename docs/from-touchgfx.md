# Coming from TouchGFX

You use TouchGFX Designer and its generated code, and want to write the screens as C++.
This page maps what you know to what lvgl26 has today. It describes concepts, not every detail of TouchGFX.

TouchGFX has a Designer that writes code, and a Model, View and Presenter split. In lvgl26 you write the screen yourself, as a `constexpr` tree, and a typed state plays the part of the Model.

## Concepts

| TouchGFX | lvgl26 |
|---|---|
| A screen made in Designer | A `constexpr` tree: `lv::column({...})[children]` |
| `Box`, `Container` | `lv::box`, `lv::group`, `lv::column`, `lv::row` |
| `Image` | `lv::image({.src = lv::image_file<a_png>})`. The compiler decodes the PNG |
| `TextArea` and a text resource | `lv::label({.text = ...})` |
| `Button` and an interaction | `lv::button({.on_clicked = &handler})` |
| `Slider`, `Gauge`, `CircularProgress` | `lv::slider`, `lv::arc`, `lv::bar` and the other LVGL widgets |
| The Model and its Presenter | An `lv::state`, and an `lv::computed` for a value made from states |
| A `ScrollList` of a few dozen items | `lv::each(items, &item::id)[row template]` over an `lv::list`. See [Lists](lists.md) |
| A screen transition | A page is mounted by `lv::mount<tree>`, and animated with `lv::animate` |
| An Animation or an easing equation | `lv::animate(widget, {...}, time, lv::ease::out_back)` |
| A timer tick (`handleTickEvent`) | A coroutine: `co_await lv::sleep(20 * ms)` in an `lv::task` |
| A font and its Typography | `lv::typeface`: the compiler makes the font from a TrueType file |
| A color or size set in code | A design token. A style takes tokens only |
| The framebuffer strategy, partial buffers | `lv::frame` for the whole picture and `lv::band` for a few lines at a time |
| The HAL and the display driver | A panel function: `lv::display display{app, frame, &changed}` |

## What you get that TouchGFX does not give

- **No tool in the loop.** The screen is a text file, so it is reviewed, diffed and written by an agent like any code.
- **Not tied to one vendor.** The code runs on any target that GCC 16 and LVGL support. TouchGFX's terms tie it to ST hardware, so check the terms of your version.
- **Mistakes stop the build.** A wrong property or a missing unit is a compile error with a message that says how to fix it.
- **MIT licence.** lvgl26 and LVGL are free to use in a product.

## Memory, honestly

TouchGFX plans all its memory in advance. LVGL uses a heap for its widgets, and lvgl26 keeps its own use of it small.
A constant text stays in flash. A constant style is shared by every node that repeats it.
The heap of a mounted screen is measured, not guessed: see `docs/research/memory.md` for the numbers on a Cortex-M4.
A fixed memory region for one screen is not built yet.

## How to port, step by step

These steps describe concepts, not every detail of TouchGFX.
The [walkthrough of the printer panel](port-slint-printer.md) ports a Slint demo, and the steps are the same.

1. **Inventory the source UI.** List the screens, and the widgets and containers on each. List the Model and what each Presenter reads from it, the text resources and languages, the fonts, the images, the interactions and the transitions. Check each against the concept table above and the list of what is not there yet.
2. **Make the decisions below.** Do it before you write code. Languages, lists and memory change the shape of the port.
3. **Make the token file.** Copy the colors, sizes and durations that your screens use into a W3C Design Tokens file. Add each font as a type style. A style takes tokens only.
4. **Port screen by screen, one file for each page.** A page is one constant tree and one function that mounts it. Start with the screen that needs the fewest other parts. A container you reuse becomes a C++ function that returns a piece of tree.
5. **Bind state.** A value that the Model gives to a Presenter becomes an `lv::state`. A value made from others becomes an `lv::computed`. Keep a page's own state in the page's file, and shared state in one header.
6. **Add behaviour.** An interaction becomes a function, named as `.on_clicked = &handler`. A tick handler becomes a coroutine that sleeps. An animation becomes `lv::animate` in a task.
7. **Build and run.** Build a page as soon as it is written. Write each page as a PNG file with `lv::write_png`, so that you and an agent can look at the result without a display.
8. **Compare a screenshot.** Take a screenshot of the TouchGFX screen at the same size, and put it beside the port. Keep the port's picture as a reference, and let a test compare every page on each push, as `tests/ports_printer.cpp` does.
9. **Check the memory.** TouchGFX plans its memory in advance, and LVGL has a heap. Mount each page on the target and read `app.heap()`, so that you know the number.
10. **Record the gaps.** Write down each thing you left out or changed, and why, as the port's README does under "What differs from the original". Do not hide a gap behind a workaround that has no name.

## Decisions you must make

| Decision | Options | Our advice |
|---|---|---|
| Reusable parts | C++ functions now, or wait for components with state and slots | Write functions, and pass the state in as an argument. A part that takes children has no counterpart yet |
| Lists | Fixed children, `lv::each` over an `lv::list`, or wait for a list that recycles its rows | `lv::each` for a list of up to a few dozen rows that changes. A long scrolling list that recycles its rows is a gap: decide before you start |
| Translations | One language, one build for each language, or a switch at run time | Text resources and languages have no counterpart. Keep the texts of a build in one header. A switch at run time is a gap |
| Fonts, sizes and glyphs | Compile-time fonts with `lv::typeface`, or `lv::scaled_font` for a size known at run time | List the sizes the design uses, and the characters it shows. Each size costs compile time and flash: see [Fonts](fonts.md). `lv::scaled_font` needs `LVGL26_TTF` |
| Images | Compile-time pictures with `lv::image_file`, or files decoded on the device with `lv::encoded_image` | Icons and small art at compile time. Photos stay files, and a decoder is a CMake option: `LVGL26_PNG` or `LVGL26_JPEG`. Convert SVG to PNG at the size shown |
| Memory budget | Measure with `app.heap()` after each mount | Measure on the target, and keep headroom. `app.heap()` reads zero when LVGL uses another allocator. A label or a button takes about 200 bytes of heap on a Cortex-M4 |
| Page switching | Mount a page when shown and delete it when it goes, or mount all pages and hide them | Mount and delete: a page that is not shown has no widgets. Animate the change with a task |
| Themes | One theme, or several that switch at run time | One theme. A theme cannot change at run time yet |
| Assets with an unclear licence | Use them, replace them, or leave them out | Replace them, and list each asset and its licence in a NOTICE file. A font's licence must allow its glyphs in a device. Read ST's terms for any asset that came with TouchGFX |
| Something is missing | Change the design, leave it out, use `lv::raw(...)`, or wait | Look it up first. Then change the design, or leave it out and record it. Use `lv::raw(...)` for one reasoned exception |

Numbers for the memory row come from `docs/research/memory.md`.

## Not there yet

- **A designer.** There is no visual editor. A design comes from tokens and a screenshot.
- **Text resources and languages.** A text in the tree is the text you write. Translations have no counterpart yet.
- **A scroll list over an update callback.** `lv::each` builds every row of an `lv::list`. A list that keeps a few rows and refills them as they scroll in is planned.
- **A real board.** The microcontroller slice runs on a Cortex-M4 in an emulator. No real board has run it yet.

## Next

- [Quick start](quick-start.md) builds a screen in one file.
- [Themes](themes.md) and [fonts](fonts.md) cover the look.
- The microcontroller slice, `examples/mcu`, shows a frame, a band and a panel function.
- [Port walkthrough: the Slint printer panel](port-slint-printer.md) shows a whole port, with its decisions.
