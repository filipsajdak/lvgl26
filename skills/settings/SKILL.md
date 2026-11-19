---
name: settings
description: Use when a developer sets up an lvgl26 program, ports it to a new target, or asks what lvgl26 lets them switch off or forbid (heap allocation, threads, files, floating point), how strict it is about plain values, or how large a picture the compiler may decode. Interviews them about the program and its rules, proposes a whole settings file (lvgl26.json) and the CMake line that names it, and adjusts it with them.
version: 0.1.0
---

# Setting up an lvgl26 program

lvgl26 reads one settings file per program, `lvgl26.json`, named in CMake with `lvgl26_settings(app lvgl26.json)`.
The keys, their defaults and what each means are in [docs/reference/settings.json](../../docs/reference/settings.json), written by the library itself.
Read that file first; this skill holds no copy of it.
The page for people is [docs/settings.md](../../docs/settings.md).

## The conversation

Do not walk the developer through the keys one by one.
Ask about the program, propose a whole file, and let them change it.

1. Ask what the program runs on: a desktop or a board with an operating system, a microcontroller without one, or a browser.
   For a microcontroller, ask for the core and how much RAM and flash it has.
2. Ask whether the project has rules that forbid something: a coding standard such as MISRA, no heap after start, no threads, no floating point.
   One standard often answers several questions at once.
3. Ask what the program does beyond its screen: does it write files or screenshots, run work on other threads, load screens from documents at run time, or talk to an agent?
4. Propose the whole file, starting from the preset that fits, with one reason per setting:
   [desktop](../../examples/settings/presets/desktop.json), [microcontroller](../../examples/settings/presets/microcontroller.json), [browser](../../examples/settings/presets/browser.json).
5. Let the developer adjust it, then write `lvgl26.json` beside their `CMakeLists.txt` and add the `lvgl26_settings` line after the program's `target_link_libraries`.
6. Build. A part of lvgl26 that the file forbids is a compile error that names the key, for example `allow.threads`; show it, and ask whether to allow the feature or to drop the part.

## What each answer means

- No heap after start: `allow.allocation` false. Tasks (`lv::spawn`) are then an error, until tasks get a fixed region of memory; say so.
- No threads, or a browser: `allow.threads` false. `lv::ui_scheduler` is then an error.
- No file system, or a rule against standard streams: `allow.files` false. `lv::write_png` and the MCP server's `serve` are then errors.
- No floating point at run time, or no FPU: `allow.floating_point` false. `lv::runtime` is then an error; fonts, pictures and tokens are still made by the compiler.
- Plain values in styles while a design system is not ready: `rules.raw_style_values` true. Suggest design tokens instead.
- Plain integers in states: `rules.raw_integer_state` true. Suggest a quantity, `lv::counter` or a named type instead.
- Token files still in the forms from before the Design Tokens Format 2025.10 (`"#3366ff"`, `"8px"`): `rules.tokens_legacy_values` true, only while `tools/dtcg_fix.py` converts them.
- Pictures larger than 128 x 128 decoded by the compiler: raise `limits.compile_time_image`, a value and a unit such as `{"value": 76800, "unit": "px"}`. Each picture costs compile time.

## What not to do

- Never also specialize `lv::cfg` in C++ in a project that has the file: that is a compile error.
- Never invent a key: the reference lists every key, and the compiler refuses any other.
- Never edit the copy of the file in the build directory: CMake writes it from the project's file.
