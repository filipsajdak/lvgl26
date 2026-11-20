# Compile time: where it goes, and what to fix

Why one file of lvgl26 costs GCC 16.2 so much, measured part by part, with the fixes ranked by what they save.
Measured on 2026-11-19; every mark below has that date, except in "GCC built from source", which was measured on 2026-11-20.
The marks are in [the README](README.md).
[compile-time.md](compile-time.md) holds the tools and the earlier levers; this note does not repeat them.

## Short answers

| Question | Answer | Mark |
|---|---|---|
| Is it parsing? | No. Parsing every header is 9 to 11 G instructions. A file with one mounted label costs 54 G. | `measured` |
| What costs the rest? | The first widget of a file: about 35 G of scans and generated structs, which every file runs again. | `measured` |
| What does each further thing cost? | A new widget kind 5.6 G, a node 0.2 G, a token file 6.8 G, a font drawn 45 G, a theme 9 G. | `measured` |
| Where is GCC's time? | 55 percent in constant evaluation, and 86 percent of the samples of a scan in the constant-expression interpreter. | `measured` |
| What do small code fixes give? | Five fixes keep every output and cut 14 to 23 percent from files that use widgets or a theme. | `measured` |
| What gives most? | A precompiled header that has done the scans once: a one-label file goes from 54 G to 6 G. | `measured` |
| Does the branch `perf/lazy-api` already fix this? | Only for a file that names no widget: 23.0 G to 13.0 G. A file with a widget runs the same scan later. | `measured` |
| Can GCC itself charge less? | Yes. Four patches to 16.2 cut a quarter from one label; trunk is already 7 % cheaper. See "GCC built from source" | `measured` |

## How it was measured

