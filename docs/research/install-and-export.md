# Install and export: how a project takes lvgl26

Status: the options for the owner, before the release starter. Nothing is built. Marks: `source` is read in this repository, `reasoned` is mine.

## Today

- lvgl26 has no `install()` and no `export()`; `project(lvgl26 VERSION 0.0.1)`. A project takes it with `add_subdirectory` or `FetchContent` and links `lvgl26::lvgl26`, an INTERFACE target (`CMakeLists.txt`). `source`
- It fetches LVGL itself, `FetchContent_Declare(lvgl ...)` at the tag in `LVGL26_LVGL_REF` (v9.6.0, with a SHA-256), and builds it with the settings baked in as PUBLIC compile definitions of `lvgl::lvgl`: `LV_CONF_SKIP`, the heap size, `LV_USE_TINY_TTF`, the PNG and JPEG decoders, the theme switches (`cmake/lvgl.cmake`). There is no `lv_conf.h`. `source`
- The headers read LVGL's headers as text with `#embed`, so `lvgl26` passes `--embed-dir=` for each include directory of `lvgl::lvgl`. A consumer needs LVGL's headers on disk, at compile time, of the same version that was built. `source`
- The options: `LVGL26_PNG`, `LVGL26_JPEG`, `LVGL26_TTF`, `LVGL26_NO_DEFAULT_THEME`, `LVGL26_UNITS`, `LVGL26_SIGNAL_TYPE` (proposed in #232), `LVGL26_CHECK_INCLUDES`, `LVGL26_CHECK_FONTS`, `LVGL26_LVGL_REF`, `LVGL26_LVGL_HEAP_BYTES`. The tests turn several on. `source`
- Functions that a project calls come from `cmake/*.cmake`, included by the top `CMakeLists.txt`: `lvgl26_settings()`, `lvgl26_editor_support()`, the include and font checks (which also read `tools/*.py` and `docs/reference/headers.json`). `source`
- `lvgl26::signal_type` (#232) is a STATIC library with eight files; it links `lvgl26::lvgl26` PUBLIC and needs the font files of `include/lv/signal/fonts` only while it compiles.

## What an installed package must carry

1. The headers of `include/lv`, with `include/lv/signal/*` (JSON, fonts) next to them, since headers embed what lies beside them.
2. LVGL: its headers (for `#embed` and `#include`), its library, and **the compile definitions it was built with**. The definitions must reach the consumer unchanged: a consumer that compiles against LVGL headers with other `LV_USE_*` values than the library has is wrong silently. `reasoned`
3. The CMake modules and their Python tools (`tools/check_includes.py`, `check_fonts.py`, the generated tables), or the functions drop out of an installed package.
4. mp-units 2.5.0 when `LVGL26_UNITS` is on (header-only).
5. `lvgl26Config.cmake` with `find_dependency` for what is not carried, `lvgl26ConfigVersion.cmake`, and the imported targets `lvgl26::lvgl26`, `lvgl26::signal_type`.

## Three ways to take it, and what each costs

| | `FetchContent` / `add_subdirectory` (today) | `find_package(lvgl26)` after an install | A starter template (the release) |
|---|---|---|---|
| One line for the user | yes | yes, after a build and install | yes: the template does it |
| LVGL config reaches the user | by construction: one build | must be exported and versioned | by construction |
| A second build of lvgl26 per project | yes, all of LVGL | no | yes |
| Works offline | no, unless a mirror | yes | needs the fetch once |
| Work for us | none | install rules for LVGL and for the modules, a test that builds a consumer | a repository, a Docker image, CI |

## Open choices for the owner

1. **Which of the three does v0.1.0 promise?** My recommendation: FetchContent and the starter only; say in the README that `find_package` is not offered yet. The release then needs no install rules.
2. **If `find_package` is wanted:** export LVGL with lvgl26 (set `LV_BUILD_INSTALL ON` and install LVGL's headers and library under lvgl26's prefix), or require the user to bring a matching LVGL and check its definitions at configure time? The first is safer; the second is smaller. Either needs a test that configures and builds a consumer against an installed tree, as `include_check_build` does for the includes.
3. **Names.** `lvgl26::lvgl26` and `lvgl26::signal_type` are the two public targets. Keep the option names as they are (`LVGL26_*`) and write them in one table in the docs, with the default of each.
4. **Version.** The CMake version moves from 0.0.1 to 0.1.0 with the tag (release plan). `find_package` would add a compatibility rule (`SameMinorVersion` while 0.x).
5. **The LVGL pin.** v9.6.0 is pinned by hash and v10 is a second pin that CI builds. Say which the release supports: both, or v9.6.0 only with v10 as a preview.
