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
- No handwritten per-property code. Properties, styles and events come from reflection.
- Anything a header does at namespace scope costs compile time in every file. Keep reflection
  scans lazy: put them behind a template parameter (see `props_of` in `include/lv/props.hpp`).
- Measure before claiming: compile time with `/usr/bin/time`, heap with `lv_mem_monitor`,
  pixels with `lv_test_screenshot_compare`.

## Known tool limits

- clang-tidy cannot analyze this code: Clang 23 has no reflection and no `<meta>` header.
  GCC's warnings, analyzer and sanitizers stand in for it.
- Reference screenshots are only written with `-DLVGL26_UPDATE_REFERENCES=ON`.

## Git

Feature branches, atomic conventional commits, ASCII only in commit messages.
