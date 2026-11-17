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

## Not there yet

- **A designer.** There is no visual editor. A design comes from tokens and a screenshot.
- **Text resources and languages.** A text in the tree is the text you write. Translations have no counterpart yet.
- **Lists and scroll wheels from a model.** A list is written as children for now.
- **A real board.** The microcontroller slice runs on a Cortex-M4 in an emulator. No real board has run it yet.

## Next

- [Quick start](quick-start.md) builds a screen in one file.
- [Themes](themes.md) and [fonts](fonts.md) cover the look.
- The microcontroller slice, `examples/mcu`, shows a frame, a band and a panel function.
