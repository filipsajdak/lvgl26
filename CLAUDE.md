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

Every change must pass `host-v96` and `host-v10` locally, and all presets in CI.

## Rules

- Both LVGL versions must work. Use only LVGL's public `include/lvgl` API.
- Mistakes should be compile errors with a message that says how to fix them. Add a
  compile-fail test in `tests/fail/` for every new one.
- The library throws nothing: compile error first, then contracts, then `std::expected`.
- Views never delete widgets. Only `lv::mount` and `lv::owned` own.
- Styles take design tokens only. Add the token to the token file; use `lv::raw(...)` only for a
  reasoned exception. Use semantic or component tokens in UI code, not primitives.
- No macros and no `#if` in our code. Settings go in `lv::config` (see `include/lv/config.hpp`);
  optional LVGL features are detected with `requires` or reflection. `#embed`, `#include` and
  `#pragma once` are the only preprocessor lines we write.
- Application values are strongly typed: a measure is an mp-units quantity (`120 * km / h`), a
  count is `lv::counter`, anything else a one-member struct. Do not add a unit system to lvgl26.
- No handwritten per-property code. Properties, styles and events come from reflection.
- Anything a header does at namespace scope costs compile time in every file. Keep reflection
  scans lazy: put them behind a template parameter (see `props_of` in `include/lv/props.hpp`).
- Measure before claiming: compile time with `/usr/bin/time`, heap with `lv_mem_monitor`,
  pixels with `lv_test_screenshot_compare`.

## Test adequacy

| Check | How | Limit |
|---|---|---|
| Complexity | `lizard`, through `tools/crap.py` | 8 for every function, also compile-time ones |
| CRAP | `tools/crap.py build/host-v96-coverage` after the coverage preset | 8 for every function that runs at run time |
| Mutation | `tools/mutation.py <header> --tests '<ctest -R>' --since origin/main` | Every survivor gets a test, or a reason |
| Fuzzing | `fuzz/json_reader.cpp` (libFuzzer, Clang); `fuzz/runtime_document.cpp` (AFL++, GCC) | Every free input has a target; every crash becomes a corpus file |

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

## Known tool limits

- clang-tidy cannot analyze this code: Clang 23 has no reflection and no `<meta>` header.
  GCC's warnings, analyzer and sanitizers stand in for it.
- Reference screenshots are only written with `-DLVGL26_UPDATE_REFERENCES=ON`.

## Git

Feature branches, atomic conventional commits, ASCII only in commit messages.
