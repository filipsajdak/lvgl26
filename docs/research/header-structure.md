# What belongs in `<lv/lv.hpp>`

Research and a proposal, written 2026-11-19, after `lv/one_of.hpp` left `<lv/lv.hpp>` in pull request 123. Measured again and decided on 2026-11-20, when the split was built.

Marks follow `docs/research/README.md`.
A mark is dated 2026-11-19 unless it says otherwise.

The owner's question: "I think lv/lv.hpp should have only headers that are required (limited set) we don't need everything added there. Maybe it would be better to make explicit header like lv/lv-all.hpp or some other name that indicates that it has everything and user than might expect longer builds. Check best practices from other projects."

## Short answers

| Question | Answer | Evidence |
|---|---|---|
| What did the split give, measured on 2026-11-20? | The core costs 9.01 G against 13.00 G for the old `<lv/lv.hpp>` (−31 %); with one label 42.07 G against 45.82 G (−8 %) | Measurement (iii) |
| What does each opt-in header add now? | `look` 3.28 G, `font` 1.97, `image` 0.44, `runtime` 0.31, `theme` 0.18, `repeat` 0.18, `encoded_image` 0.17, `list` 0.17, `scaled_font` 0.12, `pointer` 0.07, `one_of` 0.04 | Measurement (iii) |
| Can the compiler name the missing header? | For templates and functions, yes, at no measurable cost; for a plain class, no. A build-time check covers every name | "Naming the missing header" |
| What did `<lv/lv.hpp>` cost on 2026-11-19, before the lazy scans? | 37.7 G instructions, against 17.7 G for the smallest core a program needs | Measurement (i) |
| What does a file pay for `<lv/lv.hpp>` today? (2026-11-19) | 37.7 G instructions, against 17.7 G for the smallest core a program needs | Measurement (i) |
| Which header costs the most of it? | `lv/theme.hpp`: 14.2 G on top of the core, for every file, also one that names no theme | Measurement (i) |
| Why? | It scans the global namespace at namespace scope (`theme_members`, `theme_classes`), so the scan runs where the header is read | `include/lv/theme.hpp`, lines 74 to 141, `source` |
| What do the other headers cost? | `look` 3.3 G, `font` 2.1, `scaled_font` 0.8, `encoded_image` 0.7, `image` 0.4, `list` 0.4, `repeat` 0.4; the rest 0.2 or less | Measurement (i) |
| And the headers outside `<lv/lv.hpp>`? | `signal_type` 168 G, `signal` 60 G, `host` 3.6 G, `ui_scheduler` 2.9 G, `mcp` 1.3 G, `png` 0.5 G | Measurement (i) |
| What do other libraries do? | Keep fine-grained headers and an umbrella beside them; the build-time wins came from a smaller common header (fmt, Catch2) | "Other libraries" |
| Does `import lv;` make the question go away? | Not yet: our spike halved a test's compile but lost 35 of 118 compile-fail messages, and boost-ext/ut's module is off by default and still has open issues | "Modules" |

## Measured here

### (i) The cost of each header

GCC 16.2 on macOS arm64, the flags of `tests/smoke.cpp` in the `host-v96` build of `main` at `b67550f`.
Each file includes the core and one header, and has an empty `main`.
`tools/compile_report.py`, three runs of every file, the minimum in billions of instructions; the spread of the three runs was at most 0.3 G.
The core is `lv/application.hpp`, `lv/display.hpp`, `lv/state.hpp` and `lv/tree.hpp`: a tree, a state and a display, the quick start without a theme.

The last three columns count the files that name what the header provides (`lv::theme`, `lv::look`, ...): 91 example files outside the ports, 43 files of the ports, 77 run-time tests.
The count is a search for names, so it is a guide, not an exact number.

