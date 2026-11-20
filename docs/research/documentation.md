# The documentation: the owner's decisions, the audit and the plan

Written 2026-11-20, for the agents that write and rewrite the pages of `docs/`.
Marks follow [README.md](README.md). The decisions below are the owner's, from an interview on 2026-11-20; they are rules for the work, not claims to check.

Read this note before you write a page. The rules of the code are in [rules.json](../reference/rules.json) and on the page [Writing good lvgl26 code](../rules.md); how a page is written is in [docs/README.md](../README.md).

## The owner's decisions (2026-11-20)

The request: "review all docs pages and update them to follow newest changes and apply all new rules regarding how to write better code with lvgl26 (no boilerplate, fast compile-time, compile-time checked, expressive and extendable)". Added in the interview: "The user should know how to write good code that will be fast, and will compile fast."

### Scope and reader

- Surfaces: the pages in `docs/`, `README.md`, the site pages (`index.html`, `status.html`, `agents.html`, `design.html`), the example code under `examples/`, and the agent material (the skill `write-ui`, `llms.txt`, `docs/README.md`).
- Depth: everything. Stale facts corrected, every snippet and its example in the best form the library offers, a guide for every working feature, a new sidebar.
- Reader: a C++ developer with some UI experience, and an AI agent writing the code. A page assumes modern C++ and no LVGL words, and gives exact names, rules and error texts.

### The qualities the pages teach

- No boilerplate, fast at run time, fast to compile, checked at compile time, expressive, extendable.
- "Extendable" has four meanings, each documented: an application that grows (a file per screen, state beside its screen, looks and tokens, lists and repetition); reusable parts (constrained with the public concepts, state as a template argument, a part library that takes its design as a parameter); a design system of your own (token file, looks, themes, typefaces); extending lvgl26 itself (a platform, a node kind that keeps data, a widget lvgl26 does not cover yet).

### The rules

- The rules are data with stable ids: id, quality, one sentence, why, the built example, the check if one exists, the guide.
- A tool writes the page "Writing good lvgl26 code" from the data. Every guide ends with its rules and links back. Check messages, the skill and the guides cite the id.
- Contrast only what a test proves: a mistake is its compile-fail file with the exact message. A longer valid form stands beside the short one only when it is a built file, labelled "the long way". No hand-written bad code.

### Performance

- Two pages: "Code that runs fast" and "Keeping builds fast".
- Compile time: rules with rough relative sizes on the page; exact numbers live on the nightly metrics page.
- Run time: every rule rests on a number from a probe. A missing probe is added to the nightly job.
- Every guide's "What it costs" states run-time cost (processor time, RAM, ROM, stack) and compile-time cost.

### Structure

The sidebar groups the pages by the layers of the framework. An existing page keeps its address; only grouping and titles change. `input.md` keeps its address and gains a sibling for the application and its loop.

| Group | Pages |
|---|---|
| Start | Quick start, Which header, Writing good lvgl26 code |
| Core ideas | The tree and its widgets, State and bindings, Views and ids, Values and units, Events, Mounting and slots |
| A screen that changes | Lists, Repetition, Options, Match, Tasks, Animation |
| How it looks | Styles, Design tokens, Looks, Themes, Fonts, Images, A design system of your own |
| The program | Application and loop, Input, Settings, Code that runs fast, Keeping builds fast, Testing a screen, Targets |
| Growing | An application that grows, Parts and concepts, Extending lvgl26, Run-time documents, Agent kit |
| Tools | Editor support, For AI agents |
| Coming from another framework | Qt, Slint, TouchGFX, LVGL in C, two ports |

### One outline for every guide

The title, one sentence of what it is for, `Header:`, then: The smallest example, How it works, What you can do, What the compiler checks, What it costs, Rules, Limits, Names. A section that does not apply says so in one line. The docs check enforces it.

### Examples, tutorial, reference, pictures

