# Compile time: seeing it, and making it cheaper

What GCC 16.2 can tell us about constant evaluation, which tools add to it, and which levers cut the cost.
Checked 2026-11-17.
The marks are in [the README](README.md).

## Why it matters

The compiler does the work of lvgl26: fonts, themes, trees and reflection scans run in constant evaluation.
The analyzer job of the CI grew from 15 to 37 minutes over four pull requests, and one run of 56 minutes lost its runner.

| Fact | Mark |
|---|---|
| `-ftime-report` puts 85 to 90 percent of many files in "constant expression evaluation" (17 to 75 s a file). | `measured` 2026-11-17 |
| With `-fanalyzer`, `font_bytes.cpp` goes from 26 s to 91 s. "Analyzer: processing worklist" alone is 68 s. | `measured` 2026-11-17 |
| Peak memory of one file is 0.7 to 4.5 GB (Linux arm64, `-fanalyzer`). | `measured` 2026-11-17 |
| Sixteen files of the treadmill and printer ports: 657 s of CPU without the analyzer, 2224 s with it. | `measured` 2026-11-17 |
| On Linux arm64 the branch of per-glyph fonts is not slower than main: `signal_type` 0:39 against 0:53, `font` 0:33 against 0:51. | `measured` 2026-11-17 |
| The GCC 16 notes call the analyzer "unlikely to be usable on production C++ code in this release because of scaling issues". | `source` [GCC 16 changes](https://gcc.gnu.org/gcc-16/changes.html) 2026-11-17 |

## What GCC 16.2 shows

GCC counts operations per outermost constant evaluation, in `cxx_eval_constant_expression`.
It prints the count only when the limit is passed.
Nothing in GCC 16.2 names the function that costs the time.

| Option | What it shows | Mark |
|---|---|---|
| `-ftime-report`, `-ftime-report-details` | Wall time and GC memory per phase. Rows for constant evaluation, template instantiation, constraints and name lookup. | `source` [timevar.def](https://raw.githubusercontent.com/gcc-mirror/gcc/master/gcc/timevar.def), `measured` |
| `-ftime-report` with `-fdiagnostics-format=sarif-file` | The same rows as JSON, in `runs[0].invocations[0].properties["gcc/timeReport"]`. `-fdiagnostics-format=json` is gone in 16. | `measured` |
| `-fmem-report`, `-fstats` | Memory by size class, and front-end table sizes. No constexpr counter. | `source` [GCC manual](https://gcc.gnu.org/onlinedocs/gcc/Developer-Options.html) |
| `-fdump-lang-tinst-lineno` | The order of instantiations and where each started. No times. New in 16. | `source`, `measured` |
| `-fconstexpr-ops-limit` | The only operation counter, shown on overflow. Bisecting the limit gives the count: `sum(100000)` needs 1,900,018. | `source` constexpr.cc, `measured` |
| `-fconstexpr-cache-depth` | How deep calls are cached. `fib(24)` needs 92,030 operations at depth 8 and 1,650,538 at depth 0. | `source`, `measured` |
| `__builtin_constexpr_diag(kind, tag, message)` | A note, warning or error from inside constant evaluation, with the expansion stack. New in 16, not in the manual. | `source`, `measured` |
| `-wrapper gdb,--args` | `cc1plus` under a debugger. Homebrew's build has symbols, so breakpoints on `cxx_eval_*` work, but no variables. | `measured` |

How the cache works, read in `cxx_eval_call_expression`:

- A result is cached only for a call below the cache depth, with constant arguments.
- The key hashes and compares all argument values, so a big array passed by value costs a hash on every call.
- A call that reaches a `std::meta` function is not cached in GCC 16.
- A heap allocation that escapes the call makes it uncacheable, so a `std::vector` result is not cached and `std::define_static_array` is the way out.
- Each translation unit has its own cache.

All `source` constexpr.cc, 2026-11-17.

## Plugins and external tools

| Tool | What it gives | Limit | Mark |
|---|---|---|---|
| A GCC plugin on `PLUGIN_FINISH_DECL` | Time per constexpr declaration: read the "constant expression evaluation" row at the start and the end. An 80-line spike printed 0.074 s and 0.183 s for two variables. | `static_assert` and consteval calls inside bodies land on the next declaration. Needs `-ftime-report` so the timer exists. Needs plugin headers: Homebrew has them, Debian has `gcc-16-plugin-dev`. | `measured`, `source` plugin.def |
| [gcc-time-trace-plugin](https://github.com/shouth/gcc-time-trace-plugin) | A Chrome trace of headers and functions. It built and ran on 16.2 after four build changes. | Constant evaluation shows as an unnamed gap. | `measured` |
| [Externis](https://github.com/royjacobson/externis) | The same idea. | Did not build against 16.2. | `measured` |
| Clang `-ftime-trace`, templight, ClangBuildAnalyzer | Template and constant-evaluation traces. | Clang cannot parse our headers (see [lsp.md](lsp.md)). | `summary` |
| `sample` (macOS), `perf record` (Linux) on `cc1plus` | Where the interpreter spends time. | Shows compiler internals (hash tables, `cxx_eval_constant_expression`), not our functions. | `measured` |
| ninja `.ninja_log` with [ninjatracing](https://github.com/nico/ninjatracing) | Wall time per file as a Chrome trace. | Files, not functions. | `source` |
| [Perfetto](https://perfetto.dev/docs/getting-started/other-formats) | SQL over any Chrome trace. | Needs a trace to read. | `source` |
| Upstream `-ftime-trace` | An RFC patch of May 2026 writes the total compile time only. | Not in 16.2. | `source` [patch](https://gcc.gnu.org/pipermail/gcc-patches/2026-May/715621.html) |

`/usr/bin/time -l g++-16` counts the driver, not `cc1plus`: its instruction number is wrong for this.
A script has to start `cc1plus` itself (`g++ -###` prints the command), as the CLAUDE.md rule says.
`measured` 2026-11-17.

## A short list for insight

1. A script that compiles one file and prints one JSON row: wall time, instructions, peak memory and the `-ftime-report` rows. It feeds the agent and a nightly trend.
2. The SARIF time report for proportions, instructions for the trend, as they vary less than seconds on a busy machine. A rust-lang measurement says the same for `instructions:u` (`summary`).
3. The operation count by bisecting `-fconstexpr-ops-limit`, for small targets only: about 30 compiles each.
4. The plugin on `PLUGIN_FINISH_DECL` when a file needs a per-declaration answer.
5. [github-action-benchmark](https://github.com/benchmark-action/github-action-benchmark) takes the JSON and keeps history. Its default alert threshold of 200 percent is useless: set a few percent (`source`).

Nothing found shows per-header or per-line compile cost in an editor for GCC-only code.

## The report tool

`tools/compile_report.py` prints one JSON row of compile cost per file.
It builds item 1 of the list above.
Standard library only; the tests are `tools/test_compile_report.py` (`ctest -R compile_report`).

```sh
python3 tools/compile_report.py build/host-v96 tests/smoke.cpp tests/CMakeFiles/font.dir/font.cpp.o --jobs 2 --table
```

- A file is a source path or a ninja object name from `compile_commands.json`.
- The tool asks `g++ -###` for the `cc1plus` command and runs that, so the instruction count is the compiler's. The field `mode` is `driver` when no `cc1plus` line was found.
- The times come from `-ftime-report` with `-fdiagnostics-add-output=sarif:file=...`, which writes no file next to the source.
- `--table` sorts by `constexpr_s`. `--jobs N` runs N files at once. `--ops` adds `ops_between`, the range from bisecting `-fconstexpr-ops-limit`, and is slow.
- Without `/usr/bin/time` (the CI image has none) the tool uses `wait4` resource usage for peak memory, which covers that child alone. `LVGL26_NO_TIME=1` forces this.
- The exit status is 1 when a compile fails, and `error` holds the first error line.

| Field | Meaning |
|---|---|
| `wall_s` | Seconds of the whole `cc1plus` run, including code generation. |
| `instructions` | Instructions retired. macOS only, `null` on Linux and with `timer` `rusage`. The number to compare. |
| `timer` | How memory was measured: `time-l`, `time-v` or `rusage`. |
| `peak_rss_mb` | Peak resident memory. |
| `constexpr_s` | The row "constant expression evaluation". |
| `template_instantiation_s` | The row "template instantiation". |
| `parsing_s` | The row "phase parsing". It is a phase: it contains the two rows above that happen during parsing. |
| `analyzer_s` | The sum of the analyzer rows, `null` without `-fanalyzer`. |
| `total_s` | The row "TOTAL". |

The six slowest files of `host-v96` on macOS arm64, without the analyzer, `measured` 2026-11-17, two files at once:

| File | wall s | constexpr s | peak MB | instructions |
|---|---|---|---|---|
| `fuzz/mcp_message.cpp` | 39.6 | 33.3 | 1275 | 403,500,602,381 |
| `signal_type.cpp` | 34.0 | 29.4 | 1704 | 377,100,053,043 |
| `examples/agent/mcp_host.cpp` | 31.2 | 26.1 | 1131 | 305,691,788,068 |
| `font.cpp` | 27.8 | 24.7 | 1346 | 321,496,020,197 |
| `state.cpp` | 24.6 | 17.3 | 992 | 241,889,696,075 |
| `agent_kit.cpp` | 19.2 | 13.6 | 1251 | 195,750,183,033 |

Constant evaluation is 70 to 88 percent of each file.
The Linux format of `/usr/bin/time -v` is parsed from a sample written by hand, not captured on Linux (`recalled`).

## Levers that cut the cost

| # | Lever | Expected win | Risk | Mark |
|---|---|---|---|---|
| 1 | Keep `-fanalyzer` off the ports, and off whatever else the tests of the library do not need. | About 1570 s of CPU for the two ports. | Less analyzer coverage of ports. | `measured`, done in [#93](https://github.com/filipsajdak/lvgl26/pull/93) |
| 2 | Analyzer settings: `-fanalyzer-call-summaries`, `--param analyzer-supernode-explosion-factor` below 5, `analyzer-max-enodes-per-program-point` below 8. | Unknown. | Fewer paths, so a missed bug, and `-Wanalyzer-too-complex`. | `source` [analyzer.opt](https://raw.githubusercontent.com/gcc-mirror/gcc/master/gcc/analyzer/analyzer.opt) for the defaults, `summary` for the effect |
| 3 | GCC bug 110112: big read-only static arrays took 66 s under the analyzer against under 1 s without. Our font data has that shape. A fix landed in 2023. | Possibly most of the 68 s. | Needs a test: strip the array, time again. | `summary` [thread](https://inbox.sourceware.org/gcc-bugs/bug-110112-4-TL0zfgdDFF@http.gcc.gnu.org%2Fbugzilla%2F/t) |
| 4 | A ninja job pool for the heavy files: `JOB_POOLS heavy=2` and `JOB_POOL_COMPILE`. | Keeps four 3 GB files from running together. Memory insurance, not speed. | Needs a list of the heavy files. | `source` [CMake](https://cmake.org/cmake/help/latest/prop_gbl/JOB_POOLS.html) |
| 5 | Shard the analyze job, and set `timeout-minutes`. | A lost runner costs one shard. | More CI settings. | `recalled` |
| 6 | A precompiled header for `<meta>`, `<ranges>`, `<print>`. | `<meta>` 187 ms to 92 ms in a GCC 16 benchmark. Saves parsing, not our constant evaluation. | The flags must match, or GCC ignores it silently. | `source` [Romeo](https://vittorioromeo.com/index/blog/refl_compiletime.html), [GCC](https://gcc.gnu.org/onlinedocs/gcc/Precompiled-Headers.html) |
| 7 | Drop `<print>` (about 509 ms a file) and `<ranges>` (about 85 ms) from headers and tests. | Per file, as listed. | Less readable loops. | `source` Romeo |
| 8 | Evaluate heavy data once, by a tool, and `#embed` the result. Fonts cost 1.2 s (14 px) to 4.9 s (48 px). | The data is evaluated zero times per file. | Loses "the compiler draws the font", so make it opt-in. | `measured` ([fonts.md](fonts.md)) |
| 9 | Pass big aggregates by pointer or reference in constant evaluation. | Cuts the hash and compare of the cache key. | A reference must not outlive its object. | `source` constexpr.cc, the size of the win is inferred |
| 10 | `--param ggc-min-expand` and `ggc-min-heapsize` higher. | Fewer collections: a bug report saw 5 GB of GC memory in constant evaluation. | Raises peak memory, which is what hurts. | `summary` [PR 96197](https://inbox.sourceware.org/gcc-bugs/bug-96197-4-YVbjVKmoUA@http.gcc.gnu.org%2Fbugzilla%2F/t) |
| 11 | C++20 modules. | Probably none: the benchmark found a precompiled header faster than a module for `<meta>` (92 ms against 279 ms), and our own note found modules 2.2 times slower. | Experimental in GCC 16. | `source` Romeo, `measured` |

## Work a header does where it is read

`measured` 2026-11-19 with `tools/compile_report.py`, in instructions of `cc1plus`, a file that only includes the header named, against a file that includes only that header's own `lv/` includes.

| Header | Its own cost before | After | What it was |
|---|---|---|---|
| `lv/theme.hpp` | 14.2 G (29.49 against 15.33) | 0.16 G | Five scans called with no template argument (`look_targets()`, `theme_widgets()` and the functions they call) from variable templates: GCC evaluates a call that depends on nothing where the template is read |
| `lv/scaled_font.hpp` | 0.87 G | 0.67 G | Three `find("lv_tiny_ttf_...")` in variable templates; the rest is parsing |
| `lv/encoded_image.hpp` | 0.57 G | 0.21 G | Two `find(...)` for the decoders |
| `lv/font.hpp` | 3.0 G | unchanged | Real work: constant-evaluation time does not grow, parsing and instantiation do (the rasteriser, `<memory>`, `<array>`) |
| `lv/look.hpp` | 0.6 G | unchanged | Real work, mostly parsing |
| `<lv/lv.hpp>` | 37.81 G | 23.21 G | All of the above |

`tools/check_eager.py` (ctest `eager_check`) finds the two patterns in the text of the headers; it reads text, not what GCC does.

Why no compiler counter guards it, `measured` the same day:

- `-fconstexpr-ops-limit` counts one outermost evaluation. The largest of a file that only includes `<lv/lv.hpp>` lies between 4.0 M and 5.5 M operations before and after the fix alike: it is the scan of the LVGL API (`detail/api.hpp`, 4.0 M to 5.5 M on its own, its includes under 1 M). A limit would not have caught 14 G of eager theme scans.
- `-fmem-report`: 285 M allocated before, 282 M after. Not a signal.
- Instructions are the signal, but CI's Linux image has neither `perf` nor a `time` that counts them.

An open optimisation, not built: `detail/api.hpp` builds `api` (every `lv_*` function) at namespace scope, 9.0 G in every file (13.18 G with the header, 4.18 G with its includes). Every file that names a widget needs the scan, so only files without widgets would gain. Making it lazy turns `find`, `names_after`, `resolve*` and the loops over `api` in `props.hpp` and `style.hpp` into templates on D, about 30 call sites. With it lazy, the largest evaluation of an include-only file would fall under 1 M operations, and an operation limit on such a file would become a real guard. `tools/eager_allowed.json` lists `api` with this reason.

## What to measure first

1. The analyzer on `font_bytes.cpp` with `-fanalyzer-call-summaries`, with `analyzer-supernode-explosion-factor=1`, and with the font array removed. The last one tests bug 110112.
2. `-ftime-report` and peak memory of the 20 slowest files, to build the list for lever 4.
3. A precompiled header for the three standard headers, in instructions.
4. The duration of the analyze job after #93.

## Open questions

- Is the time in the analyzer proportional to data size or to the number of functions? No source says.
- Does GCC 16.2 contain the fix for bug 110112 and the faster program-state simulation that the 16 notes list? Check the commit dates.
- Does `-fno-exceptions` save analyzer time? GCC 16 adds `-fanalyzer-assume-nothrow`, so it may.
- How do the Linux kernel, curl and systemd run the analyzer in CI? `recalled` only: nobody gates every file.
- How much per-file noise does `-ftime-report` have on a loaded runner? One sample read 1.86 s against 0.3 s.