| Header | What it provides | On top of the core (G) | Examples | Ports | Tests |
|---|---|---|---|---|---|
| core | `lv::application`, `lv::display`, `lv::mount`, the factories, `lv::state`, units, styles | 17.66 alone | | | |
| `view.hpp` alone | Typed views, properties | 15.31 alone | | | |
| `style.hpp`, `options.hpp`, `match.hpp` | Constant styles; `lv::options_of`; `lv::match` | 0.0: already in the core | 0, 3, 0 | 0, 0, 1 | -, 1, 1 |
| `owned.hpp` | `lv::owned`, `lv::weak` | 0.06 | 0 | 16 | 4 |
| `task.hpp` | `lv::task`, `lv::spawn`, `lv::sleep` | 0.08 | 2 | 14 | 6 |
| `animate.hpp` | `lv::animate` | 0.14 | 0 | 3 | 1 |
| `computed.hpp` | `lv::computed`, `lv::effect` | 0.18 | 1 | 4 | 6 |
| `tokens.hpp` | `lv::tokens<file>` | 0.20 | 7 | 2 | 9 |
| `runtime.hpp` | `lv::runtime`, a screen from a document | 0.33 | 0 | 0 | 5 |
| `list.hpp` | `lv::list`, `lv::each` | 0.37 | 2 | 3 | 4 |
| `repeat.hpp` | `lv::for_each`, `lv::for_each_field` | 0.41 | 2 | 0 | 3 |
| `image.hpp` | Images decoded by the compiler | 0.44 | 1 | 7 | 2 |
| `encoded_image.hpp` | Images decoded on the device | 0.70 | 0 | 1 | 1 |
| `scaled_font.hpp` | `lv::scaled_font` | 0.82 | 1 | 0 | 1 |
| `font.hpp` | Fonts made by the compiler, `lv::typeface` | 2.06 | 3 | 2 | 5 |
| `look.hpp` | `lv::look`, `lv::design` | 3.33 | 1 | 0 | 5 |
| `theme.hpp` | `lv::theme` | 14.15 | 1 | 0 | 5 |
| all of `<lv/lv.hpp>` | | 20.03 | | | |
| `png.hpp` (outside) | `lv::write_png` | 0.46 | 7 | 2 | 2 |
| `mcp.hpp` (outside) | The agent kit's server | 1.31 | 1 | 0 | 1 |
| `ui_scheduler.hpp` (outside) | `lv::ui_scheduler`, threads | 2.93 | 0 | 0 | 1 |
| `host.hpp` (outside) | `lv::host`, the operating system's clock | 3.62 | 0 | 2 | 1 |
| `signal.hpp` (outside) | Signal's themes | 59.68 | 4 | 0 | 2 |
| `signal_type.hpp` (outside) | Signal's fonts and themes with fonts | 168.26 | | | |
| everything public | `<lv/lv.hpp>` and the seven headers outside | 176.70 | | | |

What the numbers say.

- Two fifths of `<lv/lv.hpp>` is the core; most of the rest is one header, `theme.hpp`.
- `theme.hpp` breaks the rule of `CLAUDE.md`: "Keep reflection scans lazy: put them behind a template parameter". Its tables `theme_members` and `theme_classes` are evaluated where the header is read.
- Made lazy, `theme.hpp` would likely cost what `look.hpp` costs or less; that is to be measured, not assumed.
- Most headers cost under 0.5 G, about 1 to 3 percent of the core each.
- The everything header of this repository is dominated by Signal's fonts, which are drawn in every file that includes them.

### (ii) A missing include as a compile error that names the header

A deleted fallback in the core can say where a name lives, without including the header:

```cpp
template <int = 0> void theme(detail::needs_header, ...) = delete("lv::theme is in <lv/theme.hpp>: add #include <lv/theme.hpp>");
```

A sketch with GCC 16.2, `measured`.

| Call | Without the header | With the header |
|---|---|---|
| `lv::theme(lv::spec{1})` | The message above | The real function is called |
| `lv::theme({.a = 1})`, a braced list | "no matching function" with the template form; the message with a plain function | The braced list then also picks the fallback, or nothing: the overloads interfere |

- So the message works for an argument of a named type, and needs care, and a test per name, for a braced list, which is how `lv::theme({...})` is called.
- A class template (`lv::one_of`, `lv::font`) cannot get such a message: a declaration in the core gives "incomplete type", which names the class, not the header.
- A variable template (`lv::tokens<file>`) is the same as a class.

### (iii) The cost of each header, after the lazy scans (2026-11-20)