- Each guide keeps a small self-contained example.
- One new tutorial builds a coffee machine step by step, each step a built program: a list of drinks, a page per step, a brewing task with progress and animation, strength and size as typed values, a service page with settings.
- Reference: a tool writes one page per widget from the reference data when the site is built. Never committed, linked from every guide, not in `llms-full.txt`.
- Every guide's example runs live in the browser (WebAssembly), over a test-written picture or frame strip. The nightly job builds the modules and checks their frames against the host build.

### Features that are not built

- A page shows only what works, and never plumbing that a planned feature would remove. Planned features are named on the status page only.
- Build the feature, then document it. `lv::shown` is built next, before `lv::start`, `lv::job` and `lv::run`. The printer port's pager section is cut to a pointer until then.

### Migration pages

Qt and Slint: a short fragment in their own language, written by us, beside the built lvgl26 snippet. TouchGFX stays a table with links. A page "LVGL in C" joins the group.

### Checks that a tool enforces

1. Every C++ block on a page is a snippet of a file that a test builds.
2. An error message quoted on a page is tied to its compile-fail test, and the check fails when the two differ.
3. Everything has a page: a feature that the status page lists as working has a guide, and a public name appears in a page or a reference entry.
4. Examples are linted for plumbing: a task spawned without an owner outside `main`, a loop that creates widgets, a raw value where a token belongs, a hand-written pager. An exception carries a comment with its reason.
5. The fixed outline.

### Process

- The documentation work builds nothing in full on the local machine. One changed file may be compiled with `-fsyntax-only`; GitHub CI runs the gates.
- A pilot the owner reads first: the rules page, one existing guide in the new outline (Lists), one new guide (State and bindings).
- After his comments: batches, merged when CI is green. Start, Core ideas and The program first. "How it looks" after the fonts and theme designs are decided. Message sections after the compile-fail tests pin whole messages.

### Projects split into files (2026-11-20)

The owner asked: "ensure our examples show how to split big projects into multiple files to make them managable."

- Where it is shown: the coffee machine tutorial, which starts as one file and splits in one of its steps; a small layout example of its own for the guide "An application that grows"; the two ports (printer, treadmill), brought to the same layout; and the project template repository.
- The layout is by screen, with shared folders:

```
coffee/
  main.cpp            starts the app, mounts shell
  shell.cpp .hpp      the frame, lv::shown of pages
  state.hpp           what several screens share
  screens/
    drinks.cpp .hpp   a screen and its own states
    brewing.cpp .hpp
    service.cpp .hpp
  parts/
    tile.hpp          functions that return a tree
  design/
    tokens.json       the design tokens
    design.hpp        names the token file, looks
    fonts.hpp         declares the drawn fonts
    font_title.cpp    one file per font
    font_body.cpp
  platform/
    window.cpp        the only file that knows SDL
  lvgl26.json         project settings
  CMakeLists.txt
```

- Enforcement: a layout check that users can run too. It ships as a tool that a project switches on in CMake, as the include check does, with an option to turn it off. It refuses a file that defines more than one screen, a font drawn outside `design/`, a platform name outside `platform/`, and a screen that includes another screen's header. The repository's own multi-file examples run it.
- It depends on two features built first: pages that live in other files under `lv::shown`, with no size number written by hand, and fonts drawn once, with one file per font.
- The rule is `extendable-1`. Its data names the layout check in `check_planned`, which the page shows as not built yet, until the check exists.

### The owner's steer on the first rule list (2026-11-20)

- "Fast at run time" covers processor time, not only memory: a display nobody sees, a hidden widget, a binding against polling, changing only what changed, and the cost of an animation. Every rule of run time ends up resting on a nightly probe; until it does, its data says `probe_needed`, and the tool fails a rule of run time that has neither.
- A widget is reached by an id that the compiler resolves, `root->get<"title">()`, not through a stored pointer or a search at run time (`checked-9`).
- Where two qualities pull apart, clarity wins over build time: choose the widget that says what the element is. The rule "take a widget kind the file already uses" (`fast-build-3`) is gone; its number stays unused. What a new kind costs a file (5.6 G instructions against 0.2 G for a node, 2026-11-19) is a fact for the page "Keeping builds fast".
- `no-boilerplate-9` (one of several trees in an `lv::one_of` slot) changes when `lv::shown` is built. Its data says so in `revisit`, which the page does not show.

