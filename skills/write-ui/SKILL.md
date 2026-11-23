---
name: write-ui
description: Use when writing or changing application code that uses lvgl26: a screen, a widget tree, a style, a theme, a list, a font, a picture, a task, input, or a program's main. Also use when an lvgl26 build fails, for example with "is not a member of 'lv'" or "needs #include". Points to the page for each feature and to the rules that the compiler and the build enforce.
version: 0.1.0
---

# Writing a user interface with lvgl26

lvgl26 is a C++26 framework over LVGL, in namespace `lv`.
The pages under [docs/](../../docs/README.md) are the source; this skill holds no copy of them.
Read the page of a feature before writing code that uses it.

## Start here

1. Read [docs/quick-start.md](../../docs/quick-start.md): a state, a tree, a mount and a `main`, in one file that the tests build.
2. Copy its shape: [examples/quick_start/main.cpp](../../examples/quick_start/main.cpp) and its [CMakeLists.txt](../../examples/quick_start/CMakeLists.txt).
3. Read [docs/rules.md](../../docs/rules.md), "Writing good lvgl26 code": the rules every page teaches, each with an id. Follow them, and cite the id when a review or a commit applies one.
4. For a feature beyond it, open that feature's page from the list below.

The compiler is GCC 16.2 or newer, with `-std=c++26 -freflection`. No other compiler builds the code.

## Which header

`<lv/lv.hpp>` is the core. A feature that costs compile time has a header of its own, and `<lv/all.hpp>` holds them all.
[docs/which-header.md](../../docs/which-header.md) lists every name with its header, and the error texts that mean a header is missing.

- Include `<lv/lv.hpp>` and the header of each feature the file uses. Use `<lv/all.hpp>` only for a first program.
- When the build prints `main.cpp:12: lv::list needs #include <lv/list.hpp>`, or the compiler `lv::list is in <lv/list.hpp>: add #include <lv/list.hpp>`, add that include. Do not look for another cause.
- When GCC says `'list' is not a member of 'lv'`, look the name up on that page.

## The page for each feature

| To write | Read |
|---|---|
| Values that widgets follow, values made from them, an action that follows them | [docs/state.md](../../docs/state.md) |
| Mounting a tree, unmounting, one of several trees in a slot | [docs/mounting.md](../../docs/mounting.md) |
| A look for all widgets of a kind | [docs/themes.md](../../docs/themes.md) |
| A look named by the design, from the token file | [docs/looks.md](../../docs/looks.md) |
| Rows that change | [docs/lists.md](../../docs/lists.md) |
| The same part many times | [docs/repetition.md](../../docs/repetition.md) |
| A choice among options, from an enum | [docs/options.md](../../docs/options.md) |
| Fonts | [docs/fonts.md](../../docs/fonts.md) |
| Pictures | [docs/images.md](../../docs/images.md) |
| A pointer, the loop of a program, a window | [docs/input.md](../../docs/input.md) |
| A function that returns a piece of tree | [docs/concepts.md](../../docs/concepts.md) |
| What the program may use, and the library's strictness | [docs/settings.md](../../docs/settings.md), and the skill `settings` |
| Completion and errors in the editor | [docs/editor-support.md](../../docs/editor-support.md) |
| Code that comes from Qt, Slint or TouchGFX | [docs/from-qt.md](../../docs/from-qt.md), [docs/from-slint.md](../../docs/from-slint.md), [docs/from-touchgfx.md](../../docs/from-touchgfx.md) |

Every widget, property, enumerator and style member is in [docs/reference/](../../docs/reference/), as data that the library prints about itself.
Look a name up there; do not guess it from LVGL's C names.

## Rules the code must follow

- A program starts LVGL with `lv::application` and gets its screen from an `lv::display`. It calls no `lv_init`, `lv_display_*`, `lv_tick_*` or `lv_timer_handler`.
- Application code names no `lv_` function, `lv_..._t` type or `LV_` name. `raw()` is the one door to the C API, for what lvgl26 has no form for.
- An LVGL enumerator is a name in quotes: `.align = "center"`.
- A tree and a `view.set` call the setters in the order of the properties, except for the few that LVGL needs in another (the map of a button matrix first, its controls and the cursor of a text area last): [detail/setter_order.hpp](../../include/lv/detail/setter_order.hpp) lists them. Write the properties in the order of the group rule below; nothing else is needed.
- Properties go in four groups, each alphabetical: the `id`, the properties (`size`, `text`, ...), the styles (`style`, `style_pressed`), the handlers (`on_clicked`). A list out of order is a compile error that names one field; `python3 tools/order_properties.py <files>` puts the properties of existing code in order.
- A style takes design tokens. Add a missing token to the project's token file; `lv::raw(...)` marks a reasoned exception.
- A measure is a quantity with a unit (`120 * px`, `120 * km / h`), a count is `lv::counter`, anything else a named type. A plain integer in a state is refused.
- Only `lv::mount` and `lv::owned` own widgets. A view never deletes one.
- Work that goes on while a part of the screen exists is started with `lv::spawn(task, owner)`: it ends when the owner is deleted.
- No macros and no `#if` for settings: the settings file decides.
- Do not put lvgl26's headers into a precompiled header. GCC 16.2 then silently drops precondition checks.

## When the build fails

lvgl26 turns a mistake into a compile error whose message says how to fix it.

1. Read the first error, and the `note:` lines under it: the message is often there.
2. Do what it says before trying another approach. A message that names a settings key, a token, a header or a property is exact.
3. A property or enumerator that "does not exist" is usually a C name: find the lvgl26 name in the reference data.
4. If a message is the compiler's own and names no fix, check [docs/which-header.md](../../docs/which-header.md) first, then the page of the feature.

## Checking the work

- Build and run the program's tests; a tree that compiles is already checked against the widgets' properties.
- For a picture of the screen, write one with `lv::write_png` from `<lv/png.hpp>`, as the quick start does.

## What not to do

- Never copy LVGL C examples into application code: port them, as [docs/port-lvgl-examples.md](../../docs/port-lvgl-examples.md) shows.
- Never silence an lvgl26 error with a cast or with `raw()`.
- Never invent a property, an enumerator or a token: the reference data and the token file list them all.