GCC 16.2 on macOS arm64, the flags of the tests in the `host-v96` build of `main` at `dd70e19`. The lazy API scan, the lazy theme and the first-widget fixes had landed, so the numbers of (i) no longer hold. Instructions of `cc1plus`, minimum of three runs; the spread was at most 0.19 G. `measured`

| File | G | On top of the core |
|---|---|---|
| The old `<lv/lv.hpp>`, include only | 13.00 | |
| The core, include only | 9.01 | |
| The old `<lv/lv.hpp>` with one mounted label | 45.82 | |
| The core with one mounted label | 42.07 | |
| `look.hpp` | | 3.28 |
| `font.hpp` | | 1.97 |
| `image.hpp` | | 0.44 |
| `runtime.hpp` | | 0.31 |
| `theme.hpp` | | 0.18 |
| `repeat.hpp` | | 0.18 |
| `encoded_image.hpp` | | 0.17 |
| `list.hpp` | | 0.17 |
| `scaled_font.hpp` | | 0.12 |
| `pointer.hpp` | | 0.07 |
| `one_of.hpp` | | 0.04 |
| `signal.hpp` (outside) | | 51.11 |
| `signal_type.hpp` (outside) | | 159.31 |
| `host.hpp` (outside) | | 4.10 |
| `ui_scheduler.hpp` (outside) | | 3.22 |
| `mcp.hpp` (outside) | | 1.15 |
| `png.hpp` (outside) | | 0.44 |

- No core header includes an opt-in header; `look.hpp` includes `font.hpp`, `list.hpp` and `repeat.hpp`, and `repeat.hpp` includes `list.hpp`. `source`
- One link went the other way: a style that follows an `lv::scaled_font` called a function that only `scaled_font.hpp` defined, so a program without that header did not link. The font now carries the function as its first member (`detail::font_resolver`). `measured`
- A file that includes only the core needs 12,000 to 14,000 operations in its largest evaluation, against 25,000 to 26,000 before. `measured`

### (iv) Naming the missing header (2026-11-20)

The owner's question: "research all possible options to tell the user what the error is not to make him/her search for it for hours. Maybe some compile-time/build-time check could do the job? If not possible maybe cmake can handle?"

Each option was tried on GCC 16.2 and clangd 23, with the real `lv::list`, `lv::look` and `lv::theme` in a copy of the headers, and with small stand-ins for the other shapes.

| Option | What the user sees without the header | Where it works | Cost | Mark |
|---|---|---|---|---|
| A primary template in the core with a `static_assert`, and the real one a constrained partial specialization (class or variable template) | `error: static assertion failed: lv::list is in <lv/list.hpp>: add #include <lv/list.hpp>` | `lv::list`, `lv::look`; tests `lists`, `look` compile unchanged | 36 such declarations: no measurable change, 9.00 to 8.99 G include only, 41.95 to 41.97 G with a label | `measured` |
| A deleted fallback of a function, guarded by "the header is not included" | A call with arguments: the message on the error line. A braced list (`lv::theme({...})`): `no matching function`, and the candidate note quotes the fallback's line, which holds the message | `lv::theme`; tests `theme`, `signal` compile. Unguarded, the fallback took the real call and `theme.cpp` broke | as above | `measured` |
| The same for a class that is no template (`lv::scaled_font`, `lv::pointer`) | Not possible: a class cannot be declared twice | | | `source` |
| GCC's own hint | GCC suggests a header only for names of `std`, from a built-in table | `get_std_name_hint` in `gcc/cp/name-lookup.cc`, `gcc/c-family/known-headers.cc`, `releases/gcc-16.2.0` | | `source` |
| A check before the build: `tools/check_includes.py` with the generated table | `main.cpp:12: lv::list needs #include <lv/list.hpp>` | Every name, also plain classes; follows the user's quoted includes; skips comments and strings | 0.08 s for 539 files | `measured` |
| The editor stub of `lv/lv.hpp` with the same fallback | clangd at the user's line: `static assertion failed due to requirement 'detail::header_included<row>': lv::list is in <lv/list.hpp>` | Every name, since the stubs may declare a plain class too | | `measured` |
| clangd without a fallback | `no template named 'list' in namespace 'lv'`; its include fixer may offer the header from a background index, not seen in `clangd --check` | | | `measured` |
| The page "Which header" in the docs and in `llms.txt` | The name, its header, and the compiler messages that mean "include it" | Agents and readers | | `source` |

