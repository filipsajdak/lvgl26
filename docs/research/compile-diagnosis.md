# Compile time: where it goes, and what to fix

Why one file of lvgl26 costs GCC 16.2 so much, measured part by part, with the fixes ranked by what they save.
Measured on 2026-11-19; every mark below has that date.
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
- A theme that stores only what it sets (cause 8): the gain comes from cutting the slots, which changes the API.
- What a font draw itself costs inside: see [fonts.md](fonts.md).
- LVGL's C files, mp-units, and the time a build takes on a runner.
