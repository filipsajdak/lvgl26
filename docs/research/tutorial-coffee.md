# Tutorial design: a coffee machine, and a project split into files

A design for the tutorial `docs/tutorial.md` and for the example of the guide "An application that grows", written on 2026-11-23 before any code.
Marks follow [README.md](README.md). The owner's decisions it builds on are in [documentation.md](documentation.md): "Examples, tutorial, reference, pictures" and "Projects split into files". No example is written until the reformat of all examples is merged.

## What the reader builds

A coffee machine's screen at 480 x 320, landscape. The reader picks a drink, sets its strength and size, starts it, and watches it brew. A service page holds the machine's settings.

| Page | What it shows | lvgl26 it teaches |
|---|---|---|
| Drinks | Four drinks, the chosen one marked, its price | A state of an enum, `lv::options_of`, a list of constant records, `lv::bind` |
| Your cup | Strength from 1 to 5, the size in millilitres, the beans it takes | Typed values: a struct of one member, an mp-units quantity, `lv::computed` |
| Brewing | A ring that fills, the seconds left, a cancel button | A task owned by its page, `lv::animate` with an easing token, `co_await` of a click or a time |
| Enjoy | The drink, ready | A page that follows a state; `.on_shown` |
| Service | Water hardness, cup counter, a descaling reminder | A state per setting, `lv::effect` for what lies outside the screen. No water temperature: see "What waits" |

The pages follow one state, `enum class step { drinks, cup, brewing, enjoy, service }`, through `lv::shown` with a slide.

## The steps

Each step is a whole program that a test builds, and that writes the picture the page shows. A step adds one idea, and the page shows only what changed.

| Step | Adds | Files |
|---|---|---|
| 1. A screen | The drinks as a tree, one state, a label bound to it; the reader's own token file and looks | `main.cpp`, `tokens.json` |
| 2. Values that say what they are | Strength, size and beans; a computed dose; a refused `lv::state<int>` as its compile-fail message | `main.cpp` |
| 3. Pages | `lv::shown` over the steps, with a slide | `main.cpp` |
| 4. Time | The brewing task: progress, the time left, cancel | `main.cpp` |
| 5. A design system you can reuse | Signal's theme for every widget, beside the program's own looks | `main.cpp`, `tokens.json` |
| 6. Split into files | The same program in the layout below; nothing else changes | the layout |
| 7. A service page | One more screen in its own file, and the settings file | the layout |

Steps 1 to 5 are one program each, growing. Each step is a whole program in `examples/coffee/1-screen/`, `2-values/`, `3-pages/`, `4-time/`, `5-design/`, `6-files/` and `7-service/`, a small CI target with its picture. Step 6 is the turning point of the tutorial: the reader sees the one file become the layout, with the same picture.

Signal's theme matches a widget by its exact class (`include/lv/detail/theme_styles.hpp`, `measured` 2026-11-25 by reading the code). So it draws the screen, the buttons and the ring of the machine, and not its sliders or its drop-down list. Nor does it reach a page: `lv::group` removes every style of its widget, the theme's too (`start_widget` in `include/lv/tree.hpp`), so Signal's spacing of a box never applies there. The first pictures of step 5 had pages with no padding and no gap (`measured` 2026-11-26). Step 5 drops the looks that Signal covers. Its token file keeps the column layout and the spacing of a page, the title, the price and the sliders.

## The layout of step 6 and 7

The owner's layout, unchanged:

```
coffee/
  main.cpp            starts the app, mounts the shell, writes the pictures
  shell.cpp .hpp      the frame, lv::shown of the pages, the size of the screen
  state.hpp           what several screens share: the step, the drink and its price, the cups made
  screens/
    drinks.cpp .hpp   a screen and its own states
    cup.cpp .hpp
    brewing.cpp .hpp
    enjoy.cpp .hpp
    service.cpp .hpp  step 7
  parts/
    title.hpp         functions that return a tree
    go_to.hpp
  design/
    tokens.json       the design tokens
    design.hpp        names the token file, the looks
    theme.hpp         Signal's theme, for the files that make a display
    fonts.hpp         the fonts as styles to merge: title_text, body_text
    font_text_title.cpp   the only file that draws the font of text.title
    font_text_body.cpp    the only file that draws the font of text.body
    fonts/            a copy of Barlow and its licence
  platform/
    window.cpp        the only file that knows SDL
  lvgl26.json         project settings (step 7)
```

