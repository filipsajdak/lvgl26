# Project settings

A program names one settings file, `lvgl26.json`, and every file of the program reads it. It says what the program may use, how strict lvgl26 is, and what the compiler may do. A part of lvgl26 that the file forbids is a compile error that names the key.

## The file

The file has three sections. `allow` says what the program may use: everything is allowed by default. `rules` relaxes the library's own strictness: everything is strict by default. `limits` bounds what the compiler does.

This is the preset for a microcontroller:

<!-- snippet: examples/settings/presets/microcontroller.json -->
```json
{
  "$schema": "https://raw.githubusercontent.com/filipsajdak/lvgl26/main/docs/reference/lvgl26-settings.schema.json",
  "allow": {"allocation": false, "threads": false, "files": false, "floating_point": false},
  "limits": {"compile_time_image": {"value": 4096, "unit": "px"}}
}
```

The order of the keys is free. `$schema` lets an editor complete the keys and mark a misspelled one: the schema is [docs/reference/lvgl26-settings.schema.json](https://github.com/filipsajdak/lvgl26/blob/main/docs/reference/lvgl26-settings.schema.json). Every key, its default and what it means is in [docs/reference/settings.json](https://github.com/filipsajdak/lvgl26/blob/main/docs/reference/settings.json). Both files are written by the library from its own settings struct, so they cannot disagree with what compiles.

The CMake file of a program names the file once:

<!-- snippet: examples/settings/CMakeLists.txt -->
```cmake
cmake_minimum_required(VERSION 3.30)
project(settings LANGUAGES C CXX)

include(FetchContent)
FetchContent_Declare(lvgl26 GIT_REPOSITORY https://github.com/filipsajdak/lvgl26 GIT_TAG main)
FetchContent_MakeAvailable(lvgl26)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE lvgl26::lvgl26)
lvgl26_settings(app presets/microcontroller.json)  # one file for the whole program
```

The program itself does not change. This one builds with each of the three presets in [examples/settings/presets](../examples/settings/presets), because it uses nothing that they forbid:

<!-- snippet: examples/settings/main.cpp -->
```cpp
// A program that each settings file in presets/ builds: it uses none of what they forbid.
#include <lv/lv.hpp>

constinit lv::state taps{lv::counter{}};
void tapped() { ++taps; }

constexpr auto ui = lv::column(
  lv::label({.text = lv::bind(taps, "{} taps")}),
  lv::button({.on_clicked = &tapped}, lv::label({.text = "Tap"}))
);

constinit lv::frame<320, 240> frame;  // the display's memory

int main() {
  lv::application app;
  lv::display display{app, frame};
  auto root = lv::mount<ui>(display.screen());
  if (!root) return 1;
  ++taps;
  return taps.get().value == 1 ? 0 : 2;
}
```

## What a restriction forbids

| Key | What needs it in lvgl26 |
|---|---|
| `allow.allocation` | `lv::spawn` (a task's frame), `lv::ui_scheduler::post`, `lv::encode_png` and `lv::write_png`, `lv::mcp::server` |
| `allow.threads` | `lv::ui_scheduler` |
| `allow.files` | `lv::write_png`, `lv::write_new_png`, `lv::mcp::server::serve` |
| `allow.floating_point` | `lv::runtime`, which reads the numbers of a document as `double` |

A restriction changes no code: a test compiles one file with none of them and with all four, and compares the objects.

The message names what needs the feature and the key that forbids it:

```text
error: static assertion failed: lv: lv::ui_scheduler needs threads, and this project does not
allow them (allow.threads in lvgl26.json).
```

"Allocation" means the system heap: `new`, `malloc`, a standard container that grows. LVGL's own fixed pool is not the system heap, so widgets, styles and bindings still work. A task's frame comes from that pool, but in a number the compiler does not know. So a program without allocation runs no task, until tasks get a fixed region of memory of their own.

A compile error covers lvgl26's own functions. It cannot see the program's own `malloc`, `std::thread` or `fopen`: the compiler has no hook into the program's code. `allow.files` does not cover LVGL's own file drivers yet; they join the file with LVGL's options.

## Limits with units

A limit is a value and a unit, as in design token files: `{"value": 4096, "unit": "px"}`. A unit that is none of the limit's is a compile error that lists its units. A number of bytes takes `B`, `KiB` or `MiB`, and a fraction only if it gives whole bytes: half a KiB is 512 B.

## Mistakes in the file

A misspelled key, a wrong type and a wrong unit stop the build with the file's name and what is valid:

```text
error: static assertion failed: lv: lvgl26.json: unknown key 'thread' in allow. Did you mean
'threads'? The keys of allow: allocation, threads, files, floating_point.
```

## One file for the whole program

`lvgl26_settings` compiles every C++ file of the program, and of each library of the project that the program links and that uses lvgl26, with the settings file. So no file can read other settings. A library that is part of two programs with different files is an error when CMake runs.

A library built elsewhere with another file is found before the link: each object names its settings file by a hash, and a step before the link fails for a hash that is not the program's. Where that step does not run, the hash stays an undefined symbol, and the link fails.

## Without CMake, or without a file

A build without CMake, or a CMake build that names no file, gets the defaults. It changes a setting with a specialization of `lv::cfg`, in a header that every file includes right after `<lv/lv.hpp>`:

```cpp
template <>
inline constexpr lv::config lv::cfg<lv::override>{.rules = {.raw_style_values = true}};
```

A project uses the file or this specialization, not both. With a file, the header that CMake writes defines the specialization itself, so a second one is a compile error that names the file.

## Setting it up with an agent

The plugin of this repository has a skill, [skills/settings](../skills/settings/SKILL.md), in which an agent asks what the program runs on and what the project forbids. It proposes a whole file from the presets, and the developer adjusts it.
