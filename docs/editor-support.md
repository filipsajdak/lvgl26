# Editor support

lvgl26 is C++26 with reflection, and only GCC 16 compiles it. clangd, the language server behind most editors, cannot parse it. One CMake line gives clangd stub headers to read instead: the same public API, written as plain C++.

With them your editor completes the members of `lv::arc({.`, shows what each one takes, and marks a wrong name in quotes while you type.

## Turn it on

Add one line to your `CMakeLists.txt`, after your targets:

```cmake
FetchContent_MakeAvailable(lvgl26)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE lvgl26::lvgl26)
lvgl26_editor_support()
```

Then run CMake once, and restart clangd in your editor. You need Python 3 for CMake, and clangd for the editor. Version 23 is the one that is tested; the stubs need 19 or newer.

The line does three things when CMake runs:

- It writes the stub headers into `lvgl26-editor` in your build directory.
- It turns on `compile_commands.json`, which tells clangd how each file is compiled.
- It writes a file `.clangd` into your source directory, which tells clangd to read the stubs.

The `.clangd` holds paths of your machine. Add it to `.gitignore`.

GCC still builds your program from the real headers. The stubs are for the editor only, and nothing of them reaches the program.

## What works

| In the editor | Example |
|---|---|
| Completion of members | Type `lv::arc({.` and choose from `angles`, `range`, `value` and the rest |
| Documentation on hover | `.value` says what it takes, and that it sets `lv_arc_set_value` |
| A mark on a wrong name in quotes | `.align = "centre"` is marked at the name |
| A mark on an unknown member | `lv::label({.range = ...})`: a label has no range |
| A mark on a wrong value | `.width = 120` needs a unit |
| A mark on an unknown widget or id | `lv::widget<"spinbocks">`, `root->get<"speedometer">()` |
| Typed views | `root->get<"speedo">()` is an `lv::view<"arc">`, so `set({.` completes for an arc |
| Design tokens | `tok.color.` lists the tokens of your file, each with its value |
| Go to definition | It leads into the stub, which has the documentation, not into lvgl26's code |

Measured once on one machine, with clangd 23 and the quick start. Opening the file takes about two seconds until its first check. A completion list takes a tenth of a second, and a hover a few milliseconds.

## What does not work yet

- **Completion inside quotes.** clangd offers nothing inside `"cen`. The hover on the member lists the names.
- **lvgl26's own error text.** The editor's message for a wrong name is Clang's: "call to consteval function ... is not a constant expression". GCC prints the full message, with the list of names, when you build.
- **Checks that need the values.** The editor does not see a missing arm of `lv::match`, an id that is used twice, or a number that does not fit. GCC does.
- **The setting `raw_style_values`.** The editor marks a plain value in a style even when your project allows it.
- **CLion, Visual Studio and cpptools.** They do not use clangd by default. CLion 2026.2 says it reads reflection with GCC 16; that has not been tried with lvgl26.
- **Zed, Helix, Neovim and VS Code with the clangd extension** use clangd, so they read the `.clangd`. Only clangd itself is tested, not each editor.

An editor that shows no mark is no proof that the code compiles. The build is.

## Design token files

The members of `lv::tokens<...>` come from your token file, so the stubs have a struct for each file. CMake finds the files itself: every `.json` file under your source directory that is a design token file. It skips build directories and hidden directories.

A token file outside the source directory needs its name:

```cmake
lvgl26_editor_support(TOKENS ${CMAKE_SOURCE_DIR}/../design/tokens.json)
```

When you change a token file, the next build runs CMake again, and the editor sees the new tokens. A new token file needs one run of CMake by hand.

## If you have a `.clangd` already

CMake never changes a `.clangd` that exists. It writes what lvgl26 needs into `lvgl26-editor/clangd.example` in your build directory, and says so. Copy the `CompileFlags` part into your file.

## Check that it works

Ask clangd to read a file the way your editor does:

```sh
clangd --check=main.cpp
```

The last line must say `All checks completed, 0 errors`. Then write `.align = "centre"` somewhere and run it again: clangd must name that line.

If every `lv::` name is marked as an error, clangd reads the real headers. Check that `.clangd` is in the directory you opened, that it names your build directory, and that CMake has run.

## How it works

- A reference file holds what reflection finds in LVGL: the widgets, their properties, the names of each enum. `tools/editor/generate.py` writes the stub structs from it. There is one file for each LVGL version that lvgl26 builds with.
- The rest of the API is written by hand, in `tools/editor/stubs`. Tests keep it in step with the real headers.
- The `.clangd` gives clangd a file overlay. It leads `<lv/...>` to the stubs, and each token file to a small marker that names its struct.
- The `.clangd` also removes the options that only GCC knows, such as `-freflection`.