## The audit (2026-11-20)

Every page, `README.md`, the site pages and `examples/README.md` were read against `main`. `source` 2026-11-20, nothing compiled.

| Finding | Where |
|---|---|
| The docs index listed 10 of 19 guides and said more were being written | `docs/README.md` (fixed by the pilot) |
| Token files in the forms that `lv::tokens` now refuses (`"16px"`, `"#2b7fff80"`), and "object colours are not read for alpha", which is wrong | `looks.md` |
| The counts of tests, presets, fuzz targets, ported examples and migration concepts differ between the site pages | `site/index.html`, `site/status.html` |
| `lv::screen_active()` as the screen, against the rule that a program gets it from `lv::display` | `site/index.html`, `site/agents.html`, header comments |
| "A state is an `lv_subject_t` underneath", which is no longer so | `site/index.html`, `site/status.html` |
| "A part in a state has no form yet", though `lv::on` works | `from-qt.md`, `examples/README.md` |
| Handlers written as named functions or `update` lambdas where `[] { --count; }` works | `quick-start.md`, `concepts.md`, `examples/parts`, the printer port |
| The printer walkthrough mixes a guide, a cost study and decisions; it teaches `void (*)()` handlers, `template <class Count>`, an option text with line breaks and a hand-written pager | `port-slint-printer.md` |
| Two ways to name an enumerator's text: a `labels` struct with `lv::like`, and `lv::option_text<E>` | `repetition.md`, `options.md` |
| Sentences over 25 words: 21 in `looks.md`, 9 in `input.md` | |
| Free-hand C++ blocks: 16 in the pages, 19 in `README.md` | `fonts`, `images`, `input`, `looks`, `mounting`, `quick-start`, `settings`, `themes`, `which-header` |
| No "what a mistake looks like" with real text, no link to the reference data, and no picture of a visual feature | most guides |
| Working features with no guide: the tree, state and bindings, computed values and effects, views and ids, styles, tokens, units, tasks, animation, other threads, match, run-time documents, the agent kit, the application and its loop, the microcontroller slice | `site/status.html` |

Ranked most out of date: `docs/README.md`; the counts and in-review rows of the site; the token forms in `looks.md`; `from-qt.md`, `from-touchgfx.md` and `examples/README.md`; the printer walkthrough; the screen lines of the site.

### What would shorten user code further

These need a feature first. They are not rules, and no page describes them.

| Today | Shorter, once built |
|---|---|
| A `labels` struct for `lv::like`, and a `switch` in `lv::option_text<E>`, for the same texts | One table of an enum's texts that both read |
| `app.advance(50 * ms), app.step();` | `app.step(50 * ms)` |
| Three lines that embed the token file and read it, in every file that uses tokens | The token file named once, in the settings |
| No way to reach the widgets of `lv::for_each`, so a task per widget is started by hand | A task started on each repeated widget |
| An `lv::list` filled by `assign` from a constant array | An `lv::list` built from a braced list |
| A pager of `lv::owned` trees in the printer port | `lv::shown` |
| A part with its own state and slots | Components and `lv::use` |

## What the pilot found (2026-11-20)