- GCC 16.2.0 (Homebrew), macOS arm64, the flags of the `host-v96` build: Debug, `-g`, all warnings, `-fno-analyzer`.
- The number is "instructions retired" of `cc1plus` itself, from `/usr/bin/time -l`, as `CLAUDE.md` says. `g++ -###` printed the command.
- "Front end" is `-fsyntax-only`. "Full" is `-c` with the assembler output thrown away. The back end is the difference.
- Two trees: `main` at `fc6615a` and `perf/lazy-api` at `66fc599`, which is `main` plus a lazy API table.
- The headline rows and the fixes were measured again on `main` at `8c1aaeb`, after the settings (#144). They moved by less than the noise.
- The probes are small files in a scratch directory outside the repository. Each adds one thing to the one before it.
- One run per number unless a range is given. Runs of the same file differed by up to 0.8 G.
- The share of each phase comes from `-ftime-report`, read as row divided by `TOTAL`. Its rows overlap: parsing contains evaluation and instantiation.
- The profile is `sample` on `cc1plus`, 1 ms interval, summed per GCC function.
- A fix was tried in a copy of `include/` and measured against the same probe. Nothing in `include/` of this branch changed.
- A fix counts only if the output is unchanged. The reference data that `tools/reference/reference.cpp` prints was byte-identical with all fixes. `large_tree`, `theme`, `look`, `runtime_ui` and `signal` built and passed.

## Attribution: one mounted label

The file includes `<lv/lv.hpp>`, makes `lv::label({.text = "t0"})` inside an `lv::column`, and mounts it. `perf/lazy-api`, 53.9 G full.
Each row is the cost that one more line of the probe adds, so the rows add up to the file.

| Part | G | Share | Mark |
|---|---|---|---|
| libstdc++ headers, `<meta>` among them | 8.1 | 15 % | `measured` |
| LVGL's headers | 0.8 | 1.5 % | `measured` |
| Our headers, parsed, with their namespace-scope work | 1.8 | 3.4 % | `measured` |
| The API scan: `members_of(^^::)` and the `lv_` filter (`detail::api_table`) | 9.7 | 18 % | `measured` |
| The style table: 139 style setters (`detail::style_table`) | 7.7 | 14 % | `measured` |
| The style struct `style_t` | 2.1 | 4 % | `measured` |
| The members every widget has: obj setters, events, 16 style slots (`detail::base`) | 7.6 | 14 % | `measured` |
| `member_table<"obj">`, `props<"obj">`, `kept<"obj">` | 4.3 | 8 % | `measured` |
| The label kind: its setters, its props and kept structs | 6.8 | 13 % | `measured` |
| The node and the mount | 1.0 | 2 % | `measured` |
| Back end | 2.7 | 5 % | `measured` |

What GCC reports for the same file, as shares of its total time:

| `-ftime-report` row | Include only | One label | `signal.hpp` | Mark |
|---|---|---|---|---|
| constant expression evaluation | 3 % | 55 % | 55 % | `measured` |
| template instantiation | 25 % | 12 % | 22 % | `measured` |
| overload resolution (nested) | 21 % | 66 % | 48 % | `measured` |
| preprocessing | 24 % | 8 % | 6 % | `measured` |
| phase opt and generate (back end) | 4 % | 2 % | 1 % | `measured` |

- Overload resolution overlaps evaluation: GCC times it on top of the phase it runs in.
- The profile of the base scan: 86 % of samples inside `cxx_eval_constant_expression`. Reflection's own functions (`process_metafunction`) are 15 %, `identifier_of` 3.4 %.
- So most of the time is GCC's interpreter running our consteval code, not the reflection queries themselves. `measured`

## Marginal cost

`perf/lazy-api`; front end and full in G. `main` is given where it differs by more than 1 G.

| What is added | Front end | Full | `main` full | Mark |
|---|---|---|---|---|
| An empty file | 0.22 | 0.28 | | `measured` |
| `<meta>` alone | 3.07 | 3.41 | | `measured` |
| `<lvgl/lvgl.h>` alone | 1.29 | 1.36 | | `measured` |
| The 32 standard headers our headers include | 8.10 | 9.09 | | `measured` |
| The same without `<meta>` | 8.04 | 9.03 | | `measured` |
| Those and `<lvgl/lvgl.h>` | 8.90 | 9.92 | | `measured` |
| `<lv/lv.hpp>`, nothing used | 10.74 | 13.01 | 23.04 | `measured` |
| One constexpr label, not mounted | 45.37 | 47.93 | | `measured` |
| An empty `lv::column`, mounted | 44.33 | 47.00 | | `measured` |
| 1, 10, 100 labels, mounted | 51.1, 52.7, 69.5 | 53.9, 55.8, 74.3 | | `measured` |
| 10 labels, each of another kind | 102.6 | 105.8 | | `measured` |
| 10 labels with 3 more properties (`align`, `size`, `x`) | 55.9 | 59.1 | | `measured` |
| 10 labels and a token file (`lv::tokens<json>`) | 59.5 | 62.5 | | `measured` |
| The same, each label with 1 style member | 60.2 | 63.4 | | `measured` |
| The same, each label with 3 style members | 60.1 | 63.3 | | `measured` |
| 10 labels bound to a state (`lv::bind`) | 52.7 | 56.0 | | `measured` |
| 10 labels with an `.id` | 52.7 | 55.8 | | `measured` |
| The same and 10 calls `get<"id">()` | 53.1 | 56.3 | | `measured` |
| `<lv/signal.hpp>` on top of `<lv/lv.hpp>` | 79.6 | 82.3 | | `measured` |

What follows from the table.

- The libraries are cheap: libstdc++ and LVGL cost under 10 G together. `<meta>` adds nothing once the other standard headers are there.
- The first node costs 35 G, whether it is mounted or not. The tree's size does not matter much: 0.19 G a node in the front end.
- A widget kind costs 5.6 G the first time it appears in a file, and nothing after.
- A property, a style member, a binding or an id costs almost nothing after its first use.
- A token file costs 6.8 G in each file that names it.
- `signal.hpp` costs 69 G: its two token files 5.4 G, the first theme 53.9 G, the second 8.8 G.

## Real files

| File | Front end | Full | `main` full | Mark |
|---|---|---|---|---|
| `examples/quick_start/main.cpp` | 126.3 | 134.4 | 134.3 | `measured` |
| `tests/large_tree.cpp` | 72.3 | 81.4 | 82.2 | `measured` |
| The same with its own `-fconstexpr-ops-limit` | | 92.1 | | `measured` |
| `tests/theme.cpp` | 100.1 | 103.5 | 103.6 | `measured` |
| `tests/tokens.cpp` | 116.9 | 124.7 | 124.3 | `measured` |
| `tests/look.cpp` | 273.4 | 280.1 | 281.4 | `measured` |
| `examples/ports/slint/printer/print_page.cpp` | 334.9 | 340.8 | 341.1 | `measured` |
| `examples/ports/qt/treadmill/logic.cpp` | 118.5 | 128.8 | 129.2 | `measured` |
| `examples/ports/qt/treadmill/shell.cpp` | 106.9 | 116.8 | 117.3 | `measured` |

The printer's Print page, staged the same way: its headers and token file 70.6 G, five fonts 234 G, its tree 31 G. `measured`

- The first font drawn costs 52.8 G, each further one about 45 G.
- The fonts are 70 % of the file. Six other files of the printer draw the same fonts again, 21 draws in all.

### By kind of file

The `host-v96` compile database has these translation units. The cost per file is the mean of a sample.

| Kind | Files | Sample | Mean full (G) | Per build (T) | Mark |
|---|---|---|---|---|---|
| Compile-fail tests (`-fsyntax-only`) | 277 | 13 | 24 (median 12.6) | about 6.6 | `measured`, the total extrapolated |
| Tests | 85 | 9 | 104 | about 8.9 | `measured`, the total extrapolated |
| Examples outside the ports | 90 | 9 | 118 | about 10.6 | `measured`, the total extrapolated |
| Ports | 17 | 3 | 117 to 341 | 2 to 6 | `measured`, the total extrapolated |
| C files of LVGL | 563 | 0 | | | not measured |

- A build of one preset is therefore about 30 T instructions of C++, before ccache.
- The compile-fail tests run `${CMAKE_CXX_COMPILER}` from `add_test` (`tests/CMakeLists.txt`), so ccache never sees them, and it could not keep a failed compile anyway (see below). They cost about 6.6 T on every run of each LVGL version. `source`
- The analyzer and the CI minutes are in [compile-time.md](compile-time.md). Its 16 port files took 657 s of CPU, 2224 s under the analyzer. They were not measured again.

### The compile-fail tests in CI

The six `compile-errors` jobs were timed over the last 10 green runs of main. `measured` 2026-11-19

| What | Value |
|---|---|
| One job | 2.4 to 5.7 minutes, median 3.0 to 4.7 |
| The six jobs together, median | 23.5 runner-minutes |
| Their share of a pull request run (134.5 runner-minutes) | 19.5 % |
| Start of the container in a job | about 23 s, a tenth of the job |

The tests already compile with `-fsyntax-only`.

A draft pull request ran the `host-v96` tests four ways, twice each. The second run had the cache of the first. `measured` 2026-11-19

| Variant | Test steps, first run | Test steps, second run | Slowest job, both runs |
|---|---|---|---|
| 2 shards | 505 s | 484 s | 330 s |
| 3 shards, as today | 495 s | 400 s | 239 s |
| 6 shards | 380 s | 532 s | 126 s |
| 3 shards, ccache in front of the compiler | 555 s | 348 s | 199 s |

- ccache does not help. It called 101 of 105 compiles of a shard uncacheable, and its manual says a failed compile stores no result. `measured`, `source`
- The number of shards does not change the sum. The same variant differs by a third between two runs, which is the spread of the runners.
- More shards shorten the wait for the slowest job, and each one pays the start of a container again.
- So nothing in the workflow is changed. What a test costs falls with the fixes below, by 12 % on average.

## Causes, ranked

Each gain was measured on the probe or file named, against `perf/lazy-api`. "Ours" means lvgl26 can change it.

| # | Cause | Evidence | Fix | Gain | Whose |
|---|---|---|---|---|---|
| 1 | Every file runs the same scans again: the API, the styles, the base members, each widget kind | The attribution table: 35 G before the first node exists | A precompiled header that includes `<lv/lv.hpp>` and instantiates one box and one label | One label 53.8 to 5.7 G (−90 %); 100 labels 74.2 to 26.8 (−64 %); 10 kinds 105.4 to 59.6 (−43 %); `large_tree` 81.5 to 33.7 (−59 %); quick start 134.4 to 91.2 (−32 %) | Ours to use; GCC keeps no result between files |
| 2 | A port draws the same fonts in every file | The Print page: 234 G of 335 G are five fonts | Draw them once: a port's precompiled header with the fonts | Print page 340.8 to 50.7 G (−85 %) | Ours |
| 3 | The theme scans the global namespace twice | `theme_members` and `theme_classes` both call `theme_items<D>()`: 8 G for the second | One table, `theme_table<D>`, read by both | `signal.hpp` 81.6 to 71.0 G (−13 %) with fixes 4 to 7 in | Ours |
| 4 | An insertion sort by name, on 139 to 164 items | Without the sort: −5.3 G | A merge sort of indices, stable as the old one | One label −4.0 G (−7 %); 10 kinds −4.1 G | Ours |
| 5 | The API scan asks `identifier_of` of 3474 GCC builtins | `identifier_of` costs 0.35 M to 1.4 M instructions a call, by the length of the name ("Inside GCC") | Skip declarations at source line 0 or 1 first: `source_location_of(m).line() > 1` | −3.4 G a file (−6 %); the same 2026 functions | Ours to avoid; the price is GCC's |
| 6 | `str(it.name)` builds a `std::string` per member for `data_member_spec` | Ablation: −1.0 G for obj | Pass the `std::string_view` itself | One label −2.0 G (−4 %); about −1.1 G per further kind | Ours |
| 7 | `starts_with` on every name of the API, three passes or more per widget kind | One pass over 2026 names: 1.0 G, of which `starts_with` is 0.75 G | Compare the size and one character before `starts_with` | One label −1.05 G (−2 %); −0.4 G per further kind | Ours to avoid; the price is GCC's and libstdc++'s |
| 8 | A theme is a struct of 54 widgets × 16 slots × 139 style members | Value-initialising an empty one: 8 G. Four slots instead of 16: the first theme −6.9 G, the second −6.5 G | A theme that stores only what it sets | About −6.5 G per theme | Ours; an API choice |
| 9 | The standard headers parsed in every file | 9 G of every file | A precompiled header of the standard headers and LVGL alone | −2.8 G a file | Ours to use; modest |

Fixes 3 to 7 together, measured as one tree; each file built and its reference data or test result unchanged:

| File | Before (G) | After (G) | Change | Mark |
|---|---|---|---|---|
| `<lv/lv.hpp>` only | 13.0 | 13.1 | 0 | `measured` |
| One mounted label (three runs, minimum) | 53.9 | 43.7 | −19 % | `measured` |
| 100 labels | 74.6 | 64.5 | −14 % | `measured` |
| 10 widget kinds (three runs) | 105.5 | 83.2 | −21 % | `measured` |
| `tests/large_tree.cpp` (three runs) | 81.2 | 69.8 | −14 % | `measured` |
| The same with its own operation limit of 6.5 million | 92.1 | 70.1 | −24 % | `measured` |
| `examples/quick_start/main.cpp` (three runs) | 133.9 | 108.7 | −19 % | `measured` |
| `<lv/signal.hpp>` (three runs) | 81.4 | 64.2 | −21 % | `measured` |
| `tests/theme.cpp` | 103.7 | 80.0 | −23 % | `measured` |
| `tests/look.cpp` | 279.3 | 264.3 | −5 % | `measured` |
| `print_page.cpp` | 339.5 | 326.0 | −4 % | `measured` |
| 13 compile-fail tests, mean | 24.3 | 21.3 | −12 % | `measured` |

- `include_only` still compiles under its limit of 50,000 operations, and `large_tree` under 6.5 million, with the fixes. `measured`
- The fixes leave a file that names no widget as it is, and the lazy API table keeps it so.

### One pull request per fix

Each fix alone, on `main` at `8c1aaeb` with the lazy API table, against the same tree without it. Full compile, minimum of two runs, G.

| Fix | One label | 10 widget kinds | `large_tree` | `signal.hpp` | Mark |
|---|---|---|---|---|---|
| Without fixes | 54.0 | 106.0 | 81.6 | 81.8 | `measured` |
| 3, the theme's scan once | 0.0 | −0.3 | 0.0 | −11.4 | `measured` |
| 4, the merge sort | −4.0 | −4.2 | −4.3 | −4.5 | `measured` |
| 5, no builtins in the API scan | −3.2 | −3.6 | −3.4 | −3.4 | `measured` |
| 6, no `std::string` per member | −2.1 | −11.2 | −3.1 | −0.1 | `measured` |
| 7, the cheap prefix check | −1.2 | −4.9 | −1.9 | −0.5 | `measured` |
| The sum of the five | −10.5 | −24.3 | −12.8 | −19.9 | `measured` |
| All five in one tree | −10.1 | −22.5 | −11.8 | −17.4 | `measured` |

- The gains nearly add up, so each fix can be its own pull request in any order.
- Fixes 3 and 4 overlap on a theme: the theme's scan sorts its items, so scanning once also halves what the sort saves there.
- Fix 6 grows with the number of widget kinds in a file; fix 3 helps only a file with a theme.

What each kind of file can expect from the five together, from the files measured above:

| Kind of file | Measured on | Gain | Mark |
|---|---|---|---|
| A test or example with a few widget kinds | One label, `large_tree` | 14 to 19 % | `measured` |
| A file with many widget kinds | 10 kinds | 21 % | `measured` |
| A file with a theme | `signal.hpp`, `tests/theme.cpp` | 21 to 23 % | `measured` |
| A compile-fail test | 13 of 277 | 12 % on average; none for a file that names no widget | `measured` |
| A file that draws fonts | `print_page.cpp`, `tests/look.cpp` | 4 to 5 %: the fonts are the cost, see fix 2 | `measured` |
| A file that names no widget | `<lv/lv.hpp>` only | none | `measured` |

### Fixes 3, 4, 5 and 7 as built

Built in the branch `perf/first-widget`, one commit per fix. Measured on 2026-11-20 against `main` at `9dccce2`: full compile with the flags of the build, operation limits kept, minimum of three runs, G.

| File | Before | After | Change | Mark |
|---|---|---|---|---|
| `<lv/lv.hpp>` only | 12.84 | 12.83 | −0.1 % | `measured` |
| One mounted label | 53.85 | 45.66 | −15.2 % | `measured` |
| 10 widget kinds | 105.56 | 93.93 | −11.0 % | `measured` |
| `tests/large_tree.cpp`, under its limit of 6.5 million | 92.02 | 75.04 | −18.5 % | `measured` |
| `examples/quick_start/main.cpp` | 134.07 | 112.70 | −15.9 % | `measured` |
| `<lv/signal.hpp>` | 81.34 | 64.23 | −21.0 % | `measured` |
| `tests/theme.cpp` | 103.35 | 81.99 | −20.7 % | `measured` |

- Fix 6 was withdrawn. A name passed to `data_member_spec` as a view of a static string makes a `std::string` that compares the pointer with null. Under a sanitizer GCC 16.2 does not fold that: `host-v96-asan` stopped compiling `theme.cpp`, `look.cpp` and `signal.cpp`. `member_pairs` and the theme table copy the name with `detail::str` again. `measured`
- The same holds for `std::string(view)`: it fails under a sanitizer on a string from `std::define_static_string`, and compiles without one. `std::string(view.begin(), view.end())` compiles under both, at a quarter of the cost of `detail::str`'s `push_back` loop. `measured` on a probe of 1000 names
- The builtins are filtered by `detail::declared_in_a_file`. `tests/api_table.cpp` compares the table with a scan that asks every name, so a platform that puts builtins elsewhere loses the gain, not a function.
- The merge sort is two functions, each of complexity 8 or less, as `tools/crap.py` asks.
- All 277 compile-fail tests give the same error text as before on `host-v96`; only line numbers in our headers moved.

### The second round: strings in constant evaluation

Built in the branch `perf/consteval-strings`, from the costs of "Inside GCC". Measured on 2026-11-20 against `main` at `815fcf4`: full compile, operation limits kept, minimum of three runs, G.

| File | Before | After | Change | Mark |
|---|---|---|---|---|
| `<lv/lv.hpp>` only | 13.06 | 13.07 | +0.1 % | `measured` |
| One mounted label | 45.85 | 42.22 | −7.9 % | `measured` |
| 10 widget kinds | 94.28 | 82.90 | −12.1 % | `measured` |
| `tests/large_tree.cpp`, under its limit | 75.37 | 70.98 | −5.8 % | `measured` |
| `examples/quick_start/main.cpp` | 112.88 | 106.77 | −5.4 % | `measured` |
| `<lv/signal.hpp>` | 64.42 | 62.60 | −2.8 % | `measured` |
| `tests/theme.cpp` | 81.99 | 77.54 | −5.4 % | `measured` |

- `detail::str` copies through the iterator constructor of `std::string`, which compiles under a sanitizer, at a quarter of the cost of a `push_back` loop. It gives back most of withdrawn fix 6. `measured`
- `values.hpp` and `props.hpp` compare names with `"..."sv` literals, so no view counts its characters. `measured`
- The same literals in the other eight headers, caching `api()`, and the PNG reader's `vector(n)` each saved under 0.8 G on these files and were left out. `measured`

### The precompiled header, measured

| Header | `.gch` size | Instructions to build | Saves per file | Mark |
|---|---|---|---|---|
| The standard headers and LVGL | 147 MB | at least 8.4 G | 2.8 G | `measured` |
| `<lv/lv.hpp>` | 196 MB | at least 11.7 G | 9.4 G | `measured` |
| `<lv/lv.hpp>`, a box and a label instantiated | 257 MB | at least 51.4 G | 48 G for a one-label file | `measured` |
| The printer's headers and five fonts | 314 MB | at least 303 G | 290 G for the Print page | `measured` |

- The build counts stop where GCC refused to write the file to `/dev/null`, so writing it is not counted.
- Each header pays for itself on the second file of its target.
- One `.gch` serves one set of flags: each preset and each target with other options needs its own.
- With the warm header, `tests/large_tree.cpp` built and printed `ok`.
- The warm header refines lever 6 of [compile-time.md](compile-time.md): a precompiled header saves our scans, not only parsing, when it instantiates them.
- ccache 4.14's manual: GCC needs `sloppiness=pch_defines,time_macros` and `-include` or `-fpch-preprocess`. It warns that `#pragma once` "may cause issues", and we use it everywhere. `source` (`man ccache`, "PRECOMPILED HEADERS")

## The experiments as diffs

Each was made in a copy of `include/` from `perf/lazy-api`.

Fix 3, `include/lv/theme.hpp`:

```diff
+struct theme_row {
+  const char* name;
+  std::meta::info widget;
+};
+
+template <class D>
+consteval std::vector<theme_row> theme_rows() {
+  std::vector<theme_row> out;
+  for (const auto& item : theme_items<D>()) out.push_back({std::define_static_string(item.name), item.widget});
+  return out;
+}
+
+template <class D = void>
+inline constexpr auto theme_table = std::define_static_array(theme_rows<D>());
+
 template <class D>
 consteval std::vector<std::meta::info> theme_specs() {
   std::vector<std::meta::info> out;
-  for (const auto& item : theme_items<D>())
-    out.push_back(std::meta::data_member_spec(^^typename look_of<D>::type, {.name = item.name}));
+  for (const auto& item : theme_table<D>)
+    out.push_back(std::meta::data_member_spec(^^typename look_of<D>::type, {.name = std::string_view(item.name)}));
   return out;
 }
@@ theme_widgets
-  for (const auto& item : theme_items<D>()) out.push_back(item.widget);
+  for (const auto& item : theme_table<D>) out.push_back(item.widget);
```

Fix 4, `include/lv/values.hpp`:

```diff
 template <class Item>
 consteval void sort_by_name(std::vector<Item>& items) {
-  for (std::size_t i = 1; i < items.size(); ++i)
-    for (std::size_t j = i; j > 0 && items[j].name < items[j - 1].name; --j)
-      std::swap(items[j], items[j - 1]);
+  const std::size_t n = items.size();
+  std::vector<std::size_t> a(n), b(n);
+  for (std::size_t i = 0; i < n; ++i) a[i] = i;
+  for (std::size_t w = 1; w < n; w *= 2) {
+    for (std::size_t lo = 0; lo < n; lo += 2 * w) {
+      const std::size_t mid = lo + w < n ? lo + w : n, hi = lo + 2 * w < n ? lo + 2 * w : n;
+      std::size_t i = lo, j = mid, k = lo;
+      while (i < mid && j < hi) b[k++] = items[a[j]].name < items[a[i]].name ? a[j++] : a[i++];
+      while (i < mid) b[k++] = a[i++];
+      while (j < hi) b[k++] = a[j++];
+    }
+    a.swap(b);
+  }
+  std::vector<Item> out;
+  out.reserve(n);
+  for (std::size_t i = 0; i < n; ++i) out.push_back(std::move(items[a[i]]));
+  items = std::move(out);
 }
```

`std::ranges::stable_sort` does not compile here: libstdc++'s temporary buffer is refused for a type that holds a `std::meta::info`.
Moving instead of swapping in the insertion sort saved only 1.0 G: the comparisons are the cost, not the copies.

Fix 5, `include/lv/detail/api.hpp`:

```diff
-    if (std::meta::is_function(m) && std::meta::has_identifier(m) &&
+    if (std::meta::is_function(m) && std::meta::source_location_of(m).line() > 1 && std::meta::has_identifier(m) &&
```

Fix 6, `include/lv/props.hpp`, in `member_pairs`:

```diff
-    out.push_back(std::meta::data_member_spec(it.type, {.name = str(it.name)}));
+    out.push_back(std::meta::data_member_spec(it.type, {.name = it.name}));
```

Fix 7: a helper in `include/lv/detail/api.hpp`, used by the five loops over `api(...)` that test a prefix (`setters_with_prefix`, `unmatched_binder`, `only_integer_setter`, `scan_style`, `names_after`):

```cpp
consteval bool has_prefix(std::string_view id, std::string_view prefix) {
  if (id.size() < prefix.size()) return false;
  if (prefix.size() > 3 && id[3] != prefix[3]) return false;
  return id.starts_with(prefix);
}
```

The warm precompiled header of fix 1:

```cpp
#pragma once
#include <lv/lv.hpp>
namespace lv_warm {
constexpr auto warm_box = lv::box();
constexpr auto warm_label = lv::label({.text = "w"});
inline void warm(lv::obj s) {
  auto a = lv::mount<warm_box>(s);
  auto b = lv::mount<warm_label>(s);
  (void)a;
  (void)b;
}
}  // namespace lv_warm
```

It was built with the flags of the file and `-x c++-header`, and given to the file with `-include`.

## What is GCC's

| Behaviour | Measurement | Mark |
|---|---|---|
| `std::meta::identifier_of` costs 0.35 M instructions for a name of 8 characters and 1.37 M for 40, on every call: see "Inside GCC" | 4000 generated functions, one and two passes. Corrected 2026-11-19: a second pass costs as much as the first; a second call in the same expression was cheap | `measured` |
| `members_of(^^::)` lists 3474 builtins with the 2026 `lv_` functions | Counted by reflection; the builtins are at source line 0 or 1 | `measured` |
| `source_location_of(m).file_name()` is as dear as `identifier_of`; `line()` is free | 5.4 G against 0.1 G for one pass | `measured` |
| `std::string_view::starts_with` costs about 0.37 M instructions per call in constant evaluation | One pass over 2026 names: 1.0 G with it, 0.25 G without | `measured` |
| GCC keeps no result of constant evaluation from one file for the next | Every file pays the 35 G again; only a precompiled header carries them | `measured`; [cpp26.md](cpp26.md) found no source for caching across files |
| Value-initialising a constant aggregate of 120,000 members costs 8 G | An empty `theme_spec` as a `constexpr` variable | `measured` |
| A low `-fconstexpr-ops-limit` made `large_tree` dearer, 81.5 G to 92.1 G; with fixes 3 to 7, 69.8 to 70.1 G | Same file, same tree, the option alone | `measured`; the cause is not known |
| The back end is 2 to 11 G a file, 3 to 8 % | Full minus front end, every file above | `measured` |

## Inside GCC

Why GCC 16.2 charges what it charges: the tools that see below our functions, and what they show.
Every row was measured on 2026-11-19 with the setup above. The source read is the tag `releases/gcc-16.2.0` ([constexpr.cc](https://github.com/gcc-mirror/gcc/blob/releases/gcc-16.2.0/gcc/cp/constexpr.cc), [reflect.cc](https://github.com/gcc-mirror/gcc/blob/releases/gcc-16.2.0/gcc/cp/reflect.cc)) with Homebrew's libstdc++ headers.

### The tools, and what each answers

Each was run on the one-label probe (`perf/lazy-api`, 52 G front end).

| Tool | Question it answers | Output on one label | Mark |
|---|---|---|---|
| `sample` on `cc1plus` (Instruments works the same) | Which GCC function the time is in | 86 % of samples under `cxx_eval_constant_expression`; `process_metafunction` 15 %; `eval_identifier_of` 3.4 % | `measured` |
| Homebrew's `cc1plus` | Can a profile name GCC's functions? | Yes: 45,710 text symbols. No DWARF, so no source lines and no frames for inlined functions | `measured` |
| `-ftime-report-details` | Which phase, and which phase inside which | Constant evaluation 62 %, overload resolution 72 % (nested), instantiation 12 %; GC memory 989 MB | `measured` |
| `-fstats` | How many specializations and types were made | 54,934 declaration and 14,709 type specializations; 43,010 types | `measured` |
| `-fmem-report` | Where GC memory goes, by object size | 535 MB allocated, 233 MB in use at the end | `measured` |
| `-fdump-lang-tinst-lineno` | Which instantiations, from where, in order | 59,642 lines; 6.6 MB. No times or counts | `measured` |
| `-fdump-lang-raw`, `-fdump-tree-original`, `-fdump-lang-class` | The trees GCC holds | 102 MB, 3.6 MB, 3.4 MB; useful for one function, not for cost | `measured` |
| `-fconstexpr-cache-depth=N` | What the call cache saves | 0: 63.5 G; 2: 63.3; 8 (default): 51.2; 16: 49.9; 64: 49.8 | `measured` |
| `-fconstexpr-loop-limit`, `-fconstexpr-ops-limit` | Only where a limit stops a file | No change at 262,144 rounds or 4 billion operations | `measured` |
| `-ftime-trace` | Not in GCC 16.2: `g++-16 --help=c++` and `--help=common` list no such option | | `source` |

- No GCC 16.2 option counts operations per function. The operation count exists only per evaluation, and is printed only on overflow (`constexpr.cc`, `cxx_eval_constant_expression`).
- A profile shows GCC's functions but never which of our functions is being interpreted. Our attribution came from probes that add one line at a time.
- A GCC built from source with `-O2 -g` would add source lines and inlined frames to the same profile. A build with a counter in `cxx_eval_call_expression`, keyed by the function evaluated, would give operations per consteval function. That is the tool we lack. Building GCC takes about an hour of 12 cores and several GB (`recalled`); it was not done.

### Where the cost comes from

| Cost | Measurement | Cause | Mark |
|---|---|---|---|
| One operation of the interpreter | A bare loop: 7,300 instructions a round, about 19 operations | `cxx_eval_constant_expression` walks the tree of each expression | `measured` |
| One call of a constexpr function | About 6,000 instructions more than the loop; a hit in the call cache saves about 2,500 | Every call with constant arguments is hashed (`iterative_hash_template_arg` over the bindings), looked up, and on a miss inserted with its arguments copied (`unshare_expr_without_location`) | `measured`, `source` |
| `string_view(p)` on a 24-character string | 620,000 instructions; `string_view(p, 24)` 17,500 | In constant evaluation `char_traits<char>::length` is a loop of calls, about 25,000 instructions a character. `__builtin_strlen` is folded for free | `measured`, `source` (`bits/char_traits.h`) |
| `identifier_of` | 0.35 M for 8 characters, 1.37 M for 40; the same on every pass | `eval_identifier_of` builds a string literal and calls the `string_view(const char*)` constructor (`build_special_member_call`), which the interpreter then runs: the loop above. The literal's length is known and not passed | `measured`, `source` (`reflect.cc`) |
| `display_string_of` | 0.67 M a call | The same, with a longer string | `measured` |
| `string_view::starts_with` | 285,000 instructions; `__builtin_memcmp` free; a pointer loop 50,000 | `starts_with` is `substr` and `operator==`, and `char_traits::compare` is a loop of two calls a character in constant evaluation | `measured`, `source` |
| Why libstdc++ loops | `__builtin_memcmp` and `__builtin_strlen` fail on the constexpr heap ("is not a constant expression") | `cxx_eval_builtin_function_call` folds a string builtin only on a string literal or a constant array: "expand_builtin doesn't know how to look in the values table" | `measured`, `source` |
| `std::vector<unsigned char>(n)` | 79,000 instructions an element; `new unsigned char[n]()` 17,000; `reserve` nothing per element | libstdc++ value-initialises each element through calls | `measured` |
| A loop that fills a vector by `v[i]` | 139,000 instructions a round; through a pointer 112,000 | One more call a round | `measured` |
| `std::define_static_array` | About 0.17 G for the call, and about 1,700 instructions a byte on top | Small next to making the data | `measured` |
| `substitute` and `extract` | 0.84 M for a new specialization, 0.76 M for the same one again | A call that reaches a metafunction is never cached: `metafns_called` makes it uncacheable, "until we figure out which exact metafunctions need this" | `measured`, `source` |
| Copying a struct of 1,000 `int` members | 750 instructions a member, a copy by value; by reference nothing | A copy is a `CONSTRUCTOR` of all elements; a value-initialised one is empty and cheap | `measured` |
| `large_tree` under `-fconstexpr-ops-limit=6500000` | 92 G against 81 G; the extra samples are in `maybe_constant_init_1`, 557 to 1480 | GCC tries to constant-initialise variables it need not, and an attempt cut off at the limit is lost work. Why it repeats was not found | `measured`; the cause in part |

### What to do with each

| Finding | Kind | What follows |
|---|---|---|
| `string_view(const char*)`, `starts_with`, `compare`, `==` on long strings | Ours to avoid | Keep lengths with names; test the size and one character before a library comparison (fix 7 of this note) |
| `std::vector(n)` for large data | Ours to avoid | `reserve` and write through a pointer, or `new[]`, as the font and picture readers do |
| A big struct passed or returned by value | Ours to avoid | Pass by reference; keep `kept<W>` small, as `CLAUDE.md` says |
| A call that names a metafunction is not cached | Ours to avoid | Store a scan's result in a variable template (`define_static_array`) and read the table, as `api_table` does |
| `-fconstexpr-cache-depth=16` | A cheaper GCC setting | −2.6 % on one label, more memory; a project's choice, not a library rule |
| `__builtin_strlen` and `__builtin_memcmp` | A cheaper facility, not for us | Free on constant data, but our rules allow no builtins, and they fail on the heap |
| `identifier_of` runs a strlen loop for a length GCC knows | GCC inefficiency | Candidate report A |
| String builtins cannot read the constexpr heap, so libstdc++ loops | GCC inefficiency | Candidate report B |
| Metafunction calls are never cached | GCC inefficiency, known to its authors | Candidate C; the comment in `constexpr.cc` says it is deliberate for now |
| `std::vector(n)` value-initialises element by element | libstdc++ inefficiency | Candidate D |
| A low operation limit costs more | Not understood | Reduce it first; no report |

### Candidate reports

Nothing was filed; the owner decides.

Each was tried in GCC built from source: see "GCC built from source" below.

A. `identifier_of` constructs its `string_view` from a pointer alone.

```cpp
#include <meta>
#include <string_view>
namespace ns { void f0_long_name_of_forty_characters_xxxxx(); /* ... 4000 such */ }
consteval std::size_t c() {
  std::size_t s = 0;
  for (auto m : std::meta::members_of(^^ns, std::meta::access_context::unchecked())) s += std::meta::identifier_of(m).size();
  return s;
}
constexpr auto n = c();
```

4000 names of 40 characters: 5.5 G instructions more than asking `has_identifier`, 1.37 M a call. With 8 characters: 0.35 M. `reflect.cc` knows the length in `get_string_literal`. `measured`, `source`

B. `__builtin_memcmp` and `__builtin_strlen` are not constant on constexpr-allocated storage.

```cpp
#include <string>
consteval int a() { std::string s = "hello world, on the constexpr heap"; return __builtin_memcmp(s.data(), "hello", 5); }
constexpr int x = a();  // error: '__builtin_memcmp(...)' is not a constant expression
```

On a constant array the same call costs nothing; libstdc++'s loop costs 285,000 instructions for one `starts_with`. Related reports exist for local arrays: [PR 70816](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=70816) and [PR 80265](https://gcc.gnu.org/pipermail/gcc-bugs/2017-April/580741.html) (`summary`). None was found for the heap. `measured`

C. Calls that reach a metafunction are never cached: the same `substitute` and `extract` again costs 0.76 M against 0.84 M the first time. `measured`, `source`

D. `std::vector<unsigned char>(n)` costs 79,000 instructions an element in constant evaluation, 4.6 times `new unsigned char[n]()`. `measured`

A search of GCC's Bugzilla found no report on the cost of reflection or of `identifier_of` (`summary`).

## GCC built from source: the four candidates tried

Candidates A to D, tried in GCC itself: three compilers built from source, each candidate patched and measured alone, then all together.
Measured on 2026-11-20 unless a row says otherwise.
Nothing was filed; the drafts of the reports are kept outside the repository until the owner has read them.

### Short answers

| Question | Answer | Mark |
|---|---|---|
| Did GCC's development branch already fix A to D? | No. Trunk of 2026-11-19 costs the same for A, B and D. C is 10 % cheaper there, and still never cached | `measured` |
| What did trunk improve for us? | `members_of(^^::)` no longer lists GCC's built-in functions: 674 members instead of 4142. One label costs 7 % less on trunk, and 9 % less on 16.2 with that one commit | `measured`, `source` |
| What does the best single fix give? | B's library half: 21 % less for one label. A gives 11 % | `measured` |
| What do all of them give? | 54.1 G down to 40.8 G for one label, a quarter less, with the same output | `measured` |
| Which is ready to send to GCC? | A: 36 lines added, 28 removed, and the reflection and constant-expression tests pass. A request to backport the `members_of` fix to GCC 16. B, C and D are experiments that show the gain | `measured` |

### The three compilers

| Compiler | Source | Mark |
|---|---|---|
| 16.2 | The tag `releases/gcc-16.2.0`, commit `78d4ac73dd` of 2026-08-07 | `source` |
| 16.2 patched | The same tree with the patches below on local branches. Each was built alone, and then all together | `source` |
| trunk | `master` at `9eb44775b9` of 2026-11-19, version 17.0.0 | `source` |

- Each was configured with `--enable-languages=c,c++ --disable-bootstrap --disable-multilib --enable-checking=release`, and built by the system GCC 13.3 with its default `-g -O2`.
- The machine is an aarch64 Linux machine with 14 cores, Ubuntu 24.04.
- A patched compiler is a copy of the 16.2 installation with its `cc1plus` replaced. A library experiment is a copy with one header changed.
- Building one compiler took 9 minutes with 8 jobs, and trunk 13 minutes with 4. Rebuilding `cc1plus` after a patch took a few minutes at most.

### How it was measured

- The number is the user-space instructions retired by `cc1plus` alone. A small counter read them through `perf_event_open`; `perf` itself is not installed there.
- `g++ -###` printed the `cc1plus` command, and the counter ran it. Each number is the least of three runs.
- Runs of the same file differed by under 0.3 %. A probe of 45 G therefore has a noise of about 0.1 G, or 15 k a call over 4000 calls.
- The probes are front end only: `-std=c++26 -freflection -fsyntax-only`.
- A cost per call is the difference between two probes divided by the number of calls. The cheaper twin of each probe does the same work without the call.
- The one-label file is the probe of "Attribution", on `main` at `9dccce2` with LVGL v9.6.0. It used the defines, include paths and `-std` of `host-v96`, with `-Wall -Wextra -Wpedantic -Werror` but not the other warning flags.
- Linux and macOS give different totals for the same file: 54.1 G here, 51.1 G on macOS for `perf/lazy-api`. Compare within one table only.

The probes, written fresh on that machine:

| Probe | What it does | Its twin |
|---|---|---|
| A | `identifier_of` on 4000 functions with names of 8 or 40 characters, summing the sizes | `has_identifier` on the same functions |
| B, literal | `string_view::starts_with("lv_0")` on 4000 string literals of 34 characters | `size() > 4` on the same views |
| B, heap | The same, each name first copied into a `std::string` | The same copy |
| B, built-in | `__builtin_memcmp(s.data(), "hello", 5)` on a `std::string`: the reproducer of candidate B | None: it either compiles or not |
| C, same | 1000 calls `extract<int>(substitute(^^v, {reflect_constant(0)}))` with the same argument | No call |
| C, new | The same with 1000 different arguments | No call |
| D | `std::vector<unsigned char> v(20001)` | The same with 1 element |
| `members_of` | `members_of(^^::)` with `<meta>` included | No call |

### A: `identifier_of`

The patch builds the `string_view` from the pointer and the length that `reflect.cc` already has, at the four places that build one.

| Compiler | 8 characters, a call | 40 characters, a call | One label, front end | Full | Mark |
|---|---|---|---|---|---|
| 16.2 | 381 k | 1,499 k | 54.09 G | 55.27 G | `measured` |
| trunk | 380 k | 1,497 k | 50.12 G | 51.21 G | `measured` |
| 16.2 with A | 45 k | 45 k | 48.23 G, −10.8 % | 49.37 G | `measured` |
| trunk with A | 44 k | 46 k | 47.48 G, −5.3 % against trunk | 48.63 G | `measured` |

- A call now costs the same whatever the length. The 45 k left is the call of the two-argument constructor.
- Trunk has the same `get_string_literal` as 16.2: nothing changed there between the release and 2026-11-19. `source`
- The patch applies to trunk unchanged. Trunk's `g++.dg/reflect` gives 4962 passes and 38 expected failures with and without it. `measured`

<details><summary>Patch A, against <code>releases/gcc-16.2.0</code></summary>

```diff
diff --git a/gcc/cp/reflect.cc b/gcc/cp/reflect.cc
index 58795cfd6b..42249a2ec8 100644
--- a/gcc/cp/reflect.cc
+++ b/gcc/cp/reflect.cc
@@ -2482,7 +2482,8 @@ eval_operator_of (location_t loc, const constexpr_ctx *ctx, tree r,
    literal, or NULL_TREE if the conversion failed.  */
 
 static tree
-get_string_literal (const char *name, tree elt_type)
+get_string_literal (const char *name, tree elt_type,
+		    unsigned HOST_WIDE_INT *len = nullptr)
 {
   cpp_string istr, ostr;
   istr.len = strlen (name) + 1;
@@ -2492,11 +2493,31 @@ get_string_literal (const char *name, tree elt_type)
 			     ? CPP_STRING : CPP_UTF8STRING, false))
     return NULL_TREE;
   name = (const char *) ostr.text;
-  tree ret = build_string_literal (strlen (name) + 1, name, elt_type);
+  size_t n = strlen (name);
+  if (len)
+    *len = n;
+  tree ret = build_string_literal (n + 1, name, elt_type);
   free (const_cast <char *> (name));
   return ret;
 }
 
+/* Build an object of TYPE, a string_view or u8string_view, from STR, the
+   address of a string literal returned by get_string_literal, and LEN, its
+   length.  Passing the length spares the constant evaluator the
+   char_traits::length loop of the constructor from a pointer alone.  */
+
+static tree
+build_string_view (tree str, unsigned HOST_WIDE_INT len, tree type)
+{
+  releasing_vec args;
+  vec_safe_push (args, str);
+  vec_safe_push (args, build_int_cst (size_type_node, len));
+  tree r = build_special_member_call (NULL_TREE, complete_ctor_identifier,
+				      &args, type, LOOKUP_NORMAL,
+				      tf_warning_or_error);
+  return build_cplus_new (type, r, tf_warning_or_error);
+}
+
 /* Process std::meta::{,u8}symbol_of.
    Returns: A string_view or u8string_view containing the characters of the
    operator symbol name corresponding to op, respectively encoded with the
@@ -2530,17 +2551,13 @@ eval_symbol_of (location_t loc, const constexpr_ctx *ctx, tree expr,
 	      strcpy (buf + (sp - name), sp + 1);
 	      name = buf;
 	    }
-	  tree str = get_string_literal (name, elt_type);
+	  unsigned HOST_WIDE_INT len;
+	  tree str = get_string_literal (name, elt_type, &len);
 	  /* Basic character set ought to be better convertible
 	     into ordinary literal character set and must be always
 	     convertible into UTF-8.  */
 	  gcc_checking_assert (str);
-	  releasing_vec args (make_tree_vector_single (str));
-	  tree r = build_special_member_call (NULL_TREE,
-					      complete_ctor_identifier,
-					      &args, ret_type, LOOKUP_NORMAL,
-					      tf_warning_or_error);
-	  return build_cplus_new (ret_type, r, tf_warning_or_error);
+	  return build_string_view (str, len, ret_type);
 	}
   goto fail;
 }
@@ -3688,7 +3705,8 @@ eval_identifier_of (location_t loc, const constexpr_ctx *ctx, tree r,
     name = IDENTIFIER_POINTER (TREE_VEC_ELT (r, 1));
   else
     gcc_unreachable ();
-  tree str = get_string_literal (name, elt_type);
+  unsigned HOST_WIDE_INT len;
+  tree str = get_string_literal (name, elt_type, &len);
   if (str == NULL_TREE)
     {
       if (elt_type == char_type_node)
@@ -3700,11 +3718,7 @@ eval_identifier_of (location_t loc, const constexpr_ctx *ctx, tree r,
 					  " in UTF-8",
 				fun, non_constant_p, jump_target);
     }
-  releasing_vec args (make_tree_vector_single (str));
-  tree ret = build_special_member_call (NULL_TREE, complete_ctor_identifier,
-					&args, ret_type, LOOKUP_NORMAL,
-					tf_warning_or_error);
-  return build_cplus_new (ret_type, ret, tf_warning_or_error);
+  return build_string_view (str, len, ret_type);
 }
 
 /* Display R, which is a data member description.  */
@@ -3798,7 +3812,8 @@ eval_display_string_of (location_t loc, const constexpr_ctx *ctx, tree r,
 #if __GNUC__ >= 10
 #pragma GCC diagnostic pop
 #endif
-  tree str = get_string_literal (pp_formatted_text (&pp), elt_type);
+  unsigned HOST_WIDE_INT len;
+  tree str = get_string_literal (pp_formatted_text (&pp), elt_type, &len);
   if (str == NULL_TREE)
     {
       if (elt_type == char_type_node)
@@ -3811,11 +3826,7 @@ eval_display_string_of (location_t loc, const constexpr_ctx *ctx, tree r,
 				" in UTF-8",
 				fun, non_constant_p, jump_target);
     }
-  releasing_vec args (make_tree_vector_single (str));
-  tree ret = build_special_member_call (NULL_TREE, complete_ctor_identifier,
-					&args, ret_type, LOOKUP_NORMAL,
-					tf_warning_or_error);
-  return build_cplus_new (ret_type, ret, tf_warning_or_error);
+  return build_string_view (str, len, ret_type);
 }
 
 /* Determine the reflection kind for R.  */
@@ -7584,18 +7595,15 @@ eval_exception__S_exception_cvt_tofrom_utf8 (location_t loc,
   /* And then let get_string_literal translate from SOURCE_CHARSET to
      {UTF-8,exec charset}.  */
   tree dchar_type = to_utf8 ? char8_type_node : char_type_node;
-  str = get_string_literal (name, dchar_type);
+  unsigned HOST_WIDE_INT len;
+  str = get_string_literal (name, dchar_type, &len);
   free (const_cast <unsigned char *> (ostr.text));
   if (str == NULL_TREE)
     {
-      str = get_string_literal ("", dchar_type);
+      str = get_string_literal ("", dchar_type, &len);
       gcc_assert (str);
     }
-  releasing_vec args (make_tree_vector_single (str));
-  tree ret = build_special_member_call (NULL_TREE, complete_ctor_identifier,
-					&args, TREE_TYPE (call), LOOKUP_NORMAL,
-					tf_warning_or_error);
-  return build_cplus_new (TREE_TYPE (call), ret, tf_warning_or_error);
+  return build_string_view (str, len, TREE_TYPE (call));
 }
 
 /* Helper for eval_extract, extracting a reference type T.
```

</details>

### B: string built-ins on the constexpr heap

B has two halves, and neither helps alone everywhere.
The front-end half lets `__builtin_memcmp`, `strlen` and friends read characters that the evaluation knows: the constexpr heap, a local array, a constant array.
The library half makes `char_traits<char>::compare` and `length` ask the built-in first in constant evaluation, under `__builtin_constant_p`, and keep the loop otherwise.

| Compiler | `starts_with`, literal | `starts_with`, heap | The built-in on the heap | One label, front end | Full | Mark |
|---|---|---|---|---|---|---|
| 16.2 | 307 k | 308 k | Error: not a constant expression | 54.09 G | 55.27 G | `measured` |
| trunk | 312 k | 294 k | Error | 50.12 G | 51.21 G | `measured` |
| 16.2, front end half | 311 k | 325 k | Compiles | 54.11 G | 55.28 G | `measured` |
| 16.2, library half | 53 k | 242 k | Error | 42.52 G, −21.4 % | 43.69 G | `measured` |
| 16.2, both halves | 53 k | 60 k | Compiles | 41.82 G, −22.7 % | 42.97 G | `measured` |

- The front-end half alone changes nothing for our code: libstdc++ never calls the built-in in constant evaluation. It only makes the reproducer compile.
- The library half alone already folds every literal. For one label that is most of the gain: `string_view(const char*)` on a literal is a `strlen` that folds.
- The heap needs both halves: the built-in must read the heap, and the library must call it.
- The base of the heap probes is 22 to 45 G. Their per-call numbers carry a noise of about 15 k.
- No commit on trunk touched `char_traits.h`. Trunk's `8b93d6bf07` (PR 127218, 2026-09-26) fixed an offset counted twice when `memchr` folds; it does not widen what folds. `source`

This is an experiment, not a patch for review.
A first version of the front-end half gave wrong answers on `#embed` data, and GCC's tests did not notice: see "GCC's tests".
GCC keeps `#embed` and long initializers as one `RAW_DATA_CST` element of many bytes, and the first version zero-filled its span.
The version below reads it, and lvgl26's reference data, which searches LVGL's embedded headers, is identical.

<details><summary>B, front-end half, against <code>releases/gcc-16.2.0</code></summary>

```diff
diff --git a/gcc/cp/constexpr.cc b/gcc/cp/constexpr.cc
index 7a09f2947c..6a4fd68dbe 100644
--- a/gcc/cp/constexpr.cc
+++ b/gcc/cp/constexpr.cc
@@ -2467,6 +2467,145 @@ cxx_eval_constexpr_diag (const constexpr_ctx *ctx, tree t, bool *non_constant_p,
   return void_node;
 }
 
+/* Append to BUF the characters of VAL, the constant value of an array of
+   type ATYPE, from index OFF on: BOUND of them, or up to and including the
+   first NUL if BOUND is negative.  Return false if they are not all known.  */
+
+static bool
+append_known_chars (tree val, unsigned HOST_WIDE_INT off, HOST_WIDE_INT bound,
+		    tree atype, auto_vec<char, 64> &buf)
+{
+  if (bound == 0)
+    return false;
+  auto done = [&] (unsigned HOST_WIDE_INT c)
+    {
+      buf.safe_push ((char) c);
+      return bound < 0 ? c == 0 : buf.length () == (unsigned) bound;
+    };
+  unsigned HOST_WIDE_INT next = off;
+  bool cleared = true;
+  if (TREE_CODE (val) == STRING_CST)
+    {
+      unsigned HOST_WIDE_INT n = TREE_STRING_LENGTH (val);
+      const unsigned char *p
+	= (const unsigned char *) TREE_STRING_POINTER (val);
+      for (; next < n; ++next)
+	if (done (p[next]))
+	  return true;
+    }
+  else if (TREE_CODE (val) == CONSTRUCTOR)
+    {
+      unsigned HOST_WIDE_INT pos = 0, ix;
+      tree idx, v;
+      FOR_EACH_CONSTRUCTOR_ELT (CONSTRUCTOR_ELTS (val), ix, idx, v)
+	{
+	  /* A RAW_DATA_CST (#embed, a long initializer) holds many
+	     characters in one element.  */
+	  bool raw = TREE_CODE (v) == RAW_DATA_CST;
+	  if (!raw && TREE_CODE (v) != INTEGER_CST)
+	    return false;
+	  unsigned HOST_WIDE_INT lo, hi;
+	  unsigned HOST_WIDE_INT n = raw ? RAW_DATA_LENGTH (v) : 1;
+	  if (idx == NULL_TREE)
+	    lo = pos, hi = pos + n - 1;
+	  else if (TREE_CODE (idx) == RANGE_EXPR && !raw
+		   && tree_fits_uhwi_p (TREE_OPERAND (idx, 0))
+		   && tree_fits_uhwi_p (TREE_OPERAND (idx, 1)))
+	    {
+	      lo = tree_to_uhwi (TREE_OPERAND (idx, 0));
+	      hi = tree_to_uhwi (TREE_OPERAND (idx, 1));
+	    }
+	  else if (TREE_CODE (idx) != RANGE_EXPR && tree_fits_uhwi_p (idx))
+	    lo = tree_to_uhwi (idx), hi = lo + n - 1;
+	  else
+	    return false;
+	  pos = hi + 1;
+	  if (hi < next)
+	    continue;
+	  if (lo > next)
+	    {
+	      if (CONSTRUCTOR_NO_CLEARING (val))
+		return false;
+	      for (; next < lo; ++next)
+		if (done (0))
+		  return true;
+	    }
+	  for (; next <= hi; ++next)
+	    if (done (raw ? RAW_DATA_UCHAR_ELT (v, next - lo)
+		      : TREE_INT_CST_LOW (v) & 0xff))
+	      return true;
+	}
+      cleared = !CONSTRUCTOR_NO_CLEARING (val);
+    }
+  else
+    return false;
+  /* What is left of the array is zero, unless it is uninitialized.  */
+  tree dom = TYPE_DOMAIN (atype);
+  if (!cleared || !dom || !tree_fits_uhwi_p (TYPE_MAX_VALUE (dom)))
+    return false;
+  unsigned HOST_WIDE_INT size = tree_to_uhwi (TYPE_MAX_VALUE (dom)) + 1;
+  for (; next < size; ++next)
+    if (done (0))
+      return true;
+  return false;
+}
+
+/* PTR is the value of a pointer argument of a string built-in.  If it
+   points into an array of characters whose value this evaluation knows,
+   such as storage of the constexpr heap or a local array, return the
+   address of a STRING_CST holding BOUND of its characters from there on,
+   or up to the first NUL if BOUND is negative.  Otherwise NULL_TREE.  */
+
+static tree
+known_string_of_pointer (const constexpr_ctx *ctx, tree ptr,
+			 HOST_WIDE_INT bound)
+{
+  STRIP_NOPS (ptr);
+  unsigned HOST_WIDE_INT off = 0;
+  if (TREE_CODE (ptr) == POINTER_PLUS_EXPR)
+    {
+      if (!tree_fits_uhwi_p (TREE_OPERAND (ptr, 1)))
+	return NULL_TREE;
+      off = tree_to_uhwi (TREE_OPERAND (ptr, 1));
+      ptr = TREE_OPERAND (ptr, 0);
+      STRIP_NOPS (ptr);
+    }
+  if (TREE_CODE (ptr) != ADDR_EXPR)
+    return NULL_TREE;
+  tree obj = TREE_OPERAND (ptr, 0);
+  if (TREE_CODE (obj) == ARRAY_REF)
+    {
+      if (!tree_fits_uhwi_p (TREE_OPERAND (obj, 1)))
+	return NULL_TREE;
+      off += tree_to_uhwi (TREE_OPERAND (obj, 1));
+      obj = TREE_OPERAND (obj, 0);
+    }
+  /* The folder reads a string literal itself.  */
+  if (TREE_CODE (obj) == STRING_CST)
+    return NULL_TREE;
+  tree atype = TREE_TYPE (obj);
+  if (TREE_CODE (atype) != ARRAY_TYPE)
+    return NULL_TREE;
+  tree etype = TREE_TYPE (atype);
+  if (!INTEGRAL_TYPE_P (etype) || TYPE_PRECISION (etype) != CHAR_TYPE_SIZE)
+    return NULL_TREE;
+  bool non_constant_p = false, overflow_p = false;
+  tree jump_target = NULL_TREE;
+  tree val = cxx_eval_constant_expression (ctx, obj, vc_prvalue,
+					   &non_constant_p, &overflow_p,
+					   &jump_target);
+  if (non_constant_p || jump_target)
+    return NULL_TREE;
+  auto_vec<char, 64> buf;
+  if (!append_known_chars (val, off, bound, atype, buf))
+    return NULL_TREE;
+  tree str = build_string (buf.length (), buf.address ());
+  tree eltype = cp_build_qualified_type (TYPE_MAIN_VARIANT (etype),
+					 TYPE_QUAL_CONST);
+  TREE_TYPE (str) = build_array_type_nelts (eltype, buf.length ());
+  return build_address (str);
+}
+
 /* Attempt to evaluate T which represents a call to a builtin function.
    We assume here that all builtin functions evaluate to scalar types
    represented by _CST nodes.  */
@@ -2595,6 +2734,7 @@ cxx_eval_builtin_function_call (const constexpr_ctx *ctx, tree t, tree fun,
      return constant false for a non-constant argument.  */
   constexpr_ctx new_ctx = *ctx;
   new_ctx.quiet = true;
+  bool value_ptr[2] = { false, false };
   for (i = 0; i < nargs; ++i)
     {
       tree arg = CALL_EXPR_ARG (t, i);
@@ -2609,7 +2749,10 @@ cxx_eval_builtin_function_call (const constexpr_ctx *ctx, tree t, tree fun,
 	  if (TREE_CODE (arg) == ADDR_EXPR)
 	    arg = TREE_OPERAND (arg, 0);
 	  else
-	    strop = false;
+	    {
+	      strop = false;
+	      value_ptr[i] = true;
+	    }
 	}
 
       /* If builtin_valid_in_constant_expr_p is true,
@@ -2646,6 +2789,33 @@ cxx_eval_builtin_function_call (const constexpr_ctx *ctx, tree t, tree fun,
       args[i] = arg;
     }
 
+  /* A string argument that is not the address of a constant array may
+     still point to characters this evaluation knows, on the constexpr heap
+     or in a local array: hand them to the folder as a STRING_CST.  */
+  if (strops)
+    {
+      HOST_WIDE_INT bound = -1;
+      switch (DECL_FUNCTION_CODE (fun))
+	{
+	case BUILT_IN_MEMCMP:
+	case BUILT_IN_MEMCHR:
+	  bound = (nargs == 3 && tree_fits_uhwi_p (args[2])
+		   ? (HOST_WIDE_INT) tree_to_uhwi (args[2]) : -2);
+	  break;
+	case BUILT_IN_STRNLEN:
+	  bound = (nargs == 2 && tree_fits_uhwi_p (args[1])
+		   ? (HOST_WIDE_INT) tree_to_uhwi (args[1]) : -2);
+	  break;
+	default:
+	  break;
+	}
+      if (bound != -2)
+	for (i = 0; i < strops && i < nargs; ++i)
+	  if (value_ptr[i])
+	    if (tree str = known_string_of_pointer (&new_ctx, args[i], bound))
+	      args[i] = str;
+    }
+
   bool save_ffbcp = force_folding_builtin_constant_p;
   force_folding_builtin_constant_p |= ctx->manifestly_const_eval == mce_true;
   tree save_cur_fn = current_function_decl;
```

</details>

<details><summary>B, library half</summary>

```diff
diff --git a/libstdc++-v3/include/bits/char_traits.h b/libstdc++-v3/include/bits/char_traits.h
index bd755f7368..c48da4bc89 100644
--- a/libstdc++-v3/include/bits/char_traits.h
+++ b/libstdc++-v3/include/bits/char_traits.h
@@ -375,6 +375,8 @@ _GLIBCXX_BEGIN_NAMESPACE_VERSION
 #if __cplusplus >= 201703L
 	if (std::__is_constant_evaluated())
 	  {
+	    if (__builtin_constant_p(__builtin_memcmp(__s1, __s2, __n)))
+	      return __builtin_memcmp(__s1, __s2, __n);
 	    for (size_t __i = 0; __i < __n; ++__i)
 	      if (lt(__s1[__i], __s2[__i]))
 		return -1;
@@ -391,7 +393,11 @@ _GLIBCXX_BEGIN_NAMESPACE_VERSION
       {
 #if __cplusplus >= 201703L
 	if (std::__is_constant_evaluated())
-	  return __gnu_cxx::char_traits<char_type>::length(__s);
+	  {
+	    if (__builtin_constant_p(__builtin_strlen(__s)))
+	      return __builtin_strlen(__s);
+	    return __gnu_cxx::char_traits<char_type>::length(__s);
+	  }
 #endif
 	return __builtin_strlen(__s);
       }
```

</details>

### C: calls that reach a metafunction

| Compiler | The same `substitute` and `extract` again, a call | A new one, a call | One label, front end | Full | Mark |
|---|---|---|---|---|---|
| 16.2 | 904 k | 912 k | 54.09 G | 55.27 G | `measured` |
| trunk | 816 k | 853 k | 50.12 G | 51.21 G | `measured` |
| 16.2, C for five metafunctions | 30 k | 910 k | 54.09 G, 0 % | 55.29 G | `measured` |
| 16.2, C for all, an upper bound | 29 k | 913 k | 53.57 G, −1.0 % | 54.74 G | `measured` |

- The patch keeps a call cacheable when it reaches only `substitute`, `extract` or a `reflect_*` function. A repeated call then costs 30 k instead of 904 k.
- For our code it gives nothing: lvgl26 already stores each scan in a variable template and reads the table, as the earlier finding advised.
- Even caching every metafunction, which is wrong, saves 1 % of one label. The other 99 % are first calls.
- Trunk only renamed the flag, from `metafns_called` to `state_dependent` (`a502b86d14`, 2026-08-01). Its comment still says "until we figure out which exact metafunctions need this". `source`
- `substitute` and `extract` are only arguably pure. A substitution can instantiate, and whether that succeeds may depend on what is complete where the call happens.

This is an experiment, not a patch for review.

<details><summary>C, for five metafunctions</summary>

```diff
diff --git a/gcc/cp/constexpr.cc b/gcc/cp/constexpr.cc
index 7a09f2947c..02f13a7c94 100644
--- a/gcc/cp/constexpr.cc
+++ b/gcc/cp/constexpr.cc
@@ -3929,6 +3929,25 @@ cxa_allocate_and_throw_exception (location_t loc, const constexpr_ctx *ctx,
   return var;
 }
 
+/* Experiment: true if FUN is a metafunction whose result depends on its
+   arguments alone, so that a call which reaches it may still be cached.
+   substitute names a specialization, which is the same entity whenever it is
+   named; extract reads a constant; the reflect_* ones reflect a value.  */
+
+static bool
+pure_metafunction_p (tree fun)
+{
+  static const char *const pure[] = {
+    "substitute", "extract", "reflect_constant", "reflect_object",
+    "reflect_function"
+  };
+  const char *name = IDENTIFIER_POINTER (DECL_NAME (fun));
+  for (const char *p : pure)
+    if (strcmp (name, p) == 0)
+      return true;
+  return false;
+}
+
 /* Subroutine of cxx_eval_constant_expression.
    Evaluate the call expression tree T in the context of OLD_CALL expression
    evaluation.  */
@@ -4024,7 +4043,8 @@ cxx_eval_call_expression (const constexpr_ctx *ctx, tree t,
 	  *non_constant_p = true;
 	  return t;
 	}
-      ctx->global->metafns_called = true;
+      if (!pure_metafunction_p (fun))
+	ctx->global->metafns_called = true;
       tree e = process_metafunction (ctx, fun, t, non_constant_p, overflow_p,
 				     jump_target);
       if (*jump_target)
```

</details>

### D: `std::vector<unsigned char>(n)`

The cost has two equal parts: value-initialising each element through four calls, and destroying each element again in the destructor.
libstdc++ walks a trivially destructible range in constant evaluation (`_Destroy` in `bits/stl_construct.h`), calling `destroy_at` on every element.

| Library | An element of `vector(20001)` | One label, front end | Full | Mark |
|---|---|---|---|---|
| 16.2 | 86.3 k | 54.09 G | 55.27 G | `measured` |
| trunk | 87.2 k | 50.12 G | 51.21 G | `measured` |
| 16.2, one array placement new over the range | 53.4 k | | | `measured` |
| 16.2, a placement new per element, without the call chain | 67.8 k | | | `measured` |
| 16.2, no destroy loop for a trivial type | 53.6 k | | | `measured` |
| 16.2, the array placement new and no destroy loop | 19.3 k | 53.57 G, −1.0 % | 54.72 G | `measured` |
| For comparison: `new unsigned char[n]()` | 19.0 k | | | `measured` |

- Both changes together reach the floor of `new[]`. Neither alone gets more than a third of the way.
- One label barely uses a vector of bytes. The gain is for the font and picture readers, which `CLAUDE.md` already steers to `reserve` and pointers.
- Trunk changed `stl_vector.h` and `stl_uninitialized.h` (`acfdad706d`, `allocate_at_least`, 2026-04-03; `79e2956290`, `uninitialized_move`, 2026-02-26). Neither touches this path. `source`
- The array placement new is only valid where the range is a whole array, as in `vector(n)`. libstdc++ would need a narrower condition than this experiment's.
- Dropping the destroy loop loses one diagnosis: a read through a saved pointer after `clear()` compiles, where 16.2 says "outside its lifetime". After `pop_back()` it is still diagnosed. That may be why libstdc++ has the loop.

This is an experiment, not a patch for review.

<details><summary>D, both changes</summary>

```diff
diff --git a/libstdc++-v3/include/bits/stl_construct.h b/libstdc++-v3/include/bits/stl_construct.h
index 4a0650f3cb..c954656299 100644
--- a/libstdc++-v3/include/bits/stl_construct.h
+++ b/libstdc++-v3/include/bits/stl_construct.h
@@ -224,11 +224,6 @@ _GLIBCXX_BEGIN_NAMESPACE_VERSION
       if constexpr (!is_trivially_destructible<_Value_type>::value)
 	for (; __first != __last; ++__first)
 	  std::_Destroy(std::addressof(*__first));
-#if __cpp_constexpr_dynamic_alloc // >= C++20
-      else if (std::is_constant_evaluated())
-	for (; __first != __last; ++__first)
-	  std::destroy_at(std::addressof(*__first));
-#endif
 #else
       std::_Destroy_aux<__has_trivial_destructor(_Value_type)>::
 	__destroy(__first, __last);
diff --git a/libstdc++-v3/include/bits/stl_uninitialized.h b/libstdc++-v3/include/bits/stl_uninitialized.h
index ae4442f385..556a5de715 100644
--- a/libstdc++-v3/include/bits/stl_uninitialized.h
+++ b/libstdc++-v3/include/bits/stl_uninitialized.h
@@ -981,14 +981,27 @@ _GLIBCXX_BEGIN_NAMESPACE_VERSION
     inline _ForwardIterator
     __uninitialized_default_n(_ForwardIterator __first, _Size __n)
     {
+      typedef typename iterator_traits<_ForwardIterator>::value_type
+	_ValueType;
 #ifdef __cpp_lib_is_constant_evaluated
       if (std::is_constant_evaluated())
-	return __uninitialized_default_n_1<false>::
-		 __uninit_default_n(__first, __n);
+	{
+#if __cpp_constexpr >= 202406L
+	  // Experiment: no call per element for a trivial type.
+	  if constexpr (is_pointer<_ForwardIterator>::value
+			&& __is_trivial(_ValueType))
+	    {
+	      if (__n > 0)
+		::new (static_cast<void*>(__first)) _ValueType[__n]();
+	      return __n > 0 ? __first + __n : __first;
+	    }
+	  else
+#endif
+	  return __uninitialized_default_n_1<false>::
+		   __uninit_default_n(__first, __n);
+	}
 #endif
 
-      typedef typename iterator_traits<_ForwardIterator>::value_type
-	_ValueType;
       // See uninitialized_fill_n for the conditions for using std::fill_n.
       constexpr bool __can_fill
 	= __and_<is_integral<_Size>, is_copy_assignable<_ValueType>>::value;
```

</details>

### What trunk improved

| Change on trunk | Effect on us | Mark |
|---|---|---|
| `members_of` skips undeclared built-ins: `8ee8f2beab`, PR 125819, 2026-06-18. Not on the GCC 16 branch | `members_of(^^::)`: 4142 members and 0.63 G on 16.2; 674 members and 0.12 G on trunk | `measured`, `source` |
| The same, for one label | 54.09 G on 16.2, 50.12 G on trunk. 16.2 with only that commit cherry-picked: 49.16 G, −9.1 %, and 50.30 G full | `measured` |
| `identifier_of`, the string built-ins, the metafunction cache, `vector(n)` | No change worth naming: within 10 % for C, within noise for A, B and D | `measured` |

- `detail::api_table` filters the built-ins by their source line. On trunk it finds none, and the filter costs nothing: it can stay.
- The commit cherry-picks onto 16.2 cleanly. Trunk as a whole is 1 G above 16.2 with the commit alone; the cause was not looked for.
- A backport to GCC 16 would give 16.2's users most of trunk's gain. A draft of that request is kept with the other drafts.

### All together

| Compiler and library | One label, front end | Full | Mark |
|---|---|---|---|
| 16.2 | 54.09 G | 55.27 G | `measured` |
| trunk | 50.12 G | 51.21 G | `measured` |
| 16.2 with A, B, C and D, compiler and library | 40.83 G, −24.5 % | 41.97 G, −24.1 % | `measured` |
| 16.2 with A and B's library half only | 42.09 G, −22.2 % | 43.21 G | `measured` |

- A and B's library half overlap: both remove the length loop of `identifier_of`. Together they give 22 %, not 32 %.
- The output did not change. lvgl26's reference data, printed by `tools/reference/reference.cpp`, is byte-identical between 16.2 and 16.2 with everything. It also equals the committed `docs/reference/lvgl-v9.6.0.json`.

### GCC's tests

| Test run | 16.2 | 16.2 with every patch | Mark |
|---|---|---|---|
| `g++.dg`, `reflect/*` and `*constexpr*` | 8777 passes, 34 expected failures, 13 unsupported | The same | `measured` |
| libstdc++, `21_strings`, `23_containers/vector`, `20_util/specialized_algorithms` | 1478 passes, 14 expected failures, 39 unsupported | The same | `measured` |
| trunk, `g++.dg/reflect`, without and with A | 4962 passes, 38 expected failures | The same | `measured` |
| 16.2, `g++.dg/reflect`, without and with the `members_of` commit | 2112 passes, 15 expected failures | 2113 passes, 15 expected failures: the commit adds a test | `measured` |

- The first front-end half of B passed the same 8777 tests while giving wrong answers on `#embed` data. A real patch needs tests of its own: heap, local arrays, offsets, uninitialized bytes, and `#embed`.
- The g++ tests ran with `make check-c++ RUNTESTFLAGS="dg.exp=reflect/* *constexpr*"`, the library tests with `conformance.exp` and the three directories.
- A release-checking build does not check the trees the patches build. A build with `--enable-checking=yes` was not run.

### Not done

- A build with tree checking, and the full testsuite. Only the directories above ran.
- B, C and D on trunk. Only A was carried to trunk.
- Files other than the one-label file: `signal.hpp`, `look.cpp`, the fonts. D is likely to matter there.
- The editor stubs and the compile-fail tests of lvgl26 under the patched compilers.

## What was reported to GCC

GCC declines a legally significant contribution that includes or derives from LLM output. Its policy allows such tools for research, analysis and bug reports. [GCC AI policy](https://gcc.gnu.org/ai-policy.html), adopted 2026-07-29. `source` 2026-11-20

So each report holds a reproducer and numbers. An experiment's diff is attached only as "LLM-generated, reference only, not a patch submission".

| Report | What it says | State on 2026-11-20 |
|---|---|---|
| [128900](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=128900) | Candidate A: `identifier_of` builds its view from a pointer though it knows the length | Open. The diff was first attached as a patch; a comment corrects that |
| [128901](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=128901) | Candidate B: the string built-ins do not fold on the constexpr heap | Closed as a duplicate of [94082](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94082). A comment there names the library half, which needs no compiler change |
| [128902](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=128902) | Candidate D: `vector<T>(n)` constructs and destroys each element through calls | Open |
| [125819](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=125819) | A comment asks for a backport to GCC 16 of the commit that makes `members_of` skip built-ins | Fixed on trunk; no answer on the backport |
| [128909](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=128909) | A precompiled header drops the `pre` and `post` of templates, depending on addresses | Open |
| [128870](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=128870) | "taking address of an immediate function" at random with `-fdeclone-ctor-dtor` | Open; others confirmed it on MinGW |
| [128871](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=128871) | The frame of `std::noop_coroutine` holds functions of the wrong type | Open |

Candidate C, the cache of metafunction calls, was not reported: it gains lvgl26 at most 1 %.

Related reports found on the way: [106889](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=106889), `__builtin_strlen` in constant evaluation, and [127894](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=127894), the evaluator's overhead on array scans. `source` 2026-11-20

## Two levers designed: fonts drawn once, a theme of one slot

Proposals for the owner, measured on 2026-11-20 against `main` at `cc81ec1`. Every prototype is a copy of the files in a scratch directory; nothing in `include/` changed. The flags are those of `host-v96`, Debug at `-O0`, as above.

### A. Fonts drawn once

A font is drawn where `type(tok.text.title)` is evaluated, so every file that names it draws it again. The printer port draws its eight sizes in seven files.

#### Short answers

| Question | Answer | Mark |
|---|---|---|
| Can a constant tree point at a font whose glyphs are not in the file? | Yes. A style keeps only the address, and the address of an `extern const` object is a constant. GCC folds a look's font to the address of the object in another file. | `measured` |
| What does the port gain? | The whole port goes from 1949 G to 1140 G (−42 %). The Print page goes from 326 G to 88 G. | `measured` |
| What does the program gain? | Constant data in `__TEXT` goes from 1,046,379 to 576,283 bytes (−45 %). Today every file keeps a copy of each font it draws. | `measured` |
| Does the output change? | No. The seven pages that `printer_shot` writes are equal byte for byte. | `measured` |
| What must the user write? | One source file that draws the fonts, and a header that names them. The pages stay as they are. | `measured` |

#### The routes

| Route | Print page | What breaks | Mark |
|---|---|---|---|
| Today: each file draws its fonts | 325.6 G | Nothing: the cost is the problem | `measured` |
| 1. `extern const` fonts, drawn in one file | 88.3 G; the file with the fonts 409.6 G | Nothing found. It also compiles under the sanitizer flags | `measured` |
| 1, behind `lv::typeface` (pages unchanged) | 90.4 G; the file with the fonts 403.9 G | Nothing found. It also compiles under the sanitizer flags | `measured` |
| 2. A CMake helper that writes the fonts' file | The same as route 1 | Not prototyped. Its only gain over route 1 is one file per font, see below | `measured` for one file per font |
| 3. The fonts in a C++ module | 90.8 G; the module 259.7 G | The program grows (`__TEXT,__const` 1,146,939 bytes): each importer writes the bitmaps again. It needs `-Wno-expose-global-module-tu-local`, because the font files have internal linkage. A `pre` inside a module does not link (`__tu_has_violation` is undefined, in a three-line test). Module scanning is off for ccache | `measured` |
| 4. A variable template that draws, with `extern template` | 90.4 G; the instantiating file 261.5 G | It does not link: the font's template arguments (`tok`, `type`) have internal linkage, so the declaring and the instantiating file name different objects. The standard also lets a compiler instantiate a `const` variable of literal type anyway | `measured` |

- Why each file keeps a copy today: an embedded font file such as `constexpr unsigned char inter_regular[]` has internal linkage. A bitmap names the file in its template arguments, so the same glyphs get a different symbol in each file, and the linker keeps all of them. `inline constexpr` would merge them, but CLAUDE.md forbids it for an embedded array (the sanitizer bug). `measured`
- The header that the pages read must not `#embed` the font files. At `-O0` each page then still writes both files into its object, 80 KB each time, though it draws nothing (route 1 behind the typeface, first version: `__TEXT,__const` 1,146,907 bytes). `measured`
- One file for all eight fonts costs 404 to 410 G. One file per font costs 74 to 78 G each: about 30 G of headers and tokens, plus the font. That is more work in total, but a shorter build on several cores. `measured`
- A look takes its font from the design's typeface (`font_of<D, T>` in `look.hpp`). With the drawn fonts in the typeface, a file whose look has a typography entry went from 105.8 G to 49.7 G, and GCC folded the look's font to the object in the other file. `measured`
- No code of lvgl26 reads a font's value in constant evaluation (no `line_height`, no `dsc`). Only its address matters. `measured`

#### Route 1 behind `lv::typeface`: the user's code

Today, `fonts.hpp` embeds the files, and every page draws what it names:

```cpp
// fonts.hpp
constexpr unsigned char inter_regular[] = {
#embed "fonts/Inter-24pt-Regular.ttf"
};
constexpr unsigned char inter_medium[] = {
#embed "fonts/Inter-24pt-Medium.ttf"
};
constexpr auto type = lv::typeface({{"Inter 24pt", 400, inter_regular}, {"Inter 24pt", 500, inter_medium}});

// print_page.cpp: draws the font here
constexpr auto title_font = type(tok.text.title);
```

Proposed: the header names the fonts and lists them in the typeface, and one file draws them. The pages do not change:

```cpp
// fonts.hpp: no font file here
extern const lv::drawn_font title, body, subtitle, status, detail, heading, small, label;
constexpr auto type = lv::typeface({{tok.text.title, title}, {tok.text.body, body} /* ... */});

// fonts.cpp: the one file that draws
constexpr unsigned char inter_regular[] = {
#embed "fonts/Inter-24pt-Regular.ttf"
};
constexpr unsigned char inter_medium[] = {
#embed "fonts/Inter-24pt-Medium.ttf"
};
constexpr auto files = lv::typeface({{"Inter 24pt", 400, inter_regular}, {"Inter 24pt", 500, inter_medium}});
constinit const lv::drawn_font title = files.draw(tok.text.title);
// ... one line for each font

// print_page.cpp: unchanged, and draws nothing
constexpr auto title_font = type(tok.text.title);
```

- `lv::drawn_font` is new: an object that holds an LVGL font, so that an example names no `lv_font_t`. `files.draw(token)` is new too: it always draws.
- `type(token)` gives the address of the listed font whose family, weight and size match the token. A typography token that is not on the list is a compile error that names the token and the file that draws the fonts. A design's `.type` is the same object, so looks and themes find the fonts the same way.
- The prototype is `fonts.hpp` and `fonts.cpp` of the printer in a scratch copy, with the typeface written in the example. Building it into `lv::typeface` is the work.

#### Signal

`lv/signal_type.hpp` costs every file that includes it about 159 G, because it draws Signal's fonts. lvgl26 is a header-only target (`INTERFACE`), so no file of its own can draw them. The proposal is a compiled target, `lvgl26::signal_type`, with one source file that draws Signal's fonts. `lv/signal_type.hpp` would declare them, and a program that uses it would link the target. This was not prototyped.

### B. A theme that stores only what it sets

#### Where a theme's cost goes

| Probe | G | Mark |
|---|---|---|
| A file that includes `lv/lv.hpp` and `lv/theme.hpp` | 9.4 | `measured` |
| `+` the style struct defined | 23.2 (+13.8, paid by any file with a style) | `measured` |
| `+` the theme's spec type defined | 33.9 (+9.6) | `measured` |
| `+` an empty spec as a constant | 42.5 (+8.5) | `measured` |
| `lv::theme({})` | 44.3 | `measured` |
| Each further theme | +8.3 | `measured` |

- The run-time side already stores only what is set. `theme_entries` keeps one entry for each style that sets something, and `apply_theme` walks only those. The cost is in the compiler, which builds the whole spec of 54 widgets × 16 slots × 140 members before reading it.
- A profile of eight empty specs shows the time spread over GCC's walks of the initializer: `walk_tree_1`, `cxx_eval_constant_expression`, `reduced_constant_expression_p`, `potential_constant_expression_1`, `cp_fold`. No single function stands out: the size of the value is the cost. `measured`
- A spec of the same shape whose members are a plain struct costs 2.1 G for each evaluation. Give that struct one constructor besides the default one, as `lv::arg` has, and it costs 6.2 G. So two thirds of an unset member's cost comes from `lv::arg` having constructors (lever C below). `measured`

#### The representations

Fewer slots for each widget make the spec smaller. The slots after `style` are first the states and then the parts.

| Slots per widget in a theme | A further theme | `tests/theme.cpp` | `tests/signal.cpp` | `examples/quick_start` | Mark |
|---|---|---|---|---|---|
| 16, today | 8.3 G | 73.7 G | 58.8 G | 107.1 G | `measured` |
| 9: `style` and the states. A part is written with `lv::on` | 4.8 G | 68.9 G (−7 %) | 53.5 G (−9 %) | 101.2 G (−6 %) | `measured` |
| 1: `style` only. States and parts are written with `lv::on` | 0.6 G | 60.3 G (−18 %) | 44.7 G (−24 %) | 91.9 G (−14 %) | `measured` |

- The variants were made by cutting the slot list in `look_items` in a copy of `theme.hpp`. A script rewrote Signal's theme and `tests/theme.cpp` (`.style_x = {...}` into `| lv::on("x", {...})`). Both rewritten `tests/theme.cpp` passed at run time, so the theme sets the same styles. `measured`
- The first theme of a file gains as much as each further one: the spec is smaller wherever it is built.

#### The user's code

`lv::theme({...})` stays, and so does `.button = {.style = ...}`. What moves is a slot other than `style`. It becomes a layer of `.style`, which is already the form that `docs/themes.md` shows and that a node takes:

```cpp
// Today
.arc = {.style = {.arc_color = Tok.color.line, .arc_width = Tok.stroke.track},
        .style_indicator = {.arc_color = Tok.color.active},
        .style_knob = {.bg_opa = hidden}},
.button = {.style = {.bg_color = Tok.color.active},
           .style_pressed = {.bg_color = Tok.color.surface_raised}},

// One slot
.arc = {.style = lv::style({.arc_color = Tok.color.line, .arc_width = Tok.stroke.track})
                 | lv::on("indicator", {.arc_color = Tok.color.active})
                 | lv::on("knob", {.bg_opa = hidden})},
.button = {.style = lv::style({.bg_color = Tok.color.active})
                    | lv::on("pressed", {.bg_color = Tok.color.surface_raised})},
```

- A slot that is left in an old theme stops compiling. The message must name the new form: "`.style_pressed` is written `.style = ... | lv::on("pressed", {...})` in a theme". That needs a compile-fail test.
- A node keeps its 16 slots: `kept<W>` stores a node's props once, so this cost does not repeat there. Whether nodes follow later is a separate question.

#### What it touches

- `include/lv/theme.hpp`: the slot list of `look_of` (only `style`), and the message in `add_theme_layers`, which today says to use `.style`.
- `include/lv/signal.hpp`: its four slot members become layers. `examples/theme`, `tests/theme.cpp`, `docs/themes.md` and the theme compile-fail tests change too; a slot of a node (`.style_pressed` in a tree) does not.
- `include/lv/detail/theme_styles.hpp`: nothing changes. The stored entries stay as they are.
- `include/lv/look.hpp`: nothing changes. A look is a style value, so it stands where a style does, and it also stands in `lv::on`.
- The editor stubs: `tools/editor/reference_stubs.py` writes `stub::look` and `stub::theme_spec` with the 16 slots. They get one, and `tests/editor/api_uses.cpp` follows.
- Run time: the same entries. ROM and the time to set up a theme were not measured. They should not move, because the stored list is built by the same code from the same styles.

### What the owner decided

Decided 2026-11-20, over the proposals above.

| # | Decision |
|---|---|
| A1 | Fonts drawn once are built: route 1 behind `lv::typeface`, with `lv::drawn_font` and `typeface::draw`. |
| A2 | Signal's fonts become a compiled target, `lvgl26::signal_type`, as a second step after A1 works for user code. It is the first compiled part of lvgl26, so it is prototyped and measured first. |
| A3 | The form that examples, the printer port and the docs show is one source file per font (74 to 78 G each, built in parallel), not one file that draws them all. |
| B1 | A theme has one slot, `style`. States and parts are `lv::on` layers. |
| B2 | A node keeps its 16 slots. |
| C | `lv::arg` without constructors is a measured spike: `tests/large_tree.cpp` and the eight probes, what it gains and what it changes for users, reported before any decision to build. |

The order of the builds, one pull request each: `lv::shown`, fonts drawn once, the one-slot theme, the Signal target, the `lv::arg` spike.

### C. An unset member without constructors (found on the way)

Two thirds of the cost of an unset member comes from `lv::arg` having constructors (see B). Every node pays that too, for every member it leaves out. Whether `lv::arg`'s conversions can move to the values it takes (`lv::raw`, `lv::token`, a name), so that `lv::arg` itself becomes an aggregate, has not been tried. It needs a spike of its own, measured on `tests/large_tree.cpp`.

### Recommendation

- A: route 1 behind `lv::typeface`. It adds `lv::drawn_font` and `typeface::draw`, and the typeface takes a list of drawn fonts. The printer gets `fonts.cpp`. Signal gets `lvgl26::signal_type` as its own step after that.
- B: one slot (`style`) in a theme, with every state and part as `lv::on`. A theme then costs 0.6 G instead of 8.3 G, and a file with Signal 24 % less.
- C: a spike after A and B.

## Tried and ruled out

| Idea | Result | Mark |
|---|---|---|
| The linear name lookups (`find`) per setter | Stubbing `find` saved nothing on the base and the label kind | `measured` |
| The setters' value types (`value_type`) | Replacing it by the raw type saved 0.3 to 0.4 G | `measured` |
| The doc comments read for angles (`angle_type_of`) | Stubbing it saved under 0.1 G: the reader is already cheap | `measured` |
| The deprecated-setter and keyword lists | Stubbing both saved 0.45 to 0.6 G | `measured` |
| Matching style ids by a lower-case name read once | No gain as written; stubbing the match gives at most 1.4 G | `measured` |
| A hand-written prefix loop instead of `starts_with` | 0.5 G a pass, worse than fix 7's 0.4 G | `measured` |

## Not measured

- Linux, the `host-v10` preset, the sanitizers and the analyzer. Every number is `host-v96` on macOS.
- That builtins sit at line 0 or 1 on Linux too. Fix 5 must keep its 2026 functions there; a test can count them.
- The whole compile-fail suite with the fixes: 13 of 277 were run. Their messages were not compared.
- A precompiled header under ccache, under CMake's `target_precompile_headers`, and under the analyzer.
- Why `tests/look.cpp` costs 280 G and `tests/tokens.cpp` 124 G. `look.cpp` draws a font through its design, which is likely most of it.
- What is left after the fixes. A further widget kind still costs 4.4 G, and the first theme 36.6 G on top of its token files.
- A theme that stores only what it sets (cause 8): designed and measured in "Two levers designed" above.
- What a font draw itself costs inside: see [fonts.md](fonts.md).
- LVGL's C files, mp-units, and the time a build takes on a runner.
