# lvgl26 documentation

lvgl26 is a C++26 framework over LVGL for embedded user interfaces. The compiler checks a screen before it runs, and your code calls no C.

## The pages

The groups are the layers of the framework, as the sidebar shows them. `docs.json` holds the sidebar.

| Group | Pages |
|---|---|
| Start | [Quick start](quick-start.md), [Which header](which-header.md), [Writing good lvgl26 code](rules.md) |
| Core ideas | [State and bindings](state.md), [Where a tree lives](mounting.md) |
| A screen that changes | [Lists](lists.md), [Repetition](repetition.md), [Option lists](options.md) |
| How it looks | [Themes](themes.md), [Looks](looks.md), [Fonts](fonts.md), [Images](images.md) |
| The program | [A pointer](input.md), [Project settings](settings.md) |
| Growing | [Concepts for parts](concepts.md) |
| Tools | [Editor support](editor-support.md) |
| Coming from another framework | [Qt Quick](from-qt.md), [Slint](from-slint.md), [TouchGFX](from-touchgfx.md), [the Slint printer panel](port-slint-printer.md), [LVGL's own examples](port-lvgl-examples.md) |

[Writing good lvgl26 code](rules.md) holds the rules that every page teaches, each with an id.

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
