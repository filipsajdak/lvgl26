# The Rust binding of LVGL: what it teaches lvgl26

Research of 2026-11-17 on [lvgl/lv_binding_rust](https://github.com/lvgl/lv_binding_rust).
Marks follow [README.md](README.md).
Every mark below is dated 2026-11-17.

The question: what did the official Rust binding get right, where did it stall, and what should lvgl26 copy or avoid.

In short:

- The binding is a bindgen layer, a small code generator and a hand-written core.
- It froze on LVGL 8.3.7.
  Its last commit is 2025-01-22.
- Its hard problems are ownership, callbacks, style lifetimes and version drift.
  lvgl26 meets most of them by construction.
- Its easy problems are build setup, silent gaps and stale examples.
  These are the ways lvgl26 could still fail.

LVGL's own C++ binding and the existing C++ wrappers are in [lvgl-upstream.md](lvgl-upstream.md).
They are not repeated here.

## How the work was done

- Repository facts come from the read-only GitHub API: README, file tree, manifests, CI workflow, 285 commits, and 206 issues and pull requests.
- I read 42 of the 49 Rust files at the last commit, `d83b3740`.
- I did not read `examples/bar.rs`, the two files in `lvgl/src/drivers/`, `lv_core/mod.rs`, `misc/area.rs`, `misc/mod.rs` and `point.rs`.
- I opened 66 of the 206 threads: about 20 in full, the others by their opening and closing comments.
- I know the other 140 threads by title, label and comment count only.
- Registry facts come from the crates.io API and from docs.rs pages.
- A row marked `measured` is a count I made from these data.
  The row says how.
- I built nothing and ran no Rust.
  A statement about what code does is a reading of the code, not a test.
- People appear by role and issue number, not by name (rule 7 of the README).
- File links use commit `d83b3740`, so they do not move.
- Facts about lvgl26 come from this repository at `origin/main` on 2026-11-17.

## Short answers

| Question | Answer |
|---|---|
| 1. How is it made? | Three crates: raw bindgen output, a generator that wraps simple functions, and hand-written code for everything else. |
| 1. How much is covered? | Nothing tracks it. For seven widgets the published crate wraps 46 of 149 functions. |
| 2. Which LVGL? | 8.3.7 of 2023-05-03. LVGL 9.0 came on 2024-01-22 and 9.6.0 on 2026-09-16. Master has no v9 support. |
| 3. Ownership? | No widget is ever freed. Lifetimes are borrow markers. Closures are leaked into one `user_data` slot. |
| 4. Memory? | `no_std` plus an allocator. Nothing is measured. A display flush copies the whole draw buffer onto the stack. |
| 5. Users? | Build setup and cross compiling dominate. Missing functions come second. There is no agent story. |
| 6. What stalled? | The generator rewrite, `lv_conf` as TOML, CSS styles, object deletion and LVGL 9. Two people wrote 250 of 285 commits. |
| 7. What do we learn? | Eleven points in the last section. Three need the owner's call. None overturns a decision. |

## 1. Architecture

### 1.1 Three crates

| Part | What it does | Mark |
|---|---|---|
| [`lvgl-sys`](https://github.com/lvgl/lv_binding_rust/blob/d83b3740/lvgl-sys/build.rs) | Compiles every `.c` file of the vendored LVGL with the `cc` crate, then runs bindgen (clang) over `lvgl_sys.h`. | `source` |
| [`lvgl-codegen`](https://github.com/lvgl/lv_binding_rust/blob/d83b3740/lvgl-codegen/src/lib.rs) | Reads bindgen's output as Rust text with `syn`, finds widgets, and writes wrapper methods. It runs from `lvgl/build.rs`. | `source` |
| [`lvgl`](https://github.com/lvgl/lv_binding_rust/tree/d83b3740/lvgl/src) | The user crate: `Obj`, `Style`, `Display`, `Event`, `Animation`, input devices, plus the generated `widgets`. | `source` |

- The workspace holds only these three crates.
  LVGL and `lv_drivers` are git submodules.
  [`.gitmodules`](https://github.com/lvgl/lv_binding_rust/blob/d83b3740/.gitmodules) `source`
- `generate_comments(false)` is set for bindgen, so LVGL's doc comments do not reach the Rust API.
  [`lvgl-sys/build.rs`](https://github.com/lvgl/lv_binding_rust/blob/d83b3740/lvgl-sys/build.rs) `source`
- `lvgl_sys.c` re-exports five colour macros as functions, because bindgen cannot see macros that compute. `source`
- The configuration is a header found through the environment variable `DEP_LV_CONFIG_PATH`.
  The build panics when it is missing. `source`

### 1.2 What the generator accepts

- A widget is any function that matches `^lv_([^_]+)_create$` and takes one argument. `source`
- A method is any function whose first argument type contains `lv_obj_t` and whose name starts with `lv_<widget>`.
  There is no trailing underscore in that test. `source`
- Only arguments of type `u16`, `i32`, `u8`, `bool` and `*const c_char` are accepted.
  A return type must be `bool` or an integer. `source`
- Every other function ends in `WrapperError::Skip`.
  Nothing logs it, and nothing lists it. `source`
- The missing underscore has a visible effect.
  On docs.rs the published `Btn` carries `lv_btnmatrix_set_btn_width`, `lv_btnmatrix_set_one_checked` and `lv_btnmatrix_set_selected_btn`. `measured`: read from the docs.rs page of `Btn` in 0.6.2.
- Two user reports fit the skip rule: a table without `set_row_height` ([#122](https://github.com/lvgl/lv_binding_rust/issues/122)) and no `get_text` ([#162](https://github.com/lvgl/lv_binding_rust/issues/162)).
- The table's `set_col_width` was then written by hand in `table.rs`. `source`
- `analysis.rs` sketches the next generator: a `scope` for pointers that LVGL keeps, `allow_none` read from comments, and a function kind.
  No code uses it (`#![allow(dead_code)]`). `source`

### 1.3 What a user writes

A button with a callback, a style and a label that changes.
This is [`examples/button_click.rs`](https://github.com/lvgl/lv_binding_rust/blob/d83b3740/examples/button_click.rs), shortened.

```rust
let mut screen_style = Style::default();
screen_style.set_bg_color(Color::from_rgb((0, 0, 0)));
screen.add_style(Part::Main, &mut screen_style);

let mut button = Btn::create(&mut screen)?;
button.set_align(Align::LeftMid, 30, 0);
button.set_size(180, 80);
let mut btn_lbl = Label::create(&mut button)?;
btn_lbl.set_text(CString::new("Click me!").unwrap().as_c_str())?;

button.on_event(|_btn, event| {
    if let lvgl::Event::Clicked = event { /* flip a bool, call btn_lbl.set_text(..) */ }
})?;
```

- The crate has no bound value.
  LVGL's observer module is not in 8.3.7 (it is in v9.0.0), so the closure sets the text by hand. `source`
- Three open pull requests, [#203](https://github.com/lvgl/lv_binding_rust/pull/203), [#204](https://github.com/lvgl/lv_binding_rust/pull/204) and [#205](https://github.com/lvgl/lv_binding_rust/pull/205), remove `?` from this example and two others.
  They say the calls no longer return `Result`. `source`

The same three things in lvgl26.
The tree is [`docs/quick-start.md`](../quick-start.md), shortened.
The style is the example in the header comment of [`include/lv/style.hpp`](../../include/lv/style.hpp).
I did not compile them together.

```cpp
constexpr auto card = lv::style({.bg_color = tok.color.card, .radius = tok.radius.card});
lv::box({.style = card})

lv::arc({.range = {0, 240}, .size = {120 * px, 120 * px}, .value = lv::bind(speed)})[
    lv::label({.align = "center", .text = lv::bind(speed)})],
lv::button({.on_clicked = &reset})[lv::label({.text = "Reset"})]
```

### 1.4 Coverage and how it is tracked

| Fact | Mark |
|---|---|
| The README lists three unchecked features: images, file system and tasks. It lists no widgets. | `source` |
| On master only seven widgets have hand-written files: arc, bar, keyboard, label, meter, slider and table. The rest is generated. | `source` |
| The published 0.6.2 has 27 widget types. For arc, label, bar, slider, textarea, dropdown and chart it wraps 46 of the 149 functions in LVGL's headers, 31 percent. | `measured`: the count of methods before the trait list on each docs.rs page, minus `create`, `create_at`, `new` and `on_event`; against the widget headers at the commit that 0.6.2 pins (2023-02-07). |
| The count is rough. Hand-written methods count as wrapped, the header regex reads only functions that start in column 0, and docs.rs builds with the vendored `lv_conf.h`. | `measured` |
| `style.rs` lists 89 style setters through two macros. Twelve more are commented out, among them `set_anim`, `set_transition`, `set_bg_img_src`, `set_color_filter_dsc` and `set_bg_grad`. | `source` |
| The event enum has 17 of LVGL's events. Any other code fails to convert, and the reverse conversion maps an unlisted event to `CLICKED` with a `TODO`. | `source` |
| A pull request in review ([#197](https://github.com/lvgl/lv_binding_rust/pull/197)) widens the generator. Its text lists 317 functions as added and 70 as skipped. It turns Rust enums into `i32`. | `source`: the PR text says so; I did not build it. |
| No gate counts coverage. The CI runs `cargo build` and `cargo test -- --test-threads 1` on one Ubuntu job. | `source`: [`rust.yml`](https://github.com/lvgl/lv_binding_rust/blob/d83b3740/.github/workflows/rust.yml) |
| I found 23 `#[test]` functions in the files I read, nine of them in the generator. None counts coverage. | `measured`: `grep -c` over the fetched files. |

### 1.5 Compared with reflection

| | Rust binding | lvgl26 |
|---|---|---|
| Source of names | Function names of bindgen's output, split by regex. | Reflection over the headers: `std::meta::identifier_of`. |
| Source of types | A table of five Rust types. Enums are written by hand, or become `i32`. | The parameter types of the setters. An unmapped type stays its C type: [`values.hpp`](../../include/lv/values.hpp) `value_type`. |
| What it drops | Silently, everything outside the table. | Setters that are variadic, deprecated, named like a keyword or style setters: [`is_usable_setter`](../../include/lv/props.hpp). |
| Prefix test | `lv_btn` also matches `lv_btnmatrix_*`. | `lv_<widget>_set_`, with both underscores. |
| Units and enum names | One `Align` enum of 21 values, written by hand. | Names in quotes (`.align = "center"`), read from the enum by reflection. |
| What is generated | Widget methods that fit the table, and 89 listed style setters. | 35 widgets, 210 widget properties, 139 style properties, 78 events and 38 enums at v9.6.0. |
| How it is kept honest | Nothing. | `docs/reference/lvgl-*.json` is committed, and the test `reference_data` fails on a difference. |

- The row "What is generated" is `measured`: I counted the committed `docs/reference/lvgl-v9.6.0.json`.
  The v10 pin has 29 widgets, 210 widget properties and 79 events.
  The six widgets it lacks (`list`, `win`, `menu` and its three parts) have no own property, so the total stays 210.
- The prefix row is `measured` on both pins.
  `button` has no own property and `buttonmatrix` has seven.
- `arc` and `arclabel` have 14 each, with no name in common.
  `image` and `imagebutton` share `src`, each from its own setter.
- Rust's collision is a bug of the rule, not of Rust.
  The lvgl26 rule has the underscore by construction, and the reference test would show a leak.

## 2. Versions

### 2.1 What happened

| Date | Event | Mark |
|---|---|---|
| 2020-04-10 | First commit. bindgen and `cc` from the start. | `source` |
| 2023-04-02 | Release 0.6.2, the last one on crates.io. Its pin is an LVGL commit of 2023-02-07. | `source` |
| 2023-05-03 | LVGL v8.3.7. This tag is the submodule pin of master today. | `source`: `compare` of the two commits shows them identical. |
| 2023-05-09 | Draft pull request #121, "LVGL 9.0", described as "perma-draft". Its branch has one commit. | `source` |
| 2023-06-21 | End of the busy period: 110 commits in 2023, 108 of them on or before this day. Then 14 commits in 19 months. | `measured`: the commit list of master. |
| 2024-01-22 | LVGL v9.0.0. | `source`: [release](https://github.com/lvgl/lvgl/releases/tag/v9.0.0) |
| 2024-06-14 | Pull request #172 bumps the pin to 8.3.11. It is still open, with no comment. | `source` |
| 2024-12-22 | Issue #185: an LVGL maintainer says the current maintainers are inactive. Two volunteers get write access. | `source` |
| 2025-01-15 | The branch `release/v8.3` is created, as the plan of #176 says. | `source` |
| 2025-01-22 | Last commit on master. It is 22 months old on 2026-11-17. | `source` |
| 2025-02-24 | Issue #195 "bump to LVGL 9?". It has no answer. | `source` |
| 2025-06-17 | Issue #201 "Current status of these bindings?". Six comments follow, mostly "interested too". No maintainer replies. | `source` |
| 2026-04-13 | Draft pull request #206: one user ports a minimal surface to LVGL 9.2. | `source` |

- A collaborator last commented on 2026-01-07 ([#205](https://github.com/lvgl/lv_binding_rust/pull/205)), to say the fix works and to ask why CI fails. `source`: the repository's comment feed.
- A collaborator last reviewed a pull request on 2025-05-14 ([#197](https://github.com/lvgl/lv_binding_rust/pull/197)).
  Nothing was merged after 2025-01-22. `source`
- The registry agrees: the newest `lvgl` is 0.6.2 of 2023-04-02.
  All versions together have 26,823 downloads, 3,485 of them recent. `source`: crates.io API.
- LVGL's own bindings page lists C++, JavaScript, MicroPython and PikaScript.
  It has no page for Rust. `source`: `docs/src/integration/bindings` on master.

### 2.2 What LVGL 9 would cost

- A user port to LVGL 9.2 leaves the generator unused.
  Its text calls regenerating against LVGL 9 "separate follow-up work".
  It replaces 4,083 lines of the safe layer with a slim hand-written surface (display, colour, label, image, snapshot). `source`: the text of [#206](https://github.com/lvgl/lv_binding_rust/pull/206) says so; I did not build it.
- The same port says the input module cannot move, because LVGL 9 removed `lv_indev_drv_t`, and that `lv_color_t` changed to RGB888 fields. `source`: the same text; not built.
- `Screen::try_from` reads the field `parent` of `lv_obj_t`.
  In v9.6.0 the body of `_lv_obj_t` is in `src/core/lv_obj_private.h`, outside `include/lvgl`. `source`
- The generator's name rules (`lv_<widget>_create`) would survive renames such as `btn` to `button`.
  They do not give enums, parts or events. This is my inference; I did not test it.
- My reading: the cost sits in the hand-written names.
  They are parts (10), aligns (21), events (17), style properties (about 90) and one function per built-in font size. `source`

### 2.3 What the community did

- `lightvgl-sys` is a new raw-binding crate.
  It has crate version 9.5.5 (2026-09-27) and a table from crate version to LVGL version, 9.2.x to 9.6.x.
  [crates.io](https://crates.io/crates/lightvgl-sys), `source`.
- Its README says it does not follow semantic versioning.
  Major and minor versions are tied to LVGL, and it advises `~9.5.5`. `source`
- `lv_bevy_ecs` is a safe layer over it, version 0.12.0 (2026-10-17), with a table from its version to the `-sys` version.
  [crates.io](https://crates.io/crates/lv_bevy_ecs), `source`.
- Its author offered to upstream the work in [#202](https://github.com/lvgl/lv_binding_rust/issues/202) on 2025-12-24.
  Nobody from LVGL has answered. `source`
- The split of `-sys` into its own repository was proposed in [#176](https://github.com/lvgl/lv_binding_rust/issues/176) in 2024-07.
- An LVGL maintainer agreed to two repositories on 2025-02-06.
  The answer asked for branches per LVGL version, not a repository per version. `source`

## 3. Ownership and safety

### 3.1 How each thing is modelled

| Topic | In the Rust binding | In lvgl26 | Mark |
|---|---|---|---|
| Widget lifetime | `Obj` has no `Drop`, and no `lv_obj_del` appears in the files I read. Widgets live until LVGL ends. `Obj<'a>` holds a `PhantomData` marker. | A view never deletes. `lv::owned` deletes, `lv::weak` empties itself through `lv_obj_add_delete_cb`. | `source` |
| Parent and child | `create(parent: &'a mut ...)` borrows the parent mutably, so two children cannot coexist. Generated `create` returns an unbounded lifetime. | The parent owns the child. `view::create` takes a view by value. | `source`: issue #166 |
| Styles | `add_style(&'a mut Style)`: the style must outlive the widget. `Style` is a `Box<lv_style_t>` in LVGL's heap; the files I read never call `lv_style_reset`. | A style is a const `lv_style_t` in read-only memory. LVGL stores a pointer to it. | `source` |
| Strings | `set_text` takes `&CStr`. `get_text` was proposed as `&'static str`, which a reviewer called a memory-safety issue. | A constant `.text` is kept by pointer through `lv_label_set_text_static`, unless the label cuts with dots. | `source`: #192, [`props.hpp`](../../include/lv/props.hpp) `static_text_of` |
| Fonts | `Font` copies the `lv_font_t` into LVGL's heap and leaks it. A built-in font needs the nightly feature `cfg_accessible`. A custom font needs `unsafe`. | A font is a typography token. | `source` |
| Callbacks | `on_event` boxes the closure into the widget's one `user_data` slot. A second call replaces it. | `on<Code>(f)` keeps state in the event's own `user_data`. A handler captures nothing or one pointer-sized value. | `source` |
| Start and stop | `ctor` runs `lv_init` before `main`. A feature turns it off for bare metal. `deinit` is `unsafe`. | `lv::application` starts LVGL when built and stops it when it ends. | `source` |
| Threads | `Send` was removed from `Obj`. No file I read calls `lv_lock` or uses a mutex. | `ui_scheduler` and tests under ThreadSanitizer. | `source` |
| Errors | `LvResult` with four error kinds, plus `unwrap` in `Label::from`. | Compile error, then contract, then `std::expected`. Nothing throws. | `source` |

- The closure is stored with `Box::into_raw` and I found no code that frees it. `source`
- The animation trampoline checks the animation's `user_data` but calls the closure in the widget's `user_data`.
  That is the slot `on_event` writes, so it is null for a widget without a handler. `source`: [`anim.rs`](https://github.com/lvgl/lv_binding_rust/blob/d83b3740/lvgl/src/misc/anim.rs), line 118.
- A CI run of [#205](https://github.com/lvgl/lv_binding_rust/pull/205) on 2026-01-07 failed in `anim_test` with "null pointer dereference occurred" at `anim.rs:118:28`.
  A contributor called it unsound pointer use. `source`
- The test creates an animation with an empty closure and starts it.
  It does not use `on_event`. `source`
- The flush trampoline copies the whole `lv_disp_drv_t` and writes it back.
  A comment says that not doing so "causes a segfault in rust >= 1.69.0". `source`

### 3.2 What was reported

| Report | What it says | Outcome | Mark |
|---|---|---|---|
| [#69](https://github.com/lvgl/lv_binding_rust/issues/69) | Safe code can call LVGL before `lv_init`: undefined behaviour. | Closed 2023-03-29. Fixed with `ctor`. | `source` |
| [#110](https://github.com/lvgl/lv_binding_rust/issues/110) | A dropped `Style` leaves a dangling pointer in a widget. | Closed 2023-06-20, the day a commit added lifetimes to style assignment. | `source` |
| [#111](https://github.com/lvgl/lv_binding_rust/issues/111) | Screens are never deleted. Deleting on `Drop` risks double free. Children outlive a deleted parent. | Open since 2023-05-06. | `source` |
| [#140](https://github.com/lvgl/lv_binding_rust/issues/140) | Tracking issue for dropping and threading. It has no comment, and its benchmark question has no answer. | Open since 2023-06-20. | `source` |
| [#144](https://github.com/lvgl/lv_binding_rust/pull/144) | Delete `Obj` on drop. The author calls it "very broken". | Open since 2023-06-21. | `source` |
| [#166](https://github.com/lvgl/lv_binding_rust/issues/166) | `create` has the wrong lifetime and `add_style` the wrong mutability. | Open since 2024-04-23. | `source` |
| [#192](https://github.com/lvgl/lv_binding_rust/issues/192) | Functions that return pointers: a `&'static str` over a label's buffer is unsound. | Open. The generator skips them. | `source` |
| [#180](https://github.com/lvgl/lv_binding_rust/issues/180) | A crash inside the crate's own `strncmp`, called by libdbus through SDL. | Open, no reply. | `source` |
| [#186](https://github.com/lvgl/lv_binding_rust/issues/186) and [#163](https://github.com/lvgl/lv_binding_rust/issues/163) | Unaligned hard fault on STM32H7. Cause: `arm-none-eabi-gcc` shortens enums, bindgen's clang does not. | Fix: pass `-fshort-enums` to bindgen. The README lists #163 under known issues. | `source` |
| [#152](https://github.com/lvgl/lv_binding_rust/issues/152) | A comment says the crate "is full of UB", with wrong lifetimes, and that the team regrets adopting it. | Open. | `source` |

- `lvgl-sys` exports `strcmp`, `strncmp`, `strchr` and more as `#[no_mangle]` Rust functions, taken from the Redox OS libc. `source`
- In the stack trace of #180, libdbus calls the crate's `strncmp`.
  So a system library can end up on the binding's string code. `source`
- A fix for a read past the string end (2024-03-22) is on master.
  The registry still has 0.6.2 of 2023. `source`: the commit and crates.io.
- The design of #110 works: a style cannot be dropped while a widget borrows it.
  It makes a UI built in several functions awkward.
  [#149](https://github.com/lvgl/lv_binding_rust/issues/149) shows a user who leaks the whole handle to get out. `source`
- Users who left said it.
  One wrote that "leaking widget memory structure is far simpler and match pretty well embedded constrains" ([#158](https://github.com/lvgl/lv_binding_rust/issues/158)). `source`
- The successor `lv_bevy_ecs` also offers `leak` for widgets that live as long as the program. `source`: its README.

## 4. Memory and embedded

| Topic | Fact | Mark |
|---|---|---|
| `no_std` | The `lvgl` crate is `no_std` unless tests run. Rust values that LVGL keeps go into LVGL's heap through the crate's own `Box` in `mem.rs`. The feature `alloc` adds `CString` helpers, and the examples need it. | `source` |
| Allocator | The feature `lvgl_alloc` makes LVGL's allocator Rust's global one. The README says the pool may need to grow. `Box::new` panics when LVGL's heap is full. | `source` |
| The global allocator | It ignores `layout.align()`. | `source`: [`allocator.rs`](https://github.com/lvgl/lv_binding_rust/blob/d83b3740/lvgl/src/allocator.rs) |
| Draw buffer | `DrawBuffer<N>` is a pinned `Box` of `N` pixels in LVGL's heap. The examples use N = 240 x 240 = 57,600, which is 112.5 KiB at 16-bit colour. | `source`; the size is my arithmetic. |
| Flush | The trampoline builds an array of `N` colours on the stack and copies all `N`, whatever the area. For the same N that is another 112.5 KiB, of stack. | `source`; the size is my arithmetic. |
| Heap size in the examples | `examples/include/lv_conf.h` sets `LV_MEM_SIZE` to 1 MiB, `LV_COLOR_DEPTH` to 16. | `source` |
| Footprint claims | The README repeats LVGL's "low memory footprint". The crate description says "16K RAM, 64K Flash". No measurement exists in the README, CHANGELOG or issues I read. | `source` |
| Targets in CI | One Ubuntu job with SDL2. Tracking issues for other targets ([#73](https://github.com/lvgl/lv_binding_rust/issues/73), [#78](https://github.com/lvgl/lv_binding_rust/issues/78)) are open. | `source` |
| Embedded users | ESP32 and STM32 boards appear in issues ([#102](https://github.com/lvgl/lv_binding_rust/issues/102) has 33 comments). The demo projects are third-party. | `source` |
| Cross compiling | Users needed `CROSS_COMPILE`, `CC`, `AR`, a sysroot for bindgen and `-fshort-enums`. The build script passes `-target` to bindgen and has one special case, Emscripten. | `source` |

- The memory story has no number.
  The only tool is `lv_mem_monitor`, printed by the demo. `source`
- lvgl26 measures instead.
  [`memory.md`](memory.md) gives bytes per call on a Cortex-M4 and the tests keep them. `source`

## 5. Users

### 5.1 What they ask for and what breaks

| Theme | Threads | Mark |
|---|---|---|
| Build setup: `DEP_LV_CONFIG_PATH`, submodules, libclang, SDL2 | [#152](https://github.com/lvgl/lv_binding_rust/issues/152), [#179](https://github.com/lvgl/lv_binding_rust/issues/179), [#181](https://github.com/lvgl/lv_binding_rust/issues/181), [#193](https://github.com/lvgl/lv_binding_rust/issues/193), [#194](https://github.com/lvgl/lv_binding_rust/issues/194) | `source` |
| Cross compiling and targets: ESP32, ARM, thumbv6m, Emscripten | [#53](https://github.com/lvgl/lv_binding_rust/issues/53), [#58](https://github.com/lvgl/lv_binding_rust/issues/58), [#102](https://github.com/lvgl/lv_binding_rust/issues/102), [#125](https://github.com/lvgl/lv_binding_rust/issues/125), [#155](https://github.com/lvgl/lv_binding_rust/issues/155) | `source` |
| Missing functions and widgets | [#105](https://github.com/lvgl/lv_binding_rust/issues/105), [#107](https://github.com/lvgl/lv_binding_rust/issues/107), [#118](https://github.com/lvgl/lv_binding_rust/issues/118), [#122](https://github.com/lvgl/lv_binding_rust/issues/122), [#162](https://github.com/lvgl/lv_binding_rust/issues/162) | `source` |
| Input devices other than a pointer | [#159](https://github.com/lvgl/lv_binding_rust/issues/159); the module docs list keyboard, button and encoder as unsupported, though `encoder.rs` has an `Encoder` driver | `source` |
| Examples that do not build or run | [#146](https://github.com/lvgl/lv_binding_rust/issues/146), [#150](https://github.com/lvgl/lv_binding_rust/issues/150) | `source` |
| Fonts and custom `lv_conf` | the README, the module docs of `font`, [#11](https://github.com/lvgl/lv_binding_rust/issues/11) | `source` |

- Of 206 threads, 44 have a title with one of: build, compil, cross, linker, esp32, error, fail. `measured`: a word match over titles, so it is rough.
- The labels count 34 `enhancement`, 22 `question` and 20 `bug` among 206 threads. `measured`: the labels of every thread.
- Discussions are off, so questions go to the issue tracker. `source`: the repository settings.

### 5.2 Docs, examples and agents

- The docs are the README (137 lines), the module docs on docs.rs, and nine examples.
  There is no guide. `source`
- The whole `meter` example is commented out under "TODO: Redo this example" ([#150](https://github.com/lvgl/lv_binding_rust/issues/150)).
  `sdl.rs` starts with "Still WIP". `source`
- CI cannot catch a broken example.
  All nine examples declare `required-features` that CI does not enable, so `cargo test` skips them. `source`: the manifest; I did not run it.
- The commits "Fix examples" (2024-12-22) and "Fix input device example" (2025-01-02), and the open pull requests #203 to #205, repair examples after the API moved. `source`
- A user asked for a picture of each example's expected result ([#146](https://github.com/lvgl/lv_binding_rust/issues/146)). `source`
- There is no `AGENTS.md`, `CLAUDE.md` or `llms.txt` in the tree. `measured`: a name search of the file list.
- The one mention of AI is a user on 2023-05-01.
  Copilot's grid code did not compile, and the user guessed that Copilot and ChatGPT have too little Rust code to work with ([#105](https://github.com/lvgl/lv_binding_rust/issues/105)). `source`

## 6. What failed or stalled

| Promise | Where | State |
|---|---|---|
| Styles written as CSS | [#1](https://github.com/lvgl/lv_binding_rust/issues/1), 2020-04-15 | Open. A separate CSS compiler for LVGL 7 was tried. I found no CSS macro in the tree. |
| `lv_conf` as TOML | [#11](https://github.com/lvgl/lv_binding_rust/issues/11), PR [#82](https://github.com/lvgl/lv_binding_rust/pull/82) | Open. A file `lvgl/lv_conf.toml` with two values is in the tree. |
| Attribute macros for set-up code | [#68](https://github.com/lvgl/lv_binding_rust/issues/68) | Open. I found no such macro. |
| A rewrite of the generator "that would harden us against LVGL version bumps" | [#100](https://github.com/lvgl/lv_binding_rust/issues/100) | Open. Its sketch, `analysis.rs`, is unused. |
| Release 0.7.0 | [#97](https://github.com/lvgl/lv_binding_rust/issues/97) | Never released. |
| Object deletion and threads | [#111](https://github.com/lvgl/lv_binding_rust/issues/111), [#140](https://github.com/lvgl/lv_binding_rust/issues/140), [#144](https://github.com/lvgl/lv_binding_rust/pull/144) | Open. |
| Images, file system, tasks | README list | Unchecked. |
| LVGL 9 | [#121](https://github.com/lvgl/lv_binding_rust/pull/121), [#195](https://github.com/lvgl/lv_binding_rust/issues/195), [#206](https://github.com/lvgl/lv_binding_rust/pull/206) | Open. |

All rows are `source`.

- Maintenance load.
  Two people wrote 250 of 285 commits. `measured`: author names in the commit list of master.
- Seven branches besides master exist.
  Six hold unmerged work from 2023 to 2024. `source`
- 58 threads are open: 43 issues and 15 pull requests. `source`
- A contributor opened four pull requests in 2025-04: more functions, a fixed Meter, Windows builds and a CI test.
  None has merged in 19 months. `source`
- The only gate that LVGL asked for is its binding test, and [#200](https://github.com/lvgl/lv_binding_rust/pull/200) is a draft of it. `source`
- Bus factor.
  The second author put the project on hold in 2023-09 ([#150](https://github.com/lvgl/lv_binding_rust/issues/150)).
- Two volunteers took over in 2024-12.
  Between them they have six commits, all before 2025-01-19. `measured`: the commit list.
- What worked: the 2023 sprint with tracking issues, the lifetime fix for styles and the `Box` that places Rust values in LVGL's heap.
- The forks that live on are a result too.

## 7. Other bindings

Only what I read. `source` for each line unless it says otherwise.

- **MicroPython** ([lv_binding_micropython](https://github.com/lvgl/lv_binding_micropython), active 2026-09-06).
  A generator, `gen_mpy.py`, parses the headers with pycparser.
  Memory belongs to MicroPython's garbage collector.
  LVGL's `user_data` holds the callable, under a documented callback convention that the header must follow.
  One thread only.
  The API is generated for the widgets that `lv_conf.h` enables.
- **LVGL's CI builds it.** `build_micropython.yml` in `lvgl/lvgl` builds four ports (unix, STM32, RP2, ESP32) against each LVGL commit and runs `tests/micropy_test` on the unix port.
  The job has `continue-on-error: true`, so it does not block.
- **LVGL's binding test.** `tests/src/test_cases/test_bindings.c` and `binding.png` exist in v9.0.0, v9.2.0 and v9.6.0.
  The version on master builds one UI with a style, grid, subject binding, chart arrays, a button matrix, animations, canvas drawing and images.
  An LVGL maintainer asked the Rust binding to pass it ([#176](https://github.com/lvgl/lv_binding_rust/issues/176)).
  The maintainer called it a "good measure for readiness" and said "most of the people should be happy" with a binding that passes.
- **C++** ([lv_binding_cpp](https://github.com/lvgl/lv_binding_cpp)).
  Last commit 2023-07-31, as [lvgl-upstream.md](lvgl-upstream.md) says.
  LVGL's docs still call it "in progress".
- **Lua** ([luavgl](https://github.com/XuNeo/luavgl), active 2026-10-20).
  A widget takes a table of named properties: `root:set { w = 300, flex = {...} }`.
  This is the nearest form to lvgl26's designated initializers.
  I read the README only.
- **Nim** ([lvgl.nim](https://github.com/mantielero/lvgl.nim), last push 2024-12-29).
  Generated with nimterop for LVGL 9.2.
  Its README says "no sugar yet" and lists "How to deal with callbacks?" as open.
- **Zig** ([pinephone-lvgl-zig](https://github.com/lupyuen/pinephone-lvgl-zig), last push 2023-08-10).
  A Zig wrapper, `lvgl.zig`, for LVGL 8.3.3, inside an article series.
  I did not read the wrapper.
- **JavaScript** ([lv_binding_js](https://github.com/lvgl/lv_binding_js)).
  LVGL's docs page says it uses React's virtual DOM.
  Found, not read further.
- **Go.** A GitHub search for "lvgl golang" found nothing.
  I did not look further.

## 8. What I could not check

- I built and ran no Rust code.
  Every claim about behaviour is a reading, including the animation trampoline and the closure slot.
- I did not read the seven files listed at the top, nor the threads I know by title only.
- The 31 percent coverage figure covers seven widgets, not all.
  It compares a docs.rs build with a rough header regex.
- The text of [#197](https://github.com/lvgl/lv_binding_rust/pull/197) and [#206](https://github.com/lvgl/lv_binding_rust/pull/206) is what their authors say.
  I built neither.
- I did not read the code of `lightvgl-sys` or `lv_bevy_ecs`.
  Their pages are only a README and the registry.
- I did not run lvgl26 against the binding test.
  I read the master version of `test_bindings.c` and only know that the files exist at three tags.
- Who holds write access today is not shown by the API calls I made.
  I used the `author_association` of each comment.

## 9. Lessons for lvgl26

Each point says what to do and where it lands.
Three need the owner's call (3, 4 and 11).
None overturns a recorded decision of the owner.

1. **Do not drop silently; say what is left out.**
   The generator's `Skip` fits [#122](https://github.com/lvgl/lv_binding_rust/issues/122) and [#162](https://github.com/lvgl/lv_binding_rust/issues/162).
   Pull request [#197](https://github.com/lvgl/lv_binding_rust/pull/197) still lists 70 skipped functions.
   We do better: an unmapped type passes through, and the exclusions are named rules in `is_usable_setter` ([`include/lv/props.hpp`](../../include/lv/props.hpp)).
   We can do more.
   List each excluded setter with its reason in `docs/reference/lvgl-*.json`, and let the test `reference_data` fail when a setter is excluded without one.
   The status page can then state a coverage figure, which the Rust README never did.

2. **A list written by hand is the first thing to rot.**
   Rust lists 17 events, 10 parts, 21 aligns, about 90 style properties and one function per font size.
   Its event converter maps the rest to `CLICKED`.
   lvgl26 reads 78 events, 139 style properties and 38 enums from the headers.
   Keep it so.
   Each new hand-written table of names in `include/lv` needs a reason in review, because the next LVGL release breaks it.

3. **A pin needs an owner and a canary.**
   The pin of master is a commit of 2023-05-03.
   The bump to 8.3.11 has sat open for 29 months ([#172](https://github.com/lvgl/lv_binding_rust/pull/172)).
   lvgl26 pins v9.6.0 by hash and the v10 line by commit.
   The v10 pin is 9 commits behind LVGL master on 2026-11-17, so someone does move it.
   I found no job that builds LVGL master or a newer tag: the host job of `nightly.yml` runs two presets on the pins.
   Add a scheduled job that builds against LVGL master and the newest tag, opens an issue on failure and never blocks.
   Add a table from lvgl26 revision to LVGL version to the README, as `lightvgl-sys` does.
   **For the owner:** `CLAUDE.md` says both LVGL versions must work.
   No document says who moves the v10 pin or when.
   This extends that decision.

4. **Pass LVGL's own binding test.**
   `test_bindings.c` and `binding.png` exist in v9.0.0, v9.2.0 and v9.6.0.
   LVGL runs the MicroPython port against them and asked the Rust binding to do the same.
   We already compare each example with LVGL's C original in one process, with `same_pixels` ([`tests/same_pixels.hpp`](../../tests/same_pixels.hpp)).
   Do the same here.
   Copy the C function `create_ui` into a test file and keep LVGL's MIT notice.
   Build the same UI with lvgl26 and compare the two pictures, once per pin.
   Do not compare with `binding.png`: that picture depends on LVGL's own test configuration.
   What lvgl26 cannot say yet, such as the canvas and the external chart arrays, would show up as the gaps.
   An LVGL maintainer calls the test "a good measure for readiness".
   I read the version on master; v9.6.0 has a file of the same size, which I did not compare.
   **For the owner:** the feature order puts more example ports next ([`site/status.html`](../../site/status.html)).
   This test is small, but it is new work ahead of that order.

5. **Own widgets with delete callbacks, not with lifetimes.**
   Rust tried borrow markers and ended with widgets that are never freed ([#111](https://github.com/lvgl/lv_binding_rust/issues/111), [#140](https://github.com/lvgl/lv_binding_rust/issues/140), [#144](https://github.com/lvgl/lv_binding_rust/pull/144)).
   The case it could not solve is a child deleted by its parent.
   `lv::owned` and `lv::weak` ([`include/lv/owned.hpp`](../../include/lv/owned.hpp)) solve it with `lv_obj_add_delete_cb`.
   We do better, at 24 bytes per tracked widget ([`memory.md`](memory.md)).
   Embedded users also asked for the opposite ([#158](https://github.com/lvgl/lv_binding_rust/issues/158)): a UI that is never deleted and pays nothing.
   Make sure `lv::mount` has such a path.
   [`memory.md`](memory.md) already lists that `lv::mount` registers its delete callback four times.

6. **A constant style beats a checked borrow.**
   Rust's fix for dangling styles ([#110](https://github.com/lvgl/lv_binding_rust/issues/110)) forced `&'a mut Style` and a heap `Box` for each style.
   It blocks a UI that is built in several functions ([#149](https://github.com/lvgl/lv_binding_rust/issues/149)).
   lvgl26's const `lv_style_t` in read-only memory has no lifetime and no heap ([`include/lv/style.hpp`](../../include/lv/style.hpp)).
   This confirms the owner's decision that styles take design tokens only.
   Keep it.

7. **Never share one `user_data` slot.**
   Rust keeps one closure for each widget, and the animation trampoline reads the same slot.
   A CI run of [#205](https://github.com/lvgl/lv_binding_rust/pull/205) failed on it on 2026-01-07.
   lvgl26's `on<Code>` keeps state in the event's own `user_data` and accepts stateless or pointer-sized handlers.
   Any other capture stops with a message that says how to fix it ([`include/lv/view.hpp`](../../include/lv/view.hpp)).
   `tests/l1_views.cpp` registers handlers of four kinds on one button, and `tests/fail/big_capture.cpp` guards the message.
   Keep both.

8. **Read the headers with the compiler that builds them.**
   Rust's bindgen (clang) and `cc` (gcc) disagreed about the size of an enum on ARM.
   The result was a hard fault: [#163](https://github.com/lvgl/lv_binding_rust/issues/163) and [#186](https://github.com/lvgl/lv_binding_rust/issues/186) report it.
   We reflect with the GCC that compiles LVGL, so this bug class cannot arise between our reading and our objects.
   It does not touch the decision "no real board, stay with the emulator".
   The Arm job builds LVGL and lvgl26 with one `arm-none-eabi` GCC ([`memory.md`](memory.md)).
   Keep the build free of a second reader of the headers.
   The editor stubs come from the reference data, not from the headers.

9. **Start LVGL by object, and say what happens when the order is wrong.**
   Rust's [#69](https://github.com/lvgl/lv_binding_rust/issues/69) was undefined behaviour before `lv_init`.
   The fix was `ctor`, which runs before `main` and needs an `unsafe` opt-out on bare metal.
   `lv::application` starts and stops LVGL by object and has `pre(!lv_is_initialized())` ([`include/lv/application.hpp`](../../include/lv/application.hpp)).
   Its header says to declare the application first.
   Types already cover most of the rest: a screen comes from `lv::display`, which takes the application.
   I did not look for a path around that, such as `raw()`, so confirm it before relying on it.
   A lifetime rule belongs in types, not in a contract, so I propose no `pre` on `view::create`.
   Do not copy the `no_mangle` libc functions ([#180](https://github.com/lvgl/lv_binding_rust/issues/180)).
   A binding must not offer its own `strcmp` to the whole process.

10. **Compile every example, and test what users failed at.**
    Rust's CI skipped its examples.
    They broke when the API moved: [#146](https://github.com/lvgl/lv_binding_rust/issues/146) and the pull requests #203 to #205 show it.
    We build the files behind our snippets and compare our widget examples with LVGL's.
    So we already do better.
    Take the tasks that Rust users failed at as cases for the small eval set that [`agent-docs.md`](agent-docs.md) advises.
    They are a grid layout with arrays ([#105](https://github.com/lvgl/lv_binding_rust/issues/105), [#107](https://github.com/lvgl/lv_binding_rust/issues/107)), the text of a label ([#162](https://github.com/lvgl/lv_binding_rust/issues/162)) and a table with column widths ([#122](https://github.com/lvgl/lv_binding_rust/issues/122)).
    The only AI mention in the Rust tracker is a user whose Copilot code did not compile.

11. **One person who can merge is the real risk.**
    The Rust binding stalled on people as much as on design.
    The people who built it left.
    In [#185](https://github.com/lvgl/lv_binding_rust/issues/185) an LVGL maintainer notes that one approval is required, and nobody can approve their own pull request.
    Today the owner merges every pull request.
    **For the owner:** before the first outside contributor arrives, name a second person with merge rights.
    This is a project decision, not a design one.
    I propose nothing beyond raising it.
