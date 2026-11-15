# C++26: language, compile time, coroutines

Marks follow [README.md](README.md).

## Pattern matching

Language-level pattern matching is not in C++26, and neither GCC 16 nor upstream Clang implements it. `source` 2026-11-13 (a research report that read the papers).

| Paper | State |
|---|---|
| P2688R6, pattern matching with `match` and `case`, 2026-09-22 | The live proposal, aimed at C++29. Its last poll, in February 2025, did not reach consensus for C++26. Wording is not written. |
| P2392R3, `is` and `as`, 2024-10-15 | No later revision. Lives on in cppfront. |
| P1371R3, `inspect`, 2020 | Superseded by P2688. |

What P2688R6 looks like:

```cpp
return match (coin) {
  case Coin::Penny   => 1;
  case Coin::Quarter => 25;
};
```

- A non-exhaustive or redundant case is a hard error.
- The arms need one result type, or an explicit `-> T`.
- R6 dropped `let` patterns and the `<Type>` form. Do not mirror older revisions or `mpark/patterns`.

What it means for us: `lv::match` mirrors `case pattern => arm` with `.then(handler)`. Library emulations cannot check exhaustiveness. Reflection over the enumerators can, so a missing case is a compile error.

## Compile-time cost of reflection

Benchmarks by Vittorio Romeo on GCC 16. `summary` 2026-11-14, [article](https://isocpp.org/blog/2026/03/the-hidden-compile-time-cost-of-cpp26-reflection-vittorio-romeo).

- Most of the cost is including `<meta>`, not the reflection algorithm.
- Enum to string: 180.8 ms per file with a plain include, 73.8 ms with a precompiled header.
- The algorithm costs about 0.07 ms per enumerator.
- Modules were about 2.2 times slower than the plain include in GCC 16.
- Nothing was found on caching `consteval` results between files. "Paid once" means once per source file.

Our own numbers. `measured` 2026-11-14, best of 3 with `/usr/bin/time`, includes alone 0.37 to 0.40 s.

| Way of reading LVGL's doc comments at compile time | Added time |
|---|---|
| `#embed` of the headers, even the 140 KB one | none measurable |
| One search per function asked about | about 0.4 s each |
| One pass with `string_view::find` into a small table | about 0.19 s |
| One pass with a `memchr` loop on a rare character, then compare | about 0.06 s |

Rule that follows: one pass per header into one table, behind a template parameter. A file that uses none of it pays nothing.

Open problem. `measured` 2026-11-15: the printer demo screen needs `-fconstexpr-ops-limit=20000000000` and takes about 176 s to compile. The cause is not measured. Suspected: every node carries a full property struct that is copied as the tree is assembled.

## Reading semantics from doc comments

| Precedent | What it does | Lesson |
|---|---|---|
| [GObject Introspection](https://gi.readthedocs.io/en/latest/annotations/index.html) | Reads annotations in C doc comments to generate bindings: ownership, nullability | An API it cannot classify is marked not introspectable, never guessed |
| [Clang API notes](https://clang.llvm.org/docs/APINotes.html) | A YAML file beside a library adds semantics without editing its headers | The standard fallback when comments are not reliable |
| LVGL `gen_json.py` | Exports docstrings for binding generators | Confirms that the docstrings are the only place some units live |

All `summary` 2026-11-14.

What it means for us: when an angle setter sits in a header we did not scan, the build fails. It never guesses the unit.

## The boost-ext/ut configuration pattern

How a header-only library takes configuration without macros. `source` 2026-11-13, from the raw README and `ut.hpp`, both partly truncated.

- The library declares `template <class = override, class...> inline auto cfg = runner<reporter_junit<printer>>{};`.
- A user replaces it at namespace scope: `template<> auto ut::cfg<ut::override> = cfg::runner{};`.
- The README states no placement rule. Every example puts the specialization before `main()`.
- Its macros are opt-in. Version checked: 2.3.1.

What it means for us: settings live in `lv::config` and need no macro.

## Coroutines on microcontrollers

Coroutines are a compiler feature. They need no operating system, no threads and no exceptions.

| Need | State | Mark |
|---|---|---|
| Compiler support | GCC has had it since version 10; we require 16 | `recalled` |
| The `<coroutine>` header | Part of the freestanding library since C++23 | `recalled` |
| Memory per task | A frame from LVGL's allocator, no stack per task | `measured` |
| No exceptions | The library throws nothing and builds with `-fno-exceptions` | `measured` |

Where it should run. `recalled` unless marked.

| Platform | Outlook | Why |
|---|---|---|
| ARM Cortex-M | Works | Runs in QEMU on `mps2-an386`. `measured` 2026-11-15 |
| RISC-V, including ESP32-C3 and C6 | Good | An upstream GCC target; needs a GCC 16 toolchain |
| ESP32 with Xtensa cores | Uncertain | The vendor's GCC fork lags upstream |
| 8-bit AVR, MSP430 | Not a target | LVGL itself is impractical there |
| IAR, Arm Compiler, Keil | No | They lack C++26 reflection. This is the real limit |

Measured on Cortex-M4. `measured` 2026-11-15 with `tools/arm-run.sh`.

- The slice: 267 KB of code and 73 KB of RAM. The screen costs 1,052 bytes and one task 128 bytes.
- A task waiting for a click was 840 bytes, because `button.next()` was 624 bytes. That was fixed.
- An animation task is 1,104 bytes, because the argument struct of `lv::animate` lives in the frame. Not fixed yet.
- The default contract handler pulled in 12 KB of exception run time. The example supplies its own handler.

If a platform cannot use coroutines:

1. Leave tasks out. The tree, properties, tokens and state do not depend on them.
2. Take frames from a fixed pool sized at compile time.
3. Do not emulate. Protothreads need macros, and stackful coroutines need assembly and a stack per task.
