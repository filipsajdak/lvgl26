# lvgl26

A C++26 framework over LVGL. Namespace `lv`. GCC 16 or newer only: the code uses reflection.

## Build and test

```sh
cmake --preset host-v96 && cmake --build --preset host-v96 && ctest --preset host-v96
```

| Preset | What it checks |
|---|---|
| `host-v96`, `host-v10` | LVGL v9.6.0 and the v10 pin, warnings as errors |
| `host-v96-asan`, `host-v10-asan` | AddressSanitizer and UndefinedBehaviorSanitizer, with `float-cast-overflow`, which GCC leaves out of `undefined` |
| `host-v96-tsan` | ThreadSanitizer (Linux only; GCC has no TSan on macOS) |
| `host-v96-analyze` | GCC static analyzer (`-fanalyzer`) |
| `host-v96-coverage` | Coverage; report with `gcovr --filter include/lv/ build/host-v96-coverage` |

Every change must pass `host-v96` and `host-v10` locally. Run the tests side by side:
`ctest --preset host-v96 -j 8`.

| Where | What runs |
|---|---|
| Every pull request and push to main (`ci.yml`) | `host-v96`, `host-v10`, `host-v96-asan`, `host-v96-analyze`, the compile-fail tests on both LVGL versions, ThreadSanitizer on the threaded tests, the editor stubs read by Clang, CRAP, one minute of fuzzing |
| Every night (`nightly.yml`) | `host-v10-asan`, `host-v96-tsan` on every test, half an hour of fuzzing, the slice on an Arm Cortex-M4 in QEMU (`tools/arm-run.sh`), mutation testing of the lines of `include/lv` changed since the last clean night (below). A failure opens an issue |

- The analyzer build compiles three files at once (`LVGL26_ANALYZE_JOBS` in `cmake/quality.cmake`): a test file needs up to 2.4 GB under the analyzer, and four at once lost the CI runner. Raise it only with a measurement of peak memory.
- The analyzer (`host-v96-analyze`) runs on the tests, not on a port: `lvgl26_skip_analyzer(<dir>)` in `cmake/quality.cmake` leaves the targets of a directory out. It triples the compile time of a port, and the job is near its limit.
- A check belongs in the pull request run if it takes a few minutes and a change can
  plausibly break it. Anything long goes to the nightly run.
- A test must be safe to run next to every other test: no shared file, no fixed port.
- The compile-fail tests have the ctest label `compile-fail`. They need no build:
  `ctest --preset host-v96 -L compile-fail -j 8` after configuring. The other tests are
  `-LE compile-fail`.
- The CI jobs run in the image of `.github/ci-image/Dockerfile`. Add a tool there, not as an
  install step in a workflow.
- A change to `site/` or to a Markdown file alone does not start the pull request run.
- The CI jobs compile through ccache (`.github/actions/setup`). Each build step prints its
  hit rate: an unchanged file must be a hit.
- Module scanning is off (`CMAKE_CXX_SCAN_FOR_MODULES` in `CMakeLists.txt`): ccache refuses a
  compile that has the module flags. A target with C++ modules turns it on for itself.
- ccache before 4.14 needs `CCACHE_NODIRECT=1`: its direct mode does not see a changed
  `#embed` file.

## Rules

- Both LVGL versions must work. Use only LVGL's public `include/lvgl` API.
- Mistakes should be compile errors with a message that says how to fix them. Add a
  compile-fail test in `tests/fail/` for every new one.
- The library throws nothing: compile error first, then contracts, then `std::expected`.
- No `throw`, `try` or `typeid` in the headers, also not in consteval code: the library must
  build with `-fno-exceptions -fno-rtti` (`tests/no_exceptions.cpp`). A compile-time error with
  a computed message is `detail::fail(message)`.
- A program starts LVGL with `lv::application` and gets its screen from an `lv::display`. Examples,
  the README, the site and the docs call no `lv_init`, `lv_deinit`, `lv_display_*`, `lv_tick_*` or
  `lv_timer_handler`. The tests of the library may. `raw()` is the one door to the C API.