- In the compiler, every opt-in header must include the core's declarations first, because a file may include `lv/list.hpp` before `lv/lv.hpp`. `measured`
- A guarded fallback keeps the compile-fail messages of the real functions; an unguarded one replaces them. `measured`

### (v) The check and the quick fix, built (2026-11-20)

The owner chose the check at build time, the editor and the page, and the fallbacks in the compiler for a later pull request.

| What | Result | Mark |
|---|---|---|
| The check found with no line of the program's own | A call deferred to the top directory (`cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} ...)`) finds every target that links lvgl26, also one made before lvgl26 was added and one that links it through an interface library | `measured` with `add_subdirectory` and `FetchContent`, Ninja and Unix Makefiles |
| What it says | `app/parts.hpp:4: lv::list needs #include <lv/list.hpp>`, for a name in a header of the program that a source includes with quotes | `measured` |
| What it checks again | Nothing when nothing changed (`ninja: no work to do`); one file when the file or a header it includes changed, through a depfile | `measured` |
| What it costs | 518 files of the repository: 28 s of CPU when all are checked, against 2160 s for the build; about 34 ms a file, nearly all of it Python starting. With Ninja the files compile while they are checked and only the link waits, so a full build took 425 s with it and 427 s without, within the noise | `measured`, one run each |
| A generated source | Not checked: it does not exist when CMake runs | `measured` |
| clangd's quick fix | `Include <lv/list.hpp> for symbol lv::list`, also for a plain class (`lv::scaled_font`) and for `lv::theme({...})`, once its background index holds a file that includes `lv/all.hpp` and the stub directory is on its search path | `measured` with clangd 23 and the stubs |
| Without those two | No fix: the index had no file with the opt-in headers, and then could not write the header's name | `measured` |

## Other libraries

The rows of the first table were read by a fetch agent and checked by its links; marks as it gave them.