- A screen is an `lv::page` of its own file, `lv::page_of<drinks>`, which `shell.cpp` names with `extern const lv::page drinks_page`. No size is written. A screen's header declares only its page.
- The brewing task is spawned in the brewing page's `.on_shown`, with the page's root as owner, so it ends when the page goes. The hook is named with the page: `lv::page_of<brewing, {.on_shown = &start_brewing}>`.
- The picture does not change between step 5 and step 6: the test of step 6 compares its picture with step 5's. The memory does. The shell cannot see the size of a page of another file, so the pages take one block of two slots from LVGL's heap at mount. Step 5 keeps them in the tree.
- `main.cpp` draws without a window, as the steps before it do. `platform/window.cpp` is the same program in a window (`LVGL26_BUILD_WINDOW_EXAMPLES`, as the printer port). The pages are one object library, so the window compiles none of them again.
- The CMake rules of a split step are two functions in `examples/coffee/CMakeLists.txt`: `coffee_files` (the pages, the program, the window) and `coffee_picture`.
- Step 7's `lvgl26.json` forbids threads and floating point. It cannot forbid allocation, which `lv::spawn` needs for a task's frame. Nor can it forbid files, which `lv::write_png` needs (`measured` 2026-11-25: a scratch copy that forbids files stops `main.cpp` at `lv::write_png`).
- Step 7's softener follows the water's hardness through an `lv::effect`. The service page starts it in its `.on_shown`; `start()` does nothing while the effect runs.
- The titles and the text are in the reader's own typeface, the typography tokens `text.title` and `text.body` in Barlow (owner, 2026-11-25). Step 5 draws them in its one file. Step 6 moves each into `design/font_<name>.cpp`, and `design/fonts.hpp` names them with `lv::font_of`.
- The body font is on each label and on the buttons' text, through the `go_to` part (owner, 2026-11-25, option A). It is not on the drop-down lists. A list keeps LVGL's font, so its arrow is drawn (see the gap below). A text part was not used: no public concept says what `.text` takes, a literal or a binding, and a part's parameters take a concept. So each label merges a named style, `.style = body_text`.
- A look cannot carry that font in a split program. A look draws a font through the design's typeface (`detail::font_of` in `include/lv/look.hpp`), and a design header that every file includes must hold none. So the fonts are styles of their own, merged with a look where a label has one: `lv::look<"title"> | title_text`.
- The includes are relative to the file that names them (`#include "../state.hpp"`). The build's font and include checks follow a quoted include only beside that file (`program_files` in `tools/check_fonts.py`, `provided` in `tools/check_includes.py`). Through an include directory the font check passed without the font file (`measured` 2026-11-25).

## Tests

- A picture per step, written by the step's program and compared byte for byte with the one the page shows, as `quick_start_picture` does. Step 6 compares with step 5's picture.
- Step 7 writes two pictures. `coffee-7.png` is the finished machine as it starts, the picture the README will show. `coffee-7-service.png` is its service page twelve cups later.
- A compile-fail test per mistake the tutorial shows, with its whole message on the page.
- `lvgl26_skip_analyzer(examples/coffee)`: the analyzer job is near its limit.

## What waits, and the form the tutorial uses today

| Gap | The form today |
|---|---|
| A drawn font holds no LVGL symbol, and no token names LVGL's own font. A font that reaches a drop-down list turns its arrow into a box: the arrow takes the font of the list's indicator part (`lv_dropdown.c:1088`), which inherits it, and LVGL's theme sets a font on the screen only (`lv_theme_default.c:250`) (`measured` 2026-11-25, by reading the code) | Signal's theme without its typefaces in step 5. The reader's body font is on the labels and the buttons' text, and not on the lists. The gap is `font-symbols` in `docs/migration/coverage.json` |
| `lv::run<fn>`, a task that a tree declares (`task-handles.md`) | `lv::spawn(brew(page), page)` in `.on_shown` |
| A time such as `0:25` in one binding | The seconds left: "25 s" |
| The layout check of `extendable-1` | The layout by hand. Fonts drawn once are built, so the check is next, with the program of "An application that grows" |
| A state of an absolute temperature: `lv::state water{mp_units::point<deg_C>(92)}` is refused with "lv: a state holds a quantity ... This state's type is mp_units::quantity_point<...degree_Celsius..., int>" (`measured` 2026-11-23, GCC 16.2.0, `-fsyntax-only`). A `delta<deg_C>` is accepted, but it is a difference, not a temperature | None: the service page has no water temperature |

## The owner's decisions (2026-11-23)

1. "An application that grows" gets a small program of its own, in the layout, which runs the layout check. The tutorial links to it.
2. The screen is 480 x 320.
3. Each step is a whole program, `examples/coffee/1-screen/` to `7-service/`: seven small CI targets, each with its picture.
4. The reader's own tokens from step 1. Signal appears in a late step, as a design system you can reuse.
5. Water temperature was checked: `lv::state` refuses an mp-units quantity point. The tutorial leaves it out, and the gap is in the table above. No workaround.