- An LVGL enumerator is written as a name in quotes (`.align = "center"`). An example names no
  `lv_` function, `lv_..._t` type or `LV_` name: `tools/check_examples.py` fails on one. A line
  that needs the C API on purpose ends with the comment `// C API`.
- Something follows an `lv::state` or an `lv::computed` through a node in its list (`detail::watch_node`, `include/lv/detail/watch.hpp`): a widget, an `lv::computed`, an `lv::effect`, `lv::changed`. Do not add an LVGL observer for it: the LVGL subject exists only for `raw()`. LVGL's own bind function, and with it a subject, is kept only where `keeps_lvgl_binder` in `include/lv/props.hpp` says so (an animation LVGL starts after the first draw, and `checked`). A walk over a list is safe against watchers that remove or add watchers, and the tests in `tests/watchers.cpp` say how.
- Views never delete widgets. Only `lv::mount` and `lv::owned` own.
- Styles take design tokens only. Add the token to the token file; use `lv::raw(...)` only for a
  reasoned exception. Use semantic or component tokens in UI code, not primitives.
- A look shared by all widgets of a kind is a theme (`lv::theme`, `include/lv/theme.hpp`), given to
  the display. A theme's styles are built in constant evaluation with `std::define_static_array`,
  so they need no template argument and no heap. A style for one part in one state is `lv::on(...)`
  merged into `.style` (also in a theme): a constant `lv_style_t` the style keeps in its last member,
  `layers`. Do not add a style member per part and state. `include/lv/signal/*.json` is written by
  `tools/signal_tokens.py` from `design/signal/tokens.design.json`: change the design file, then
  run the tool.
