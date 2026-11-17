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

## `lv_obj_add_style` does not check its realloc

Found 2026-11-18 while sketching storage for lvgl26 (`docs/research/node-storage.md`, sketch (vii)).
Marks are dated 2026-11-18.
The report below is a draft for the owner to file; it was not filed.

### Findings

| Finding | Mark |
|---|---|
| `lv_obj_add_style` assigns `lv_realloc` straight to `obj->styles`, line 148 of `src/core/lv_obj_style.c` in v9.6.0, the v10 pin `53819fe` and master `2ee56544` | `source` |
| On failure `obj->styles` becomes NULL, the old array leaks, and the shift loop (line 153) and `lv_memzero` (line 156) write through NULL | `measured`, ASan and UBSan on master |
| `LV_ASSERT_MALLOC` is the only check, and `LV_USE_ASSERT_MALLOC` is 0 by default and in `lv_conf_template.h` | `source` |
| The default theme calls `lv_obj_add_style` while a widget is created: `lv_obj_create` crashed at 7 of 128 heap sizes | `measured` on master |
| `get_local_style` and `get_trans_style` in the same file check the same realloc since pull request 9984, merged 2026-04-20 | `source` |
| The shrinking realloc in `lv_obj_remove_style` (line 1684 on master) is also unchecked. TLSF shrinks in place; not reproduced | `source` |
| LVGL's review instructions say: "NULL dereference: every `lv_malloc`/`lv_realloc` result must be checked" | `source`, `.github/copilot-instructions.md` |
| In 2024 a maintainer answered that a failed small allocation is hard to recover from "and the system will crash anyway"; `LV_ASSERT_MALLOC` was added then | `source`, issue 6865 |
| The fix below, applied to master, makes both reproductions pass; the new test fails before it and passes after | `measured`, outside LVGL's test runner |

### Already reported or fixed?

Searched lvgl/lvgl issues and pull requests, open and closed, on 2026-11-18.

| Item | What it is | Same bug? |
|---|---|---|
| [#6865](https://github.com/lvgl/lvgl/issues/6865), closed 2024-10-02 | `style_cnt` overflow in v8; a comment asks what happens when this realloc fails | No: closed with an assertion, which is off by default |
| [#9984](https://github.com/lvgl/lvgl/pull/9984), merged 2026-04-20 | Checks the realloc in `get_local_style` and `get_trans_style` | No: it missed `lv_obj_add_style` in the same file |
| [#8614](https://github.com/lvgl/lvgl/issues/8614), closed | Integer overflow of `style_cnt` in `lv_obj_add_style`, v8.3 | No: a count overflow, not a failed allocation |
| [#7489](https://github.com/lvgl/lvgl/issues/7489), closed as stale | Missing allocation checks in `lv_draw.c`; discusses `ptr = realloc(ptr, n)` in general | Related, not this site |
| [#9794](https://github.com/lvgl/lvgl/issues/9794), closed | The same pattern in `lv_obj_class_create_obj` | Same pattern, another function |
| [#10406](https://github.com/lvgl/lvgl/pull/10406), open | Out-of-memory checks in `lv_array` | No |

No issue or pull request names this call.

### The report

```markdown
Title: fix(obj_style): lv_obj_add_style() writes through NULL when lv_realloc() fails

### LVGL version

master 2ee5654440bfc21a2cade4aa6e248f8d34b5e33e (2026-11-17); also v9.6.0.

### What happened?

`lv_obj_add_style()` grows the style list with

    obj->style_cnt++;
    obj->styles = lv_realloc(obj->styles, obj->style_cnt * sizeof(lv_obj_style_t));
    LV_ASSERT_MALLOC(obj->styles);

(src/core/lv_obj_style.c:148). When the allocation fails, `obj->styles` becomes NULL,
the old array leaks, and the loop at line 153 and `lv_memzero()` at line 156 write
through the NULL pointer. `LV_USE_ASSERT_MALLOC` is 0 by default, so nothing stops it.

The default theme calls `lv_obj_add_style()` from `lv_obj_create()`, so creating any
widget when the heap is nearly full can crash. #9984 fixed the same pattern in
`get_local_style()` and `get_trans_style()` of this file; this call was not covered.

### How to reproduce?

Default configuration (`LV_CONF_SKIP`), builtin allocator, `LV_MEM_SIZE` 65536:

    #include "lvgl.h"
    #include <stdio.h>

    static void fill_heap(void)
    {
        for(size_t size = (size_t)1 << 20; size >= 8; size /= 2) {
            while(lv_malloc(size) != NULL) {}
        }
    }

    int main(void)
    {
        lv_init();
        lv_display_create(320, 240);
        lv_obj_t * obj = lv_obj_create(lv_screen_active());

        static lv_style_t style;
        lv_style_init(&style);
        lv_style_set_bg_opa(&style, LV_OPA_COVER);

        fill_heap();
        lv_obj_add_style(obj, &style, 0);   /* crashes */
        printf("returned\n");
        return 0;
    }

Built with `-fsanitize=address,undefined`:

    src/core/lv_obj_style.c:153:9: runtime error: applying non-zero offset 48 to null pointer
    ==11718==ERROR: AddressSanitizer: SEGV on unknown address 0x000000000020
        #1 in lv_obj_add_style lv_obj_style.c:153

The same happens inside `lv_obj_create()` through `theme_apply()` (lv_theme_default.c:698)
when only a few hundred bytes are free.

### Expected

`lv_obj_add_style()` leaves the object with its previous styles and returns, as
`get_local_style()` does since #9984.

### Suggested fix

    lv_obj_style_t * new_styles = lv_realloc(obj->styles, obj->style_cnt * sizeof(lv_obj_style_t));
    if(new_styles == NULL) {
        obj->style_cnt--;
        LV_LOG_WARN("couldn't allocate styles");
        return;
    }
    obj->styles = new_styles;

A pull request with this change and a test in tests/src/test_cases/test_style.c follows.
```

### The fix, prepared

A local clone of master holds it on the branch `fix/obj-add-style-realloc`, commit `eda5f02`, not pushed.
It changes `lv_obj_add_style` as the report suggests, and adds `test_add_style_out_of_memory` to `tests/src/test_cases/test_style.c`.
The test takes all of the builtin heap and adds a style: the count and the array must not change.
Then it frees the heap and adds the style again.
It runs only with the builtin allocator, which is the one a test can exhaust.
`scripts/commit_msg_check.py` accepts the message.
`scripts/code-format.py` was not run: it needs astyle 3.4.12, which is not installed here.
LVGL's test runner was not run: it needs Python packages and libraries not installed here.

### LVGL's rules for a contribution

- Commit messages: `<type>(<scope>): <subject>`, imperative, lower case, no period, at most 90 characters (`docs/src/contributing/pull_requests.mdx`).
- The pull request title becomes the commit: the project squash-merges, and a check verifies the title.
- The template asks for `Fixes #xxxx`, a description, tests where applicable, and `scripts/code-format.py` with astyle 3.4.12.
- Contributions fall under the Developer's Certificate of Origin; no sign-off line is required ("no further action is needed").
- No rule about AI-assisted contributions was found in `docs/src/contributing` or `.github`.