| Finding | Mark |
|---|---|
| A text binding prints the number of a state through LVGL's `printf`: a quantity with its unit, a count or a named value as its number, and an enum as its number, not its name | `source`, `include/lv/props.hpp` (`show_text`) |
| A bound widget's follower is a node on LVGL's heap (`make_node`, `lv_malloc`), freed with the widget; a computed and an effect keep their nodes inside themselves | `source`, `include/lv/props.hpp`, `include/lv/computed.hpp` |
| `memory-region.md` (2026-11-18) says the heap of a bound label was "removed by the spike in mounted-storage"; the code still allocates the node above | `source`; measure again before a page states a number |
| `lv::changed` is public (`headers.json`) and a follower of a state, but the status page does not name it, so State and bindings does not document it | `source` |
| `state.observe()` returns LVGL's observer type, so a page cannot show it; `slider_bind_state` uses it with a second state where an `lv::computed` fits | `source` |
| Most compile-fail tests pin a fragment of their message. The message check therefore asks that the shown text contains a match of the pin; once the tests pin whole messages, the same check is exact | `source`, `tests/CMakeLists.txt` |
| The editor check read every file a page shows; a file of `tests/fail/` is now left out, since it is wrong on purpose | `source`, `tools/editor/check.py` |
| Examples that break a rule today: `quick_start` (`reset()` and `&reset`), `parts` (`update` lambdas), the printer (`void (*)()`, `\n` options, the pager), `mounting` (test code beside `set`) | `source` |
| The row template of `examples/lists` styles each node and repeats `.style_checked` pairs, against `no-boilerplate-7` and `expressive-4`. It stays until "How it looks" is rewritten: a look changes its picture | `source` |
| A text binding with a format prints a quantity's number without its unit: `lv::bind(speed, "now {}")` shows "now 40" | `source`, `tests/state.cpp` |

## The plan

State: "pilot" is done in the pilot; "rewrite" is an existing page; "new" is a page to write. File names of new pages are the ones in `pages_planned` of `rules.json` where a rule names them.

| Page | File | State | Example to draw from | Waits for | Batch |
|---|---|---|---|---|---|
| Quick start | `quick-start.md` | rewrite | `examples/quick_start` (a lambda for `reset`) | | 1 |
| Which header | `which-header.md` | rewrite | generated table | | 1 |
| Writing good lvgl26 code | `rules.md` | pilot | `rules.json` | the owner's pass; then the ids freeze | pilot |
| A coffee machine (tutorial) | `tutorial.md` | new | new, one program per step; it starts as one file, and a step splits it into the layout | `lv::shown` with pages in other files; fonts drawn once; Tasks | 4 |
| The tree and its widgets | `tree.md` | new | `examples/state` tree, `tests/l2_tree.cpp` | | 1 |
| State and bindings | `state.md` | pilot | `examples/state` | its picture | pilot |
| Views and ids | `views.md` | new | `examples/mounting` local, without test code | | 1 |
| Values and units | `values.md` | new | `examples/state` typed, `examples/quick_start` | | 1 |
| Events | `events.md` | new | `examples/lvgl/widgets/button_event.cpp` | | 1 |
| Mounting and slots | `mounting.md` | rewrite | `examples/mounting` | `lv::shown` for pages inside a tree | 1 |
| Lists | `lists.md` | pilot | `examples/lists` | an example of `.on_row` | pilot |
| Repetition | `repetition.md` | rewrite | `examples/repeat` | one table of enumerator texts | 2 |
| Options | `options.md` | rewrite | `examples/options` | the same | 2 |
| Match | `match.md` | new | a new example | | 2 |
| Tasks | `tasks.md` | new | `examples/bounce`, `examples/wasm/counter` | `lv::start`, `lv::job` | 2 |
| Animation | `animation.md` | new | `examples/bounce` | a run-time probe | 2 |
| Styles | `styles.md` | new | `examples/theme` | the fonts and theme redesign | 3 |
| Design tokens | `tokens.md` | new | `examples/lists/tokens.json` | the same; the token format moves here from `looks.md` and `editor-support.md` | 3 |
| Looks | `looks.md` | rewrite | `examples/looks` | the same | 3 |
| Themes | `themes.md` | rewrite | `examples/theme`, `examples/signal_type` | the same | 3 |
| Fonts | `fonts.md` | rewrite | `examples/fonts` | the same | 3 |
| Images | `images.md` | rewrite | `examples/images` | | 3 |
| A design system of your own | `design-system.md` | new | Signal | the same | 3 |
| Application and loop | `application.md` | new | `examples/window`, `examples/mcu/slice.cpp` | `lv::run` | 1 |
| Input | `input.md` | rewrite | `examples/window` | | 1 |
| Settings | `settings.md` | rewrite | `examples/settings` | | 1 |
| Code that runs fast | `run-fast.md` | new | the fast-run rules | run-time probes in the nightly job | 2 |
| Keeping builds fast | `build-fast.md` | new | the fast-build rules | the nightly metrics page | 2 |
| Testing a screen | `testing.md` | new | `lv::write_png`, the example tests | | 1 |
| Targets | `targets.md` | new | `examples/mcu`, `examples/wasm` | | 1 |
| An application that grows | `growing.md` | new | a small layout example of its own, in the layout above, which runs the layout check | `lv::shown` with pages in other files; fonts drawn once, one file per font; the layout check | 4 |
| Parts and concepts | `concepts.md` | rewrite | `examples/parts`, with `--Count` | | 4 |
| Extending lvgl26 | `extending.md` | new | `examples/platform/sdl_window.hpp`, `lv::widget` | | 4 |
| Run-time documents | `runtime.md` | new | `examples/agent` | | 4 |
| Agent kit | `agent-kit.md` | new | `examples/agent`, `site/agents.html` | | 4 |
| Editor support | `editor-support.md` | rewrite | | | 4 |
| For AI agents | `for-agents.md` | new | `llms.txt`, the skills | | 4 |
| Coming from Qt, Slint, TouchGFX | `from-qt.md`, `from-slint.md`, `from-touchgfx.md` | rewrite | ports | components | 5 |
| LVGL in C | `from-lvgl.md` | new | `examples/lvgl` | | 5 |
| Two ports | `port-slint-printer.md`, `port-lvgl-examples.md` | rewrite, the printer split | the printer and the treadmill, brought to the layout above | `lv::shown` for the pager and for pages in other files; fonts drawn once | 5 |