- An element names its look; the token file says how it looks. `lv::look<"queue.card">` (`include/lv/look.hpp`) is a group of the project's tokens as a constant style, by entry name (a style property, with its token type checked), so a style written out entry by entry in a tree is a look. The project names its token file once: `lv::design<lv::override>` specialized with `lv::design_of`, like `lv::cfg`; a part library passes the design on as a template parameter. A nested group named like an LVGL part or state is a layer; any other nested group is a look of its own, so a variant is never named `alt`, `knob` or the like. The node's own properties beat a look (LVGL's local styles), and what a factory sets by default gives way to a look that says it (`style_sets_width` in `tree.hpp`): a new default in a factory yields in the same way. A shorthand entry (`pad_all`) is stored as its long forms. A new mistake in a look gets a message that lists what is valid and a compile-fail test `tests/fail/look_*.cpp`; `tools/look_groups.py` lists groups that no look names.
- A consteval function with no template parameter whose body calls a scan with constant arguments (`std::meta::members_of(^^::, ...)`) runs the scan where the header is read, in every file: `enum_of_property` in `look.hpp` cost 0.8 G instructions in a file that names no look until the scope became a parameter. Measure a new header's include cost against main with a file that only includes `<lv/lv.hpp>` (`tools/compile_report.py` takes the flags of a file of the build; copy a command for a new one), not through a test that already uses styles.
- `lv/signal.hpp` draws no font and reads no typography token. Signal's typefaces, their tokens (`include/lv/signal/type.json`) and the themes with fonts are in `lv/signal_type.hpp`. Both headers build their themes from the one description in `lv/signal.hpp`. Measure a change to `lv/signal.hpp` against main in instructions: it must not cost more.
- A header embeds only a file that lies next to it. `include/lv/signal/fonts/` holds copies of the files in `site/media/fonts/`, and the tests `signal_type_file_*` fail when a copy differs.
- An array that a header embeds is `constexpr`, not `inline constexpr`. Under a sanitizer GCC 16.2 does not know that the address of an inline variable is not null. Reading its text in constant evaluation then fails to compile.
- A font in a style comes from a typography token through `lv::typeface`; `lv::raw(font)` marks one
  that no token names. `include/lv/detail/coverage.hpp` is derived from font-rs and stays under the
  Apache License 2.0: keep its notice, and list any other borrowed part in `NOTICE.md`.
- A part of LVGL that costs the device code or heap is behind a CMake option of its own, off by
  default (`LVGL26_TTF`, `LVGL26_PNG`, `LVGL26_JPEG`). So is leaving something out: `LVGL26_NO_DEFAULT_THEME`
  drops LVGL's own theme (`host-v96-no-theme` tests it). The tests build with them. Its C functions are found by reflection
  (`detail::find`), and a `static_assert` names the option when they are missing.
- lvgl26 sets no `-fconstexpr-ops-limit`: GCC's default of 33,554,432 operations covers what a user
  writes. The limit counts one constant evaluation, so work that needs more is split into evaluations
  of its own: a font draws each glyph, each row of kerning pairs and its bitmap in variable templates
  (`glyph`, `kerning_row`, `bitmap` in `include/lv/font.hpp`), reached with `std::meta::substitute`.
  The tests `font`, `font_bytes` and `signal_type` compile with no option, and are the guard. A program
  of the repository that needs more gets a fixed budget of its own, at about twice its need
  (`large_tree`, `lv_reference` in `tests/CMakeLists.txt`). A single loop in constant evaluation must
  still stay under 262144 rounds.
- Code that runs in the compiler is written for its interpreter: look codes up in tables, give a
  buffer its size once and write through pointers, avoid a call for each byte
  (`include/lv/detail/png_reader.hpp` needs an eighth of the work of its first form).
- A large result of constant evaluation is stored with `std::define_static_array`: returning a
  `std::array` of 200 KB costs several times the decoding. Its data becomes part of a symbol name,
  and Apple's linker refuses a very long one, so sizes are limited (`compile_time_image_pixels`).
  It costs about 130 operations a byte, in one loop that must stay under 262144 rounds. A font's
  bitmap is therefore a `std::array` in a variable template of its own, filled through a pointer at
  about 20 operations a byte, and named by its glyphs rather than its data (`bitmap` in `font.hpp`).
- `docs/reference/lvgl-<version>.json` is the reference as data: widgets, properties, what each takes,
  enumerator names, style members. `tools/reference/reference.cpp` prints it by reflection, and the
  test `reference_data` fails when a committed file is stale. Write it again with the program the
  test names; never edit it by hand.
- Work that goes on while a part of the screen exists is started with `lv::spawn(task, owner)`: it
  ends when `owner` is deleted. A task without an owner lives until it returns, or until a widget it
  has waited on is deleted.
- No macros and no `#if` in our code. Settings go in `lv::config` (see `include/lv/config.hpp`);
  optional LVGL features are detected with `requires` or reflection. `#embed`, `#include` and
  `#pragma once` are the only preprocessor lines we write.
- Application values are strongly typed: a measure is an mp-units quantity (`120 * km / h`), a
  count is `lv::counter`, anything else a one-member struct. Do not add a unit system to lvgl26.
- No handwritten per-property code. Properties, styles and events come from reflection.
- A unit that LVGL documents only in a comment (degrees or tenths of a degree) is read from the
  embedded LVGL headers (`include/lv/detail/angle_docs.hpp`). A hand-written list of setters is not
  allowed. A new header with an angle goes into `angle_headers`; `tests/angle_units.cpp` checks the
  reading against the pixels.
- A list of options for a dropdown or a roller is written `.options = {"a", "b"}` or
  `lv::options_of<E>`, not as a text with line breaks: examples and docs use the list. Which setters take a
  list is found by name (`include/lv/detail/option_docs.hpp`), never listed by hand, and `tests/options_docs.cpp`
  checks the names against LVGL's doc comments: the library does not read the headers, because an `#embed`
  and a search in every file cost 0.2% of the compile time of `tests/large_tree.cpp`. A state of an enum follows `.selected` through
  `lv::options_of<E>` in a constexpr tree only (the observer's data is in read-only memory); at run time
  `set()` refuses it with a contract. `lv::but` lives in `include/lv/but.hpp`, one definition for every
  user of it.
- Anything a header does at namespace scope costs compile time in every file. Keep reflection
  scans lazy: put them behind a template parameter (see `props_of` in `include/lv/props.hpp`).
- Inside a template, name what is generated for one widget through a template parameter
  (`members_of_props<"obj", D>`, `getter_table<"obj", D>`), never plainly. A name that depends on
  nothing is generated where the template is read: in every file that includes the header.
  A state type reads `lv::cfg`, so a class template names it through its parameter
  (`count_state` in `include/lv/list.hpp`); otherwise a project's settings come too late.
- Under a sanitizer GCC 16.2 does not compare two different function addresses, nor the address of
  an inline variable or a template parameter object with null, in a constant expression. Carry a
  number instead (`follows` of `lv::arg` says what a property follows), or compare
  `std::meta::reflect_constant` of both (see `detail::is_null` in `include/lv/values.hpp`).
- GCC 16.2 runs a call whose arguments are all constants twice when it is written inside a
  function template: once when the template is instantiated, once when the function runs. Hand
  the result of a scan in as an argument (see `scan_base` in `include/lv/props.hpp`).
- Repetition (`lv::for_each`, `lv::for_each_field`, `include/lv/repeat.hpp`) is expanded by the compiler into the children of the parent: a `detail::repeated` value is flattened by `splice` in `include/lv/tree.hpp`, so the tree is the one written by hand. A part sets no `.id`. A caption is a string literal, since an annotation holds a structural type and GCC 16.2 refuses `std::string_view` there. A record with a text literal is taken by reference (`[]<const auto& R>`), because a literal is no template argument. Clang reads no annotation, so `tests/editor/api_uses.cpp` repeats the members of a row type in a template that is never instantiated.
- A node of a tree keeps `detail::kept<W>`, not `props<W>`. A props struct has thousands of
  members, and the compiler walks every one each time it copies a node.
- `lv::arg` and what it holds have no default member initializer, so that an unset property is
  all zeros. A constructor to run there is paid for every unset member of every node.
- `tests/large_tree.cpp` is compiled under a fixed `-fconstexpr-ops-limit`. It stops compiling
  when a node becomes expensive for the compiler again. Raise the limit only with a measurement.
- A header that every file includes pays for what it declares: measure `tests/large_tree.cpp` with
  `tools/compile_report.py`, at least five runs alternating with main, and compare the minimum and the median.
  A `constexpr` variable or a search that is evaluated where it is written is paid by every file.
- Measure before claiming: compile time with `/usr/bin/time`, heap with `lv_mem_monitor`,
  pixels with `lv_test_screenshot_compare`.
- Compare compile times in instructions, not in seconds: on a busy machine the same compile takes
  several times longer. `/usr/bin/time -l` prints "instructions retired", but for the process it
  starts, so start `cc1plus` itself: `g++ -###` prints its command.
  `tools/compile_report.py <build dir> <file>...` does this and prints one JSON row per file (see `docs/research/compile-time.md`).

## Test adequacy

| Check | How | Limit |
|---|---|---|
| Complexity | `lizard`, through `tools/crap.py` | 8 for every function, also compile-time ones |
| CRAP | `tools/crap.py build/host-v96-coverage` after the coverage preset | 8 for every function that runs at run time |
| Mutation | The full run of the changed lines is nightly: one shard for each changed header (`tools/mutation_plan.py`, the tests of each in `tools/mutation_tests.json`). Locally `tools/mutation.py <header> --tests '<ctest -R>' --since origin/main --sample 40` is enough for a quick look | Every survivor gets a test, or a reason in `tools/mutation_accepted.json` |
| Fuzzing | `fuzz/json_reader.cpp` (libFuzzer, Clang); `fuzz/runtime_document.cpp`, `fuzz/mcp_message.cpp` and `fuzz/mcp_stream.cpp` (AFL++ with its GCC plugin: `fuzz/afl.sh`) | Every free input has a target; every crash becomes a corpus file |

- Mutation is nightly: the run takes the lines of `include/lv` changed since the tag `nightly-mutation-base` (moved by the run after a clean night), and opens an issue with the survivors. Run it by hand with `since=<ref>` (workflow_dispatch) to cover an older change. An MR description says `Mutation: nightly, see run <url>`, or the line with counts when they are known.
- A function over complexity 8 is split, not excused. A branch per type is one overload per type.
- A `consteval` function has no run-time coverage: test it with `static_assert` and compile-fail tests.
- Test code that only runs in constant evaluation does not count as coverage. Give run-time code a
  run-time test with values the compiler cannot fold.
- gcovr does not add up the instantiations of a template; use `tools/crap.py` for coverage numbers.
- Mull cannot be used: it needs LLVM, and Clang cannot compile the reflection code.

## Where a requirement goes

| What must hold | Write it as |
|---|---|
| A property of a type, known at compile time | A named concept on the interface. Standard concepts first. Every template parameter of a public function or class has one, unless it has no requirement at all |
| A property of values, known at compile time, whose message must name things | `static_assert` with a computed message, or `consteval` |
| A call of the wrong shape, with a fixed message | A fallback overload `= delete("reason")` with the same signature and no constraint |
| Something only known at run time that is a bug if violated | `pre` on the declaration; `post` for a guarantee; `contract_assert` only for an invariant inside a body |
| A lifetime rule | Ref-qualifiers and types, not a contract |
| Bad input from outside (a document, a file) | `std::expected`, never a contract |

- A function with a precondition is not `noexcept`.
- GCC 16.2 cannot compile `pre` on a function with a deduced return type, or a fold over the
  parameters inside `pre`. Use `contract_assert` as the first statement there, with a comment that
  says so.
- Every new error message gets a compile-fail test in `tests/fail/`.
- GCC 16.2 folds a call with constant arguments without its contracts when the call is part of
  a larger expression. A constexpr function with a contract on its values repeats the condition
  as its first statement with `detail::keep_checked(...)` (see `include/lv/detail/checked.hpp`),
  and gets two compile-fail tests: the call as a whole initializer, and inside an expression.
- To test that a valid value passes a precondition, make the call the whole initializer of a
  `constexpr` variable, before any other use of the same arguments. A `static_assert` does not
  evaluate the precondition.

- A part (a function that returns a piece of tree) constrains its parameters with the public concepts of `docs/concepts.md`: `lv::tree_handler` (a handler in a constexpr tree), `lv::event_handler` (run time), `lv::dependency`, `lv::tree_node`, `lv::view_value`, `lv::style_value`, `lv::font_value`, `lv::token_value`, and `lv::repeat_part` for the part of `lv::for_each`. Never `void (*)()` for a handler, and never a bare `template <class Count>` for a state. A new concept is public (`lv::`, not `lv::detail::`), has `static_assert`s in `tests/concepts.cpp`, a compile-fail test in `tests/fail/part_*.cpp`, and its stub in `tools/editor/stubs/lv/`.
- Clang 21 does not read `template <lv::dependency auto& Count>`: write `template <auto& Count> requires lv::dependency<std::remove_cvref_t<decltype(Count)>>` in code that the editor stubs or `tests/editor/api_uses.cpp` read.
- A handler in a constexpr tree is a function pointer: a function or a lambda without captures. A part that needs state names it as a template argument.

## Definition of done for a feature

A feature pull request also:

- Moves the feature's line from "planned" to "works today" in `site/status.html` (the comment at the top of that file says how).
- Adds the feature to the feature list in `site/index.html` and in `README.md`.
- Adds or updates an example, and the site media in `site/media/` when the feature is visual.
- Records any new rule in `CLAUDE.md`.
- Updates `docs/migration/coverage.json` when it changes what a port can use: a row of a toolkit, or a gap that closes. The migration table of the status page is written from it: run `python3 tools/migration_matrix.py`. The tests `migration_matrix` and `migration_tools` fail when the page is out of date.

## Documentation

`docs/` is the one source of the documentation: Markdown, read on GitHub, rendered into `site/docs/` and collected into `site/llms.txt` and `site/llms-full.txt` for agents. `docs/research/` is not part of it.

- `python3 tools/docs.py check` must pass. `python3 tools/docs.py build` renders the pages (needs pandoc); then `python3 tools/check_site.py`.
- A code block that shows a file has the line `<!-- snippet: path name -->` before it and equals that part of the file. A test builds the file. `tools/docs.py sync` rewrites the blocks.
- A page names no `lv_` function, `lv_..._t` type or `LV_` name in its code. Only a page about `raw()` may, and it says `<!-- c-api: shown -->`.
- A new page goes into `docs/docs.json`, or it has no place in the sidebar and in `llms.txt`.
- One line per paragraph. A picture on a page is written by a test (see `quick_start_picture`).
- `site/docs/`, `site/llms.txt` and `site/llms-full.txt` are generated: do not commit them.

## Editor stubs

clangd cannot parse the headers, so editors read stubs: the public API as plain C++. `tools/editor/generate.py` writes the generated part from `docs/reference/*.json` and from a project's token files. `tools/editor/stubs/lv/` is the hand-written part. `lvgl26_editor_support()` in `cmake/editor.cmake` runs it for a project. `docs/editor-support.md` is the page for users.

- A change to the public API changes the stubs in the same pull request. Declare a new public name in `tools/editor/stubs`, and use it in `tests/editor/api_uses.cpp`.
- The test `editor_names` lists the public names by reflection (`tools/editor/names.cpp`) and fails for a name that the stubs or `api_uses.cpp` lack. It needs no Clang, so it runs under every preset.
- GCC compiles `tests/editor/api_uses.cpp` with the real headers, and Clang reads it with the stubs (`editor_api_uses`). Write a `static_assert` on a type there where both must agree, as for `root->get<"id">()`.
- The `editor_*` tests that read the stubs need clang++ 21 or newer, and `editor_support` runs clangd when it finds one. In CI they run in the job `editor`, in an image of its own (`.github/ci-image/editor/Dockerfile`): the main image has no Clang.
- A stub repeats a check only when the editor can show it at the user's line: a `requires` on the call, or a `consteval` constructor. `tests/editor/wrong_*.cpp` and `unknown_member.cpp` hold those cases, in the form of Clang's `-verify`.
- Every function of a template in a stub has a body, `{ __builtin_unreachable(); }` when nothing else fits. Clang warns about a function without one when its arguments have no linkage. The stub tests run with `-Werror`.
- The stubs are read by Clang only, and may use what our own code may not (`__builtin_unreachable`, an include guard in `tools/editor/fallback/contracts`).
- A new member kind or value text in the reference data needs a rule in `tools/editor/reference_stubs.py`: `editor_tools` fails when a value becomes `stub::anything`.
- A file under `examples/` that needs a build setting the editor check lacks, such as the SDL driver, says `// editor-check: skip, <reason>` in its first ten lines. The check prints it as skipped.

## Research

`docs/research/` holds what has already been looked up, with sources. Its `README.md` lists the topics. Some are about the platform: LVGL upstream, C++26, a browser target, fonts, images, editor support, components and lists. The others are the examples of other frameworks, device UI standards, the instrument cluster, 3D and effects, and documentation for agents.

- Read `docs/research/README.md` and the topic file before a web search.
- Search only for what the notes do not answer, or for a claim marked `summary` or `recalled` that the work depends on.
- Add every new finding to its topic file in the same change, with its link, its mark and the date.
- A decision made from research goes into the code, the design system or this file. The notes hold the evidence, not the rule.
- A hook in `.claude/settings.json` repeats the first three rules before every `WebSearch` and `WebFetch`.

## The plugin

The repository is a Claude Code plugin: `.claude-plugin/` holds the manifests and `skills/` the skills.

- A skill points to files of the repository with relative links. It never copies their content.
- `tools/check_plugin.py` fails when a skill links to a file that does not exist. Run it after moving a file a skill names.

## Known tool limits

- clang-tidy cannot analyze this code: Clang 23 has no reflection and no `<meta>` header.
  GCC's warnings, analyzer and sanitizers stand in for it.
- Reference screenshots are only written with `-DLVGL26_UPDATE_REFERENCES=ON`.

## Git

Feature branches, atomic conventional commits, ASCII only in commit messages.
