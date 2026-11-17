# The constant-evaluation limit

What GCC's `-fconstexpr-ops-limit` costs us, what a very high value risks, and how to need none.
Measured on 2026-11-17 with GCC 16.2.0 on macOS arm64, on `main` at commit `f51e8cc`. Marks follow [README.md](README.md).

## Short answers

| Question | Answer |
|---|---|
| Does a large tree need a high limit? | No. A tree of about 2,700 widgets needs 5.5 to 6 million operations, the same as one label. |
| What do ordinary files need? | Between 4 and 10 million operations. GCC's default is 33.5 million. |
| What needs a high limit? | Fonts: about 100 million. Signal's typefaces: 64 to 88 million. Nothing else measured. |
| Why not set a high limit for everyone? | A runaway evaluation then runs for minutes and fills memory. At 1 billion it took 9 minutes and 2.8 GB. |
| How can fonts avoid a high limit? | Evaluate each glyph on its own. The limit counts one constant evaluation, not the whole file. |

## What the limit counts

GCC stops a constant evaluation after `-fconstexpr-ops-limit` operations. The default is 33,554,432.
The count restarts for every constant evaluation: each `constexpr` variable, and each `consteval` call that is not part of another.
This was measured with a synthetic job of 15 million loop rounds.

| Form of the job | Result at the default limit | Mark |
|---|---|---|
| One evaluation of 15 million rounds | Stops after 4.3 s: "operation count exceeds limit of 33554432" | `measured` |
| Ten evaluations of 1.5 million rounds, in ten `constexpr` variables | Compiles, in 16.5 s and 92 MB | `measured` |

The budget a file needs is therefore the cost of its largest single evaluation, not the sum of its work.

## What lvgl26 files need

The smallest limit at which a file compiles, found by bisection with `-fsyntax-only`. Each result is a range: the file fails at the first number and compiles at the second.

| File | Needs (operations) | Mark |
|---|---|---|
| `#include <lv/lv.hpp>` and nothing else | 4.5 to 4.75 million | `measured` |
| One label | 5.75 to 6.0 million | `measured` |
| A screen of 288 widgets, as one expression | 5.75 to 6.0 million | `measured` |
| The same, as eight `constexpr` variables | 5.75 to 6.0 million | `measured` |
| A screen of about 2,700 widgets, one expression | 5.5 to 6.0 million | `measured` |
| The same, as thirty `constexpr` variables | 5.5 to 6.0 million | `measured` |
| `tests/large_tree.cpp`, `theme.cpp`, `tokens.cpp` | 4 to 5.5 million | `measured` |
| `tests/runtime_ui.cpp` | 5.5 to 7 million | `measured` |
| `tests/image.cpp` | 7 to 8.5 million | `measured` |
| The `bounce` scene of the window demo | 5 to 7 million | `measured` |
| The printer panel at stage 2, with the current image API | 5 to 10 million | `measured` |
| `tests/signal_type.cpp` | 64 to 88 million | `measured` |
| `tests/font.cpp` | 88 to 112 million | `measured` |

- The floor of 4.5 million is the library: reflection scans that run once for each file that includes it.
- The size of a tree does not matter at these sizes. The earlier note that suspected copies of property structs describes code that has since changed.
- The CMake files of `bounce` and the printer set 20 billion. They need one two-thousandth of that.
- The library sets 1 billion for every user (`CMakeLists.txt`). Only fonts need more than 10 million.

## What a high limit risks

A constant evaluation that never ends, for example a loop with a mistake, runs until it reaches the limit.

| Limit | Time to stop | Peak memory | Mark |
|---|---|---|---|
| 33.5 million (GCC's default) | 4.7 s | 137 MB | `measured` |
| 100 million | 15.7 s | 288 MB | `measured` |
| 1 billion (what lvgl26 sets) | 545 s, of which 63 s was system time | 2.8 GB | `measured` |

- Memory grew with the number of operations, at about 2.8 bytes for each, in this loop.
- At the same rate, 20 billion operations would need about 56 GB. That is an extrapolation, not a measurement, and it exceeds most machines.
- The limit is an interface option of `lvgl26`, so every user of the library pays this risk for a need that only fonts have.
- A high limit also hides a regression. A tree that becomes a hundred times more expensive still compiles.

## How to need less

1. **Stop setting the limit for everyone.** GCC's default covers every file measured except fonts, with at least three times the headroom.
2. **Give fonts their own budget.** A font is made in one evaluation, which needs about 100 million operations at 48 px. A target that makes fonts asks for it, and no other target does.
3. **Evaluate a font glyph by glyph.** With one `constexpr` variable for each glyph, each evaluation needs about 1 million operations, and the default limit is enough for any size. The experiment above shows that separate evaluations keep separate budgets. This is not built yet, so the cost of the 95 template instantiations is not measured.
4. **Fix a budget for each example and test, at twice its need.** `tests/large_tree.cpp` does this with 9 million. A regression then stops the build, where today a limit of 20 billion hides it.
5. **Say what the error means.** The compiler's message names the option, not the cause. The docs should say: a tree needs no raised limit, and a font does.

## Not measured

- The cost of splitting a font by glyph, and the size of the final copy that joins the pieces.
- Fonts larger than 48 px as a function of size. Two earlier notes have 14, 24 and 48 px.
- Pictures larger than the 128 x 128 default. Decoding grows with the area, and the cap exists for that reason.
- Other GCC versions and Linux. The counts are the compiler's own, so they should match, but the times will not.
- Clang does not compile the code, so its limit is not relevant.

## How this was measured

For each file the script took the compile command from the build, removed the limit and the outputs, and added `-fsyntax-only` and a new `-fconstexpr-ops-limit`. It multiplied the limit by four until the file compiled, then halved the range three times.
The generated screens have rows of a label, an arc and a button with a label inside, in columns of 12 or 30 rows.
The runaway job is two nested loops of 200,000 rounds each.
