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
| Service | Water hardness, cup counter, a descaling reminder | A state per setting, `lv::effect` for what lies outside the screen |

The pages follow one state, `enum class step { drinks, cup, brewing, enjoy, service }`, through `lv::shown` with a slide.

## The steps

Each step is a whole program that a test builds, and that writes the picture the page shows. A step adds one idea, and the page shows only what changed.

| Step | Adds | Files |
|---|---|---|
| 1. A screen | The drinks as a tree, one state, a label bound to it | `main.cpp` |
| 2. Values that say what they are | Strength, size and beans; a computed dose; a refused `lv::state<int>` as its compile-fail message | `main.cpp` |
| 3. Pages | `lv::shown` over the steps, with a slide | `main.cpp` |
| 4. Time | The brewing task: progress, the time left, cancel | `main.cpp` |
| 5. A look of its own | A token file and looks in place of Signal's theme | `main.cpp`, `tokens.json` |
| 6. Split into files | The same program in the layout below; nothing else changes | the layout |
| 7. A service page | One more screen in its own file, and the settings file | the layout |

Steps 1 to 5 are one file each, growing. Step 6 is the turning point of the tutorial: the reader sees the one file become the layout, with the same picture.

## The layout of step 6 and 7

The owner's layout, unchanged:

```
coffee/
  main.cpp            starts the app, mounts the shell
  shell.cpp .hpp      the frame, lv::shown of the pages
  state.hpp           what several screens share: the step, the drink
  screens/
    drinks.cpp .hpp   a screen and its own states
    cup.cpp .hpp
    brewing.cpp .hpp
    enjoy.cpp .hpp
    service.cpp .hpp  step 7
  parts/
    tile.hpp          functions that return a tree
  design/
    tokens.json       the design tokens
    design.hpp        names the token file, the looks
  platform/
    window.cpp        the only file that knows SDL
  lvgl26.json         project settings (step 7)
  CMakeLists.txt
```

- A screen is an `lv::page` of its own file, `lv::page_of<ui>`, which `shell.cpp` names with `extern const lv::page`. No size is written.
- The brewing task is spawned in the brewing page's `.on_shown`, with the page's root as owner, so it ends when the page goes.
- `design/` holds no font files: see "What waits" below.

## Tests

- A picture per step, written by the step's program and compared byte for byte with the one the page shows, as `quick_start_picture` does.
- Steps 6 and 7 also compare each page: the drinks, the cup, brewing at a fixed time, enjoy, service.
- A compile-fail test per mistake the tutorial shows, with its whole message on the page.
- `lvgl26_skip_analyzer(examples/coffee)`: the analyzer job is near its limit.

## What waits, and the form the tutorial uses today

| Gap | The form today |
|---|---|
| Fonts drawn once, one file per font (`design/font_title.cpp`): today each file draws the fonts it uses | Signal's theme for steps 1 to 4, and the looks of step 5 without drawn fonts. Fonts join `design/` when the feature exists |
| `lv::run<fn>`, a task that a tree declares (`task-handles.md`) | `lv::spawn(brew(page), page)` in `.on_shown` |
| A time such as `0:25` in one binding | The seconds left: "25 s" |
| The layout check of `extendable-1` | The layout by hand; the check comes with the fonts, since a font drawn in a screen's file would fail it |

## Questions for the owner

1. Is the example of "An application that grows" the tutorial's step 7, or a small program of its own? One program means one set of pictures and less code to keep in step; the plan says "of its own".
2. 480 x 320, or the 320 x 240 of the quick start? The larger screen fits a list beside its detail.
3. Each step a whole copy in `examples/coffee/1-screen/` to `7-service/`, or one program with the steps as its history? Copies build and test each step; they cost seven targets in CI.
4. Signal first and a look of its own in step 5, or the reader's own tokens from step 1?
5. Water temperature as a setting: an absolute temperature in °C is a quantity point in mp-units, `point<deg_C>(92)`. Whether `lv::state` holds one is not checked yet. Check it first, or show the hardness and the counter only?