Batch 1 is Start, Core ideas and The program, the stable layers. Batch 2 is the rest of the screen and the performance pages, once their probes exist. Batch 3 is "How it looks", after the redesign. Batch 4 is Growing and Tools; batch 5 the migration pages.

A section "What the compiler checks" shows whole messages only after the compile-fail tests pin them; until then it shows the message GCC prints, checked against the pinned part.

### Probes that do not exist yet

Each rule of run time that no nightly probe measures says what its probe must measure (`probe_needed` in `rules.json`):

- `fast-run-1`: LVGL's heap after a mount and after a thousand changes of the states of a screen that does not change shape: the same number.
- `fast-run-2`: instructions and pixels flushed for one row inserted into a list of twenty, against building the twenty rows again.
- `fast-run-3`: LVGL's heap of a row that shows a text field through lv::bind, against one that copies the text.
- `fast-run-4`: LVGL's heap and instructions a second of a task whose owner was deleted, against one with no owner.
- `fast-run-6`: LVGL's heap and instructions to draw a label in a font the compiler made, against the same font drawn by TinyTTF.
- `fast-run-7`: LVGL's heap after the first display, with and without LVGL26_NO_DEFAULT_THEME.
- `fast-run-10`: instructions a second of a label bound to a state that changes once a second, against a timer that sets the label every 50 ms.
- `fast-run-11`: instructions and pixels flushed for one property change, against unmounting and mounting the same tree.
- `fast-run-12`: instructions a second of one animation, against the area of the animated widget.

The guides of the pilot say "not measured yet" for these too: the processor time of a state change and of a row change, the stack of both, and the code size of a state, a computed and an effect.

### Checks that do not exist yet

- The layout check of a project split into files, switched on in CMake like the include check (`extendable-1`). It waits for `lv::shown` with pages in other files and for fonts drawn once.
- Everything has a page (decision, check 3).
- The lint for plumbing in examples (check 4).
- A rule whose example is pending fails once a marked region exists for it: today the data only says why there is none.
- The prose rules (one line per paragraph, 25 words a sentence) are kept by review, not by a tool.

### Rules to revisit

- `no-boilerplate-9`: lv::shown: pages that take turns inside a tree get a form of their own; the rule then covers roots only.

### Later work outside the pages

The project template repository, brought to the layout of a project split into files.

Per-widget reference pages written when the site is built, the live browser examples with a picture underneath, the rewrite of `README.md` and the site pages, and the coffee machine tutorial.
