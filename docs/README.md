# lvgl26 documentation

lvgl26 is a C++26 framework over LVGL for embedded user interfaces. The compiler checks a screen before it runs, and your code calls no C.

## The pages

The groups are the layers of the framework, in the order of the sidebar. `docs.json` holds the sidebar. The [README](../README.md) shows the main features in short, and `docs/reference/` holds the widgets, properties and settings as data.

### Start

| Page | What it gives you |
|---|---|
| [Quick start](quick-start.md) | A speedometer screen in ten minutes: a tree, a typed value, a picture |
| [Which header](which-header.md) | The header of every name, and what to do when the build says one is missing |
| [Writing good lvgl26 code](rules.md) | The rules every page teaches, each with an id, its example, its check and its measurement |

### Core ideas

| Page | What it gives you |
|---|---|
| [The tree and its widgets](tree.md) | A screen as one constant tree: children as arguments, properties generated from LVGL, their order |
| [State and bindings](state.md) | Values that widgets follow, values made from them, and actions that follow them |
| [Views and ids](views.md) | A typed handle to one widget, reached by its id; who owns a widget; the untyped way |
| [Values and units](values.md) | Typed property values, measures with mp-units, angles in the unit LVGL means, and LVGL's values by name |
| [Where a tree lives](mounting.md) | Mounting a tree in a variable or a global, unmounting it, pages that follow a state, and one slot for one of several trees |

### A screen that changes

| Page | What it gives you |
|---|---|
| [Lists](lists.md) | Rows that change, with a fixed capacity: only the widgets of the changed row are touched |
| [Repetition](repetition.md) | Parts that the compiler repeats, for each enumerator, each captioned member or each record |
| [Option lists](options.md) | The options of a dropdown or a roller as a list, or from the enumerators of an enum |
| [Match](match.md) | A choice by value that the compiler checks: every value has exactly one arm |
| [Tasks](tasks.md) | Coroutines on LVGL's loop: wait for events, time and the first of several; work from another thread |
| [Animation](animation.md) | One property moved from one value to another, with an easing curve, awaited by a task |

### How it looks

| Page | What it gives you |
|---|---|
| [Design tokens](tokens.md) | A Design Tokens Format file read by the compiler; styles that take tokens only; three tiers |
| [Themes](themes.md) | One look for every widget: the built-in Signal theme with its typefaces, and a theme of your own from a token file |
| [Looks](looks.md) | A named group of design tokens used as a style, with parts, states and variants |
| [Fonts](fonts.md) | A TrueType file becomes a font in read-only memory, made by the compiler and chosen by a typography token |
| [Images](images.md) | A PNG file becomes pixels in read-only memory, decoded by the compiler into the display's own format |

### The program

| Page | What it gives you |
|---|---|
| [Application and loop](application.md) | Two objects start LVGL: the application, its clock and loop, and a display with its frame or band |
| [A pointer](input.md) | A mouse or a touch panel, the application's loop, and a window that closes, hides and shows |
| [Project settings](settings.md) | One JSON file for the whole program: what it may use, how strict the library is, and what the compiler may do |
| [Targets](targets.md) | A desktop window, a Cortex-M4 with what it measures, and the experimental browser target |

### Growing

| Page | What it gives you |
|---|---|
| [Concepts for parts](concepts.md) | The concepts that constrain a part's parameters, and what a wrong argument looks like |

### Tools

| Page | What it gives you |
|---|---|
| [Editor support](editor-support.md) | Completion, documentation and marks on mistakes in editors that use clangd, from one CMake line |

### Coming from another framework