| Library | What it does | Link | Mark |
|---|---|---|---|
| Boost.Asio | Header-only by default, separate compilation opt-in (`BOOST_ASIO_SEPARATE_COMPILATION`); the page says nothing on `asio.hpp` and compile time | [using.html](https://www.boost.org/doc/libs/release/doc/html/boost_asio/using.html) | `source` |
| Boost.Asio | Its author: each header compiles alone, and narrow includes cut compile time | [2005 list mail](https://listarchives.boost.org/Archives/boost/2005/10/95554.php) | `summary` |
| libstdc++ | `bits/stdc++.h` includes all standard headers, meant as the base of a precompiled header | [using_headers.html](https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_headers.html) | `source` |
| C++23 | `import std;` and `import std.compat;` replace the umbrella; the paper cites throughput without numbers | [P2465R3](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2465r3.pdf) | `source` |
| NanoRange | `<algorithm>` 0.44 s against 1.2 to 3.4 s for range libraries | [Compile-times wiki](https://github.com/tcbrindle/NanoRange/wiki/Compile-times) | `summary` |
| Qt | The module pages give no advice; a forum answer says module includes such as `<QtWidgets>` were never recommended, and per-class headers like `<QPushButton>` are | [Qt Widgets](https://doc.qt.io/qt-6/qtwidgets-index.html), [forum](https://forum.qt.io/topic/18279/including-qtgui-or) | `source` (silent), `summary` |
| range-v3 | The documentation says to include individual headers; `range/v3/all.hpp` exists | [range-v3 docs](https://ericniebler.github.io/range-v3/) | `source`; the file name `recalled` |
| {fmt} 11.0 | `fmt/base.h` is the minimal subset; 100 files compile in 4.8 s against 19.0 for fmt 10, 25.9 for IOStreams, 1.6 for printf; `format.h` is now about as fast as the old `core.h` | [11.0.0 release](https://github.com/fmtlib/fmt/releases/tag/11.0.0) | `source` |
| Eigen | One header per module; `Dense` is Core with Geometry, LU, Cholesky, SVD, QR and Eigenvalues; `Eigen/Eigen` adds Sparse | [Quick reference](https://libeigen.gitlab.io/eigen/docs-nightly/group__QuickRefPage.html) | `source` |
| Catch2 v3 | Split the single header for compile time, about 80 percent less inclusion overhead; `catch_all.hpp` is the migration step and "will cause significant compile time drag" | [migrate-v2-to-v3](https://github.com/catchorg/Catch2/blob/devel/docs/migrate-v2-to-v3.md), [release notes](https://github.com/catchorg/Catch2/blob/devel/docs/release-notes.md) | `source` |
| nlohmann/json | `json_fwd.hpp` declares `basic_json` without defining it | [json_fwd.hpp](https://github.com/nlohmann/json/blob/develop/include/nlohmann/json_fwd.hpp) | `source` |
| Google style | Include what you use; do not rely on what another header includes | [Include What You Use](https://google.github.io/styleguide/cppguide.html#Include_What_You_Use) | `source` |

### boost-ext/ut

Read from a shallow clone of `master` at `09e8a3e` (2026-11-19), and through the GitHub API; nothing was run.

| Finding | Link | Mark |
|---|---|---|
| One header, `include/boost/ut.hpp`, 4,211 lines, and one module file, `include/boost/ut.cppm` | [include/boost](https://github.com/boost-ext/ut/tree/master/include/boost) | `source` |
| The module is the header: `export module boost.ut; import std;` then `#define BOOST_UT_CXX_MODULES 1` and `#include "boost/ut.hpp"`, plus explicit instantiations of the reporter | [ut.cppm](https://github.com/boost-ext/ut/blob/master/include/boost/ut.cppm) | `source` |
| Under `BOOST_UT_CXX_MODULES` the header leaves out its standard includes and marks its API `export` through `BOOST_UT_EXPORT` | [ut.hpp](https://github.com/boost-ext/ut/blob/master/include/boost/ut.hpp) | `source` |
| The user writes `#include <boost/ut.hpp>` or `import boost.ut;`, and links `Boost::ut` or `Boost::ut_module` | [README](https://github.com/boost-ext/ut/blob/master/README.md) | `source` |
| The CMake option `BOOST_UT_DISABLE_MODULE` is `ON` by default; the module needs CMake 4.0, C++23 and `CXX_MODULE_STD` | [CMakeLists.txt](https://github.com/boost-ext/ut/blob/master/CMakeLists.txt) | `source` |
| `BOOST_UT_DISABLE_MODULE` is passed to users as a definition, but the header never reads it: the option only decides whether the module target is built | same, and `ut.hpp` | `source` |
| The other options are for its own build: tests, examples, benchmarks, coverage, sanitizers, warnings as errors, memcheck, install. None switches a part of the header off | same | `source` |
| The header includes 19 standard headers, `<iostream>`, `<sstream>`, `<fstream>` and `<unordered_map>` among them, and `<format>` when present | `ut.hpp`, lines 72 to 105 | `source` |
| Its README names four techniques for fast compiles: no unique type per lambda (a test is `void (*)()`, "compiles 5x faster"), type erasure so that instantiations are shared, a single header with few includes, and simplified `std::function` and `std::string_view` | [README, fast compilation times](https://github.com/boost-ext/ut#fast-compilation-times) | `source`; the "5x" is the README's claim |
| Its benchmark is from 2019-12, ut 1.1.0, GCC 9 and Clang 9, wall time of one `time -f %e` run | [ut-benchmark Makefile](https://github.com/cpp-testing/ut-benchmark/blob/master/benchmarks/Makefile) | `source` |
| Include only, GCC 9: ut 0.39 s, GoogleTest 0.83, Boost.Test 0.99, doctest 1.73, Catch2 8.53 | [results/gcc9.bench](https://github.com/cpp-testing/ut-benchmark/blob/master/results/gcc9.bench) | `source` |
| 1,000 tests in one file: ut 1.44 s, doctest 2.42, GoogleTest 6.90, Boost.Test 8.88, Catch2 9.57; 10,000 tests in 100 files: ut 13.26, doctest 8.01, Catch2 84.96 | same | `source` |
| No benchmark of the module against the header was found | the README and the repository | `source` (absence) |
| Issues about the module: it fails to build or import (#650, 2024; #713, 2026; #697, 2025), MSVC refuses it (#558, #604), a test suite with the module fails (#697); a collaborator answered in #713 that "it's typically more a problem of the compiler and toolchain than the project itself" | [#713](https://github.com/boost-ext/ut/issues/713), [#650](https://github.com/boost-ext/ut/issues/650), [#558](https://github.com/boost-ext/ut/issues/558) | `source` |
| No open or closed issue is about the compile time of the header | issue search "compile time" | `source` (absence) |

What ut means for us.

1. **The core against an everything header.** ut is one header because a test framework is one thing; it stays cheap by what it does not do (unique types, eager work), not by splitting. Our expensive part is eager work at namespace scope (`theme.hpp`), which is the same lesson.
2. **`import lv;`.** ut ships its module from the header source with one macro, which is the shape our spike's `lv.cppm` already has. Its module is off by default seven years on, and its issues are about toolchains. Our spike measured a real gain (a test's compile 18.95 s to 9.31 s, a compile-fail test 8.15 s to 1.80 s), but lost 35 of 118 compile-fail messages on GCC 16.2. A module would shrink the cost of the core; it would not settle what the core is, and it cannot be the default yet.
3. **Techniques we could measure.**
   - A handler is already a function pointer in a constexpr tree (`CLAUDE.md`), as ut's test is.
   - ut erases types so that instantiations are shared (`eq<int, int>`, not one per value). Ours are per tree and per path by design; whether parts of `apply_node` could be shared across nodes of the same widget kind is measurable on `large_tree`.
   - ut keeps its header to its standard includes; we could measure what `<meta>` and our standard includes cost alone, as Romeo's benchmark says most of reflection's cost is `<meta>` (`cpp26.md`, `summary`).

## What the others learned

- An umbrella header lives beside fine-grained ones, as a convenience and a migration step; none of them made it the only door (Catch2, Eigen, range-v3, Qt, Boost).
- The build-time wins came from making the common header small (`fmt/base.h`, Catch2 v3), not from removing the umbrella.
- The everything header has a name that says so: `catch_all.hpp`, `range/v3/all.hpp`, `Eigen/Eigen`, `<QtWidgets>`; the standard has `bits/stdc++.h` for a precompiled header only.
- Several pages say nothing on build time; the evidence for "include what you use" is style guides and maintainers, not measurements.

## Proposal

### The required core

`<lv/lv.hpp>` keeps what nearly every program writes, and what costs nothing extra once a tree is mounted.

| In the core | Why |
|---|---|
| `application`, `display`, `view`, `tree`, `state`, `style`, `units`, `values` | Every program; the quick start |
| `owned`, `task`, `match`, `options`, `computed`, `animate` | 0.2 G or less each; common in ports |
| `tokens` | 0.2 G; a program with a design names its tokens |

| Opt-in, each its own include | On top of the core (G) |
|---|---|
| `lv/theme.hpp` | 14.2 today; to be measured again after its scan is made lazy |
| `lv/look.hpp` | 3.3 |
| `lv/font.hpp` | 2.1 |
| `lv/scaled_font.hpp`, `lv/encoded_image.hpp`, `lv/image.hpp` | 0.8, 0.7, 0.4 |
| `lv/list.hpp`, `lv/repeat.hpp`, `lv/runtime.hpp` | 0.4, 0.4, 0.3 |
| `lv/one_of.hpp` (already), `lv/png.hpp`, `lv/signal.hpp`, `lv/signal_type.hpp`, `lv/host.hpp`, `lv/ui_scheduler.hpp`, `lv/mcp.hpp` (already outside) | |

### The cost a file pays under each option

Include-only files, from measurement (i); the last two rows are estimates from the same table.

| Option | `<lv/lv.hpp>` | The everything header | A file of the quick start |
|---|---|---|---|
| Today | 37.7 G | none exists; all public headers 194.4 G | 37.7 G, plus `signal.hpp` |
| H1 A small core, an everything header beside it | 17.7 to 18.5 G | about 37.7 G without Signal | 17.7 G, plus `signal.hpp` |
| H2 H1, and `theme.hpp` made lazy first | 17.7 to 18.5 G | about 24 G, if the scan becomes as cheap as `look.hpp` | The same |
| H3 Keep `<lv/lv.hpp>` as today, add `lv/core.hpp` (the fmt way) | 37.7 G | `<lv/lv.hpp>` itself | 17.7 G, if it includes `lv/core.hpp` |

Recommended: H2.

- The 14 G of `theme.hpp` is a defect against our own rule, and its fix lowers every option.
- After it, a small core with an explicit everything header keeps `<lv/lv.hpp>` for the reader of the quick start and makes the cost of each feature visible in its include.
- H3 keeps every existing include working, but every new user pays for everything by default, which is what the owner asked to avoid.

### The name of the everything header

| Name | For | Against |
|---|---|---|
| `lv/lv-all.hpp` (the owner's) | Says "all", next to `lv.hpp` | The only hyphen among our header names, which use `_` |
| `lv/all.hpp` | range-v3's name; short | "all" without the library name in an `#include` line reads as generic |
| `lv/everything.hpp` | Cannot be misread | Long |
| `lv/lv_all.hpp` | Our spelling with `_`, as `signal_type.hpp` | Repeats `lv` |

Recommended: `lv/all.hpp`; the directory already names the library, as in `range/v3/all.hpp` and `catch2/catch_all.hpp`.
It includes the core and every opt-in header of the library, but not Signal (the design system of the showcase, 228 G with its fonts), the host clock, threads or the agent kit, which are platforms and tools.

### What the docs, the quick start and the examples include

- The quick start: `<lv/lv.hpp>` and `<lv/signal.hpp>`, as today.
- A guide page names the header of its feature at its first snippet: "`#include <lv/list.hpp>`", as `docs/mounting.md` does for `lv/one_of.hpp`.
- An example includes what it uses; `tools/check_examples.py` can check that an example includes the header of each `lv::` name it uses, from a table of names per header, so no example relies on `lv/all.hpp`.
- The ports may include `lv/all.hpp`: they show a whole application, and their cost is measured on their own.

### What a missing include looks like

- For a function called with an argument of a named type, a deleted fallback in the core can name the header (measurement (ii)).
- For a braced list, a class or a variable template, the user sees "was not declared" or "incomplete type". The docs then say which header, and the editor's stubs complete the name only after the include, as the real headers do.
- Recommended: no fallbacks for now; a page "Which header" in the docs, listed by name, generated from the same table as the check of the examples.

### The editor stubs and `import lv;`

- The stubs mirror the headers one to one (`tools/editor/stubs/lv/`); their `lv.hpp` follows the core, and a stub `all.hpp` includes the same set as the real one. `tools/editor/inventory.py` lists the headers it reads; it reads `all.hpp`.
- `import lv;` would export the core first; a second module, or a partition per opt-in header, follows the same split. ut's shape (the module includes the header under a macro) keeps the two forms one source.

## Decided by the owner (2026-11-20)

1. `<lv/lv.hpp>` is the core of the table "The required core".
2. The opt-in headers are theme, look, font, scaled_font, image, encoded_image, list, repeat, runtime, one_of and pointer.
3. The everything header is `lv/all.hpp`. It includes the core and the opt-in headers, and not Signal, `host.hpp`, `ui_scheduler.hpp`, `mcp.hpp` or `png.hpp`.
4. The docs and the quick start include `lv/all.hpp`; every other example, the ports too, includes exactly what it uses, because "examples should always promote the best practices of using the framework".
5. Which header declares which name is generated, by reflection, and the examples' check and the page "Which header" read it.
6. How the user learns the missing header is open: the options are in (iv).

## Not confirmed

- What `theme.hpp` costs once its scan is lazy: the 24 G of H2 was an estimate; (iii) measured 0.18 G on 2026-11-20.
- The counts of users per header: a search for names, not an include analysis.
- That the missing-include fallback works for every call form; it was sketched for one function.
- Compile cost on Linux or under a sanitizer: every number is from the `host-v96` flags on macOS.
