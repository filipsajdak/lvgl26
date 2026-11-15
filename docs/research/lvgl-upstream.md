# LVGL upstream and prior art

Most of this file comes from one research report of 2026-11-13. It checked the `lvgl/lvgl` repository, its docs, the release notes and the forum. Marks follow [README.md](README.md).

## Releases

| Claim | Mark |
|---|---|
| The latest stable release is [v9.6.0](https://github.com/lvgl/lvgl/releases/tag/v9.6.0) of 2026-09-16. Its changelog calls it the last release of the v9 series. | `source` 2026-11-13 |
| `master` is 10.0.0-dev. The v10.0.0 milestone has no due date. | `source` 2026-11-13 |
| v9.6 moved the public API into `include/lvgl/`. `src/` is private. Kconfig is the single source of configuration. | `source` 2026-11-13 |
| Everything marked `LV_DEPRECATED` is to be removed in v10. | `source` 2026-11-13 |
| Already removed on master: the `list`, `win` and `menu` widgets, the fragment module, the generic flag API, `find_by_id`, `lv_subject_init_*`, `LV_EVENT_DRAW_TASK_ADDED`, `LV_CONF_MINIMAL`. | `source` 2026-11-13 |
| Open proposals: a new rendering engine (#10959) and pluggable text shaping (#10960). | `source` 2026-11-13 |

The migration guide is [migration-v10.mdx](https://github.com/lvgl/lvgl/blob/master/docs/src/changelog/migration-v10.mdx).

What it means for us: use only `include/lvgl`, and test every change on v9.6.0 and on the v10 pin.

## Machine-readable API

- `scripts/gen_json/gen_json.py` exports the API as JSON. It uses pycparser and Doxygen. Every item has a `docstring`. Docs: [api_json](https://github.com/lvgl/lvgl/blob/master/docs/src/integration/bindings/api_json.mdx). `source` 2026-11-13.
- The output has no schema version and depends on `lv_conf`. Whether it is right for the `include/` layout is not confirmed. `recalled`.
- Users report that the JSON lacks information and that the script failed on Linux. `summary` 2026-11-14, [forum](https://forum.lvgl.io/t/define-special-types-for-interfaces/13769).
- The MicroPython generator parses the headers itself and reads the JSON only for forward-declared structs. `source` 2026-11-13.
- Header parameters carry `@nullable` tags (#10529). `source` 2026-11-13.
- `LV_DISABLE_API_MAPPING` drops the compatibility aliases in `include/lvgl/api_map/`. `source` 2026-11-13.
- `lvgl/lvgl_pro` has `lvgl_widgets_xml/v9.6.0/*.xml` with widget properties, enums and parts. That repository has no licence, so do not reuse it. `source` 2026-11-13.

What it means for us: the doc comments are the only place some semantics live, such as the unit of an angle. We read them with `#embed` at compile time. That needs no Python, pycparser or Doxygen at build time.

## Introspection

- The body of `lv_obj_class_t` is private. Subclassing a widget class from C++ needs the private API, which has no stability promise. `source` 2026-11-13.
- The property API is off by default (`LV_USE_OBJ_PROPERTY`, `LV_USE_OBJ_PROPERTY_NAME`). It gives `lv_obj_set_property`, `lv_obj_get_property` and lookups of an id by name. Ids are 32 bits, with the value type in bits 31 to 28. `source` 2026-11-13.
- `LV_USE_OBJ_NAME` adds `lv_obj_set_name` and `lv_obj_find_by_name`. `source` 2026-11-13.

## Observer

- On master a subject comes from `lv_subject_create(type)` and lives on the heap. v9.6 still has `lv_subject_init_*`. `source` 2026-11-13.
- Types: INT, FLOAT, POINTER, COLOR, STRING, GROUP, NONE.
- An observer callback is `void(lv_observer_t*, lv_subject_t*)`. It fires once on subscription.
- There is no batching or transaction API.

## XML and LVGL Pro

- `lv_xml` left the repository in v9.5 (PR #9565). Forum staff said in February 2026 that it is closed source and that loading XML at run time needs a commercial agreement. `source` 2026-11-13, [forum](https://forum.lvgl.io/t/xml-engine-further-plan/23587).
- The Pro editor and CLI export plain C. The README of `lvgl_pro` lists the Community and Evaluation tiers as free and the Product tier at 20,000 USD per product for five years. `source` 2026-11-13.
- An importer of LVGL XML must therefore be clean-room.

## Styles

- A const style has `prop_cnt == 255`. `LV_STYLE_CONST_INIT(name, arr)` builds one that can live in ROM. `source` 2026-11-13.
- A style built with `lv_style_set_*` allocates at run time.

## Assets

- The image header is 12 bytes: magic `0x19`, then colour format, flags, width, height and stride. `source` 2026-11-13.
- The RAW colour formats hand an encoded PNG or JPEG to the decoders.
- `LV_USE_FS_MEMFS` with `lv_fs_make_path_from_buffer` opens a blob in memory as a file. That suits `#embed`.
- Fonts: `lv_binfont_create_from_buffer`, `lv_tiny_ttf_create_data(data, size, px)` or FreeType at run time.

## Threading and memory

- LVGL is not thread-safe. `lv_lock` and `lv_unlock` exist, and `lv_timer_handler` takes the lock itself. `source` 2026-11-13.
- Only `lv_tick_inc` and `lv_display_flush_ready` are safe from any thread.
- The built-in allocator is a TLSF pool of `LV_MEM_SIZE` bytes, 64 KiB by default.

## Testing

- `LV_USE_TEST` gives a test display, input devices, `lv_test_wait`, `lv_test_get_free_mem` and `lv_test_screenshot_compare(png)`. The compare needs lodepng and writes `*_err.png` on failure. `source` 2026-11-13.
- `lv_snapshot_take(obj, cf)` returns an `lv_draw_buf_t`.

## Licence

LVGL is MIT in all its repositories. LVGL Pro and the XML run time are commercial. `source` 2026-11-13, [licence](https://github.com/lvgl/lvgl/blob/master/docs/src/introduction/license.mdx).

## Existing C++ wrappers

| Project | State | What to learn |
|---|---|---|
| `lvgl/lv_binding_cpp` (official) | Marked work in progress, last commit 2023-07, MIT | Its README asks for a mostly generated binding |
| `vpaeder/lvglpp` | MIT, LVGL 8.3, handwritten | A wrapper leaving scope deletes its widget |
| `aptumfr/lv` | AGPL-3.0, C++20, LVGL 9.4 | Pointer-sized views; do not read its code into an MIT project |
| `pedapudi/lvgl_cpp` | Apache-2.0, C++20, active | Its README warns that lambdas capturing wrappers can dangle |
| `SoftOboros/lvglpp` | MIT, 2026 | Rust-style ownership tags |

All `source` 2026-11-13.

Name conflict: `pedapudi/lvgl_cpp` uses the name `lvgl_cpp` and `namespace lvgl`. `LV_*` macros ignore namespaces.

## What the maintainers have said

- 2021, issue #2303: generate the binding, with a few manual parts. Objects made in C++ and in C should look identical. `source` 2026-11-13.
- 2026-01-12, forum: a maintainer would like a good C++ binding, generated at least for the widgets.

## Constraints a wrapper must respect

1. A parent owns its children. Deletion is recursive.
2. Callbacks are C function pointers with `void* user_data`.
3. `LV_USE_EXT_DATA` is off by default. It attaches data and a destructor to objects, events, subjects, animations and timers.
4. Use only the public `include/lvgl` API. Take `lv_lock` from other threads.