| Page | What it gives you |
|---|---|
| [Coming from Qt Quick](from-qt.md), [Slint](from-slint.md), [TouchGFX](from-touchgfx.md) | What you know, mapped to lvgl26; the steps of a port; the decisions you must make; what is not there yet |
| [Port walkthrough: the Slint printer panel](port-slint-printer.md) | A whole application ported: the result, how it is organised, what differs, what it costs, and the choices made |
| [Port LVGL's own examples](port-lvgl-examples.md) | How an example is ported and compared with the C original, pixel for pixel; what a skipped example means |

## For AI agents

- [llms.txt](https://filipsajdak.github.io/lvgl26/llms.txt) lists the pages, and `llms-full.txt` next to it holds them all in one file.
- Every page is also published as Markdown: replace `.html` with `.md` in its address.
- Read [Writing good lvgl26 code](rules.md) before writing code. Cite a rule by its id.
- The repository is a Claude Code plugin. Its skills point to these pages.

## Writing a page

### Code on a page is built

- A block that shows code has the line `<!-- snippet: path name -->` before it, and equals the lines between the two `//! [name]` lines of that file. Without a name it equals the whole file. A test builds the file. `python3 tools/docs.py sync` writes the blocks again from their files.
- A page names no `lv_` function, `lv_..._t` type or `LV_` name in its code. Only a page about `raw()` may, and it says `<!-- c-api: shown -->`.
- A mistake is shown as a file of `tests/fail/`, as a whole-file snippet, followed by `<!-- message: <test> -->` and a `text` block with the message as GCC prints it. The check fails when the text does not show what the compile-fail test pins in `tests/CMakeLists.txt`.
- A longer valid form stands beside a short one only when it is a built file too, labelled "the long way". There is no hand-written bad code.
- A picture on a page is written by a test, as `quick_start_picture` writes its own.
- `python3 tools/docs.py check` must pass.
- The root `README.md` is checked like a page, and `python3 tools/check_readme.py` holds it to the code: a change of syntax or of a feature updates it in the same pull request.

### The outline of a guide

A guide explains one feature. It opens with its title, one sentence of what the feature is for, and the line `Header:` with the header in backticks: `` Header: `<lv/list.hpp>` ``, or `` Header: `<lv/lv.hpp>`, the core. `` for a feature of the core.

Then come these sections, in this order. A section that does not apply holds one line that says so.

| Section | What it holds | What the check enforces |
|---|---|---|
| The smallest example | The first code to copy: a part of a built file | Not empty |
| How it works | The idea, short, in plain words | Not empty |
| What you can do | Each form of the feature, each with a part of a built file | Every C++ block is a snippet |
| What the compiler checks | A few files of `tests/fail/`, each with its message; the other mistakes by the name of their test | Each file shown has its message, tied to its test; no other code |
| What it costs | A table with the rows Processor time, RAM, ROM, Stack and Compile time | Each row links its source, or says "not measured yet" in both cells |
| Rules | The rules of the feature, as a block `<!-- rules: id id ... -->` and `<!-- rules: end -->` | The ids exist, the lines are those `sync` writes, and every rule that names the page as its guide is listed |
| Limits | What the feature cannot do today. Planned work is named on the status page only | Links the [status page](https://filipsajdak.github.io/lvgl26/status.html) |
| Names | The names of the header, and where every property and enumerator is listed | Links `docs/reference/` |

The check also fails when the line `Header:` lacks an opt-in header that a name in the page's examples needs. [headers.json](https://github.com/filipsajdak/lvgl26/blob/main/docs/reference/headers.json) says which header declares each name.

A page follows the outline once `docs.json` lists it under `outline`; from then on the check holds it there.

"What it costs" never states an unmeasured number. A number names the research note that measured it, and its date, or the code it is read from. Compile time is given in instructions of the compiler, as rough relative sizes.

### Prose

- One line per paragraph.
- Sentences of at most 25 words, in plain words. No marketing.
- Exact names and exact error texts, so that an agent can follow the page.
- A page shows only what works today. A planned feature is named on the status page, not described on a page.

### The rules

The rules are data: [docs/reference/rules.json](https://github.com/filipsajdak/lvgl26/blob/main/docs/reference/rules.json). `python3 tools/rules.py --page docs/rules.md` writes the page from it, and the test `rules_page` fails when the page is stale.

- An id is a quality and a number, such as `fast-build-2`. It never changes, and a removed rule leaves its number unused.
- A rule is one sentence. Its example is a part of a built file, or it says why there is none.
- A rule names a check only when the compiler or the build enforces it in a program, with the test that pins its message.
- A rule of run time or compile time names its measurement and its date, or says "not measured yet".
- A guide is an existing page, or a page in `pages_planned` until it is written.
