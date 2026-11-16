# lvgl26

A C++26 framework over LVGL. Namespace `lv`. GCC 16 or newer only: the code uses reflection.

## Build and test

```sh
cmake --preset host-v96 && cmake --build --preset host-v96 && ctest --preset host-v96
```

| Preset | What it checks |
|---|---|
| `host-v96`, `host-v10` | LVGL v9.6.0 and the v10 pin, warnings as errors |
| `host-v96-asan`, `host-v10-asan` | AddressSanitizer and UndefinedBehaviorSanitizer |
| `host-v96-tsan` | ThreadSanitizer (Linux only; GCC has no TSan on macOS) |
| `host-v96-analyze` | GCC static analyzer (`-fanalyzer`) |
| `host-v96-coverage` | Coverage; report with `gcovr --filter include/lv/ build/host-v96-coverage` |

Every change must pass `host-v96` and `host-v10` locally. Run the tests side by side:
`ctest --preset host-v96 -j 8`.

| Where | What runs |
|---|---|
| Every pull request and push to main (`ci.yml`) | `host-v96`, `host-v10`, `host-v96-asan`, `host-v96-analyze`, the compile-fail tests on both LVGL versions, ThreadSanitizer on the threaded tests, CRAP, one minute of fuzzing |
| Every night (`nightly.yml`) | `host-v10-asan`, `host-v96-tsan` on every test, half an hour of fuzzing, the slice on an Arm Cortex-M4 in QEMU (`tools/arm-run.sh`). A failure opens an issue |

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
- Views never delete widgets. Only `lv::mount` and `lv::owned` own.
- Styles take design tokens only. Add the token to the token file; use `lv::raw(...)` only for a
  reasoned exception. Use semantic or component tokens in UI code, not primitives.
- A look shared by all widgets of a kind is a theme (`lv::theme`, `include/lv/theme.hpp`), given to
  the display. A theme's styles are built in constant evaluation with `std::define_static_array`,
  so they need no template argument and no heap. `include/lv/signal/*.json` is written by
  `tools/signal_tokens.py` from `design/signal/tokens.design.json`: change the design file, then
  run the tool.
- A font in a style comes from a typography token through `lv::typeface`; `lv::raw(font)` marks one
  that no token names. `include/lv/detail/coverage.hpp` is derived from font-rs and stays under the
  Apache License 2.0: keep its notice, and list any other borrowed part in `NOTICE.md`.
- A part of LVGL that costs the device code or heap is behind a CMake option of its own, off by
  default (`LVGL26_TTF`, `LVGL26_PNG`, `LVGL26_JPEG`). The tests build with them. Its C functions are found by reflection
  (`detail::find`), and a `static_assert` names the option when they are missing.
- `-fconstexpr-ops-limit` is raised for every user of lvgl26 (`CMakeLists.txt`), because the compiler
  draws fonts. A single loop in constant evaluation must still stay under 262144 rounds.
- Code that runs in the compiler is written for its interpreter: look codes up in tables, give a
  buffer its size once and write through pointers, avoid a call for each byte
  (`include/lv/detail/png_reader.hpp` needs an eighth of the work of its first form).
- A large result of constant evaluation is stored with `std::define_static_array`: returning a
  `std::array` of 200 KB costs several times the decoding. Its data becomes part of a symbol name,
  and Apple's linker refuses a very long one, so sizes are limited (`compile_time_image_pixels`).
- `docs/reference/lvgl-<version>.json` is the reference as data: widgets, properties, what each takes,
  enumerator names, style members. `tools/reference/reference.cpp` prints it by reflection, and the
  test `reference_data` fails when a committed file is stale. Write it again with the program the
  test names; never edit it by hand.
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
- Anything a header does at namespace scope costs compile time in every file. Keep reflection
  scans lazy: put them behind a template parameter (see `props_of` in `include/lv/props.hpp`).
- Inside a template, name what is generated for one widget through a template parameter
  (`members_of_props<"obj", D>`, `getter_table<"obj", D>`), never plainly. A name that depends on
  nothing is generated where the template is read: in every file that includes the header.
- GCC 16.2 runs a call whose arguments are all constants twice when it is written inside a
  function template: once when the template is instantiated, once when the function runs. Hand
  the result of a scan in as an argument (see `scan_base` in `include/lv/props.hpp`).
- A node of a tree keeps `detail::kept<W>`, not `props<W>`. A props struct has thousands of
  members, and the compiler walks every one each time it copies a node.
- `lv::arg` and what it holds have no default member initializer, so that an unset property is
  all zeros. A constructor to run there is paid for every unset member of every node.
- `tests/large_tree.cpp` is compiled under a fixed `-fconstexpr-ops-limit`. It stops compiling
  when a node becomes expensive for the compiler again. Raise the limit only with a measurement.
- Measure before claiming: compile time with `/usr/bin/time`, heap with `lv_mem_monitor`,
  pixels with `lv_test_screenshot_compare`.
- Compare compile times in instructions, not in seconds: on a busy machine the same compile takes
  several times longer. `/usr/bin/time -l` prints "instructions retired", but for the process it
  starts, so start `cc1plus` itself: `g++ -###` prints its command.

## Test adequacy

| Check | How | Limit |
|---|---|---|
| Complexity | `lizard`, through `tools/crap.py` | 8 for every function, also compile-time ones |
| CRAP | `tools/crap.py build/host-v96-coverage` after the coverage preset | 8 for every function that runs at run time |
| Mutation | `tools/mutation.py <header> --tests '<ctest -R>' --since origin/main` | Every survivor gets a test, or a reason |
| Fuzzing | `fuzz/json_reader.cpp` (libFuzzer, Clang); `fuzz/runtime_document.cpp` and `fuzz/mcp_message.cpp` (AFL++ with its GCC plugin: `fuzz/afl.sh`) | Every free input has a target; every crash becomes a corpus file |

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

## Definition of done for a feature

A feature pull request also:

- Moves the feature's line from "planned" to "works today" in `site/status.html` (the comment at the top of that file says how).
- Adds the feature to the feature list in `site/index.html` and in `README.md`.
- Adds or updates an example, and the site media in `site/media/` when the feature is visual.
- Records any new rule in `CLAUDE.md`.

## Documentation

`docs/` is the one source of the documentation: Markdown, read on GitHub, rendered into `site/docs/` and collected into `site/llms.txt` and `site/llms-full.txt` for agents. `docs/research/` is not part of it.

- `python3 tools/docs.py check` must pass. `python3 tools/docs.py build` renders the pages (needs pandoc); then `python3 tools/check_site.py`.
- A code block that shows a file has the line `<!-- snippet: path name -->` before it and equals that part of the file. A test builds the file. `tools/docs.py sync` rewrites the blocks.
- A page names no `lv_` function, `lv_..._t` type or `LV_` name in its code. Only a page about `raw()` may, and it says `<!-- c-api: shown -->`.
- A new page goes into `docs/docs.json`, or it has no place in the sidebar and in `llms.txt`.
- One line per paragraph. A picture on a page is written by a test (see `quick_start_picture`).
- `site/docs/`, `site/llms.txt` and `site/llms-full.txt` are generated: do not commit them.

## Research

`docs/research/` holds what has already been looked up, with sources. Its `README.md` lists the topics: LVGL upstream, C++26, the examples of other frameworks, device UI standards, the instrument cluster, 3D and effects, a browser target, documentation for agents, fonts, images, editor support.

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
