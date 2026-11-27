# lvgl26

A C++26 framework over LVGL. Namespace `lv`. GCC 16.2 or newer only: the code uses reflection.

## Build and test

```sh
cmake --preset host-v96 && cmake --build --preset host-v96 && ctest --preset host-v96
```

| Preset | What it checks |
|---|---|
| `host-v96`, `host-v10` | LVGL v9.6.0 and the v10 pin, warnings as errors |
| `host-v96-asan`, `host-v10-asan` | AddressSanitizer and UndefinedBehaviorSanitizer, with `float-cast-overflow`, which GCC leaves out of `undefined` |
| `host-v96-tsan` | ThreadSanitizer (Linux only; GCC has no TSan on macOS) |
| `host-v96-analyze` | GCC static analyzer (`-fanalyzer`) |
| `host-v96-coverage` | Coverage; report with `gcovr --filter include/lv/ build/host-v96-coverage` |

Every change must pass `host-v96` and `host-v10` locally. Run the tests side by side:
`ctest --preset host-v96 -j 8`.

| Where | What runs |
|---|---|
| Every pull request and push to main (`ci.yml`) | `host-v96`, `host-v10`, `host-v96-asan`, `host-v96-analyze`, the compile-fail tests on both LVGL versions and with the sanitizers' flags, ThreadSanitizer on the threaded tests, the editor stubs read by Clang, CRAP, one minute of fuzzing |
| Every night (`nightly.yml`) | `host-v10-asan`, `host-v96-tsan` on every test, half an hour of fuzzing, the slice on an Arm Cortex-M4 in QEMU (`tools/arm-run.sh`), the interactive example for WebAssembly (the experimental browser target, below), the compile cost of the probe files (below), mutation testing of the lines of `include/lv` changed since the last clean night (below), clangd's quick fix for a missing header (`editor_quick_fix`, ctest label `nightly`). A failure opens an issue |

- The analyzer build compiles three files at once (`LVGL26_ANALYZE_JOBS` in `cmake/quality.cmake`): a test file needs up to 2.4 GB under the analyzer, and four at once lost the CI runner. Raise it only with a measurement of peak memory.
- The analyzer (`host-v96-analyze`) runs on the tests, not on a port: `lvgl26_skip_analyzer(<dir>)` in `cmake/quality.cmake` leaves the targets of a directory out. It triples the compile time of a port, and the job is near its limit.
- A check belongs in the pull request run if it takes a few minutes and a change can
  plausibly break it. Anything long goes to the nightly run.
- A test must be safe to run next to every other test: no shared file, no fixed port.
- The compile-fail tests have the ctest label `compile-fail`. They need no build:
  `ctest --preset host-v96 -L compile-fail -j 8` after configuring. The other tests are
  `-LE compile-fail`.
- The CI jobs run in the image of `.github/ci-image/Dockerfile`. Add a tool there, not as an
  install step in a workflow.
- The Arm preset builds with `-fno-declone-ctor-dtor` (`cmake/arm/toolchain.cmake`): `-Os` and `-Oz` turn on `-fdeclone-ctor-dtor`, and GCC 16.2 then rejects valid consteval code at random (GCC bug 128870, fix aimed at 16.3). `tools/arm-run.sh` compiles the reproducer `tools/arm_gcc_128870.cpp` 20 times. Remove both with GCC 16.3.
- A change to `site/` or to a Markdown file alone does not start the pull request run.
- A token file is written in the object forms of the Design Tokens Format 2025.10: a colour is `{"hex": ..., "colorSpace": "srgb", "components": [...]}` (`hex` first, because the reader takes it first and skipping the rest costs compile time), a length `{"value": 8, "unit": "px"}`, a duration `{"value": 200, "unit": "ms"}`. `tools/dtcg_fix.py` rewrites an older file and `tools/signal_tokens.py` writes Signal's files in these forms. `lv::tokens<doc>` refuses the older forms (`"#3366ff"`, `"8px"`, a bare number) with a message that names the token, unless `lv::config::tokens_legacy_values` is set, which is only for a project's files while they convert. A compile-fail test that reads an older form on purpose includes `tests/fail/legacy_tokens.hpp` and names `legacy_tokens` as the settings. The library's own token files name their own settings tag (`signal_settings`), so a project's `lv::cfg` cannot change how they are read.
- Every token file is checked against the official Design Tokens Format schemas 2025.10 (`tools/check_tokens.py`, ctest `tokens_check`). The schemas are copies in `tools/dtcg/`. A file that does not conform is listed in `tools/token_exceptions.json` with counts per violation kind: a new violation fails, and so does a listed one that no longer occurs. Write the list again with `--update` after a file converges, never to hide a new violation.
- The CI jobs compile through ccache (`.github/actions/setup`). Each build step prints its
  hit rate: an unchanged file must be a hit.
- Module scanning is off (`CMAKE_CXX_SCAN_FOR_MODULES` in `CMakeLists.txt`): ccache refuses a
  compile that has the module flags. A target with C++ modules turns it on for itself.
- ccache before 4.14 needs `CCACHE_NODIRECT=1`: its direct mode does not see a changed
  `#embed` file.

## Rules

- Both LVGL versions must work. Use only LVGL's public `include/lvgl` API.
- Mistakes should be compile errors with a message that says how to fix them. Add a
  compile-fail test in `tests/fail/` for every new one.
- The library throws nothing: compile error first, then contracts, then `std::expected`.
- No `throw`, `try` or `typeid` in the headers, also not in consteval code: the library must
  build with `-fno-exceptions -fno-rtti` (`tests/no_exceptions.cpp`). A compile-time error with
  a computed message is `detail::fail(message)`.
- A program starts LVGL with `lv::application` and gets its screen from an `lv::display`. Examples,
  the README, the site and the docs call no `lv_init`, `lv_deinit`, `lv_display_*`, `lv_tick_*` or
  `lv_timer_handler`. The tests of the library may. `raw()` is the one door to the C API.
- An LVGL enumerator is written as a name in quotes (`.align = "center"`). An example names no
  `lv_` function, `lv_..._t` type or `LV_` name: `tools/check_examples.py` fails on one. A line
  that needs the C API on purpose ends with the comment `// C API`.
- Something follows an `lv::state` or an `lv::computed` through a node in its list (`detail::watch_node`, `include/lv/detail/watch.hpp`): a widget, an `lv::computed`, an `lv::effect`, `lv::changed`. Do not add an LVGL observer for it: the LVGL subject exists only for `raw()`. LVGL's own bind function, and with it a subject, is kept only where `keeps_lvgl_binder` in `include/lv/props.hpp` says so (an animation LVGL starts after the first draw, and `checked`). A walk over a list is safe against watchers that remove or add watchers, and the tests in `tests/watchers.cpp` say how.
- Views never delete widgets. Only `lv::mount` and `lv::owned` own.
- The format of UI code is `.clang-format` (two spaces, one line of at most 100 columns) and `tools/format_ui.py`, which runs clang-format 21 to 23 and then three passes that change white space only: `.style = {` stays on one line, and a product such as `240 * px` keeps its spaces. A property list that ends with a comma is laid out one property per line, and one without it stays on one line while it fits; a handler of one statement stays on the line of its property. The ctest `editor_format_examples` fails when an example would change, `editor_format_ui` pins the lever for the three versions, and `tools/call_form.py` converts the `[...]` form of old code. Run `python3 tools/format_ui.py examples` after editing an example, then `python3 tools/docs.py sync`. A screen is built from named pieces (rules `expressive-6` and `expressive-7`).
- Children follow the properties as arguments: `lv::group({...}, child, child)`, `lv::group(child, child)`, `lv::each(rows, &row::id, {...}, row)` (`lv::each(rows, &row::id, row)` when it has no options; one template for every form, because three overloads cost 0.19 percent of a real file), `lv::shown(state, arms...)`, `lv::when(flag, tree)`. Examples, the README and the docs use this form; `[...]` builds the same node and keeps compiling. Every factory's call is one of two templates of `detail::takes_children`, a class that is no template, with an explicit object parameter: a member template of each factory was declared again for each of the 16 factories, and each extra candidate cost every node of a file (Valgrind `Ir`). So a child that is no child is a `static_assert` in the body, not a deleted overload. A call form builds its node in one step, never through `[]`: one more call copies the node once more in constant evaluation. Both share `detail::check_children`.
- Properties are written in four groups, each alphabetical: the `id`, the properties (`size`, `text`, `value`, ...), the styles (`style`, `style_pressed`, ...), the handlers (`on_clicked`, ...). A group follows from the name alone (`property_group` in `include/lv/props.hpp`, the one comparison `property_order` that sorts the base and merges a widget's own setters; `tools/editor/reference_stubs.py` takes the same key). GCC refuses a list out of order and names one field (test `props_wrong_order`), and the message cannot be changed: it comes before any code of ours runs. `python3 tools/order_properties.py <files>` moves the properties of a list (it skips a list with a comment between them and says so); examples, docs and the README are written in this order. `each_options`, `shown_options`, `arm_options`, a theme and `lv::style` follow no group: their members stay alphabetical.
- A node and a run-time `view.set` call the setters in the order of the props, except where LVGL needs another: `include/lv/detail/setter_order.hpp` is the one table (a property runs `first` or `last` of its widget, with a one-line reason beside it). It holds a rule for a real case only, with a run-time test in `tests/setter_order.cpp` that reads what LVGL did, and the tree is written as before. `set_members_of` in `tree.hpp` and `apply_set_props` in `props.hpp` take the order from it; a widget with no rule (`has_setter_rule`) pays one compare of each rule, once.
- A change of syntax or form updates `README.md` in the same pull request. Its snippets follow their files through `tools/docs.py sync`, and `readme_check` (`tools/check_readme.py`) refuses an old form, a C name, a broken link, and a version, count or build line that differs from the code. Before a commit or a pull request that changes `include/lv/`, `examples/` or `docs/docs.json` without the README, a hook in `.claude/settings.json` (`tools/hooks/readme_reminder.py`) reminds the agent. It never blocks, and runs git only inside `$CLAUDE_PROJECT_DIR`, with `core.fsmonitor`, hooks, external diffs and the system configuration turned off.
- A swipe and a hold (`include/lv/gesture.hpp`, opt-in) are waits and handlers over LVGL's own gestures. LVGL sends `LV_EVENT_GESTURE` while the finger is down, once for each press, to the pressed widget if that does not pass gestures on and otherwise to its first ancestor that does not; every widget passes them on, so a swipe that reaches no such widget is lost. `lv::swiped` and `.on_swiped` therefore make their widget keep gestures (`lv_obj_set_gesture_bubble(false)`), the nearest waiter wins, and `.gesture_bubble = false` on a widget with no wait turns the swipes off for its subtree. LVGL sends no swipe once a scroll has started, and a widget that is a scroll container in an axis takes a drag in that axis as a scroll even without overflow (elastic): a page needs `.scroll_dir = "none"` and a list `"ver"` for sideways swipes. `lv::hold` is built on `LV_EVENT_PRESSED` and one `lv_anim_t`, not on LVGL's long press (one time for the device, no progress); a scroll takes the pressed state off the widget, which ends the hold. The thresholds are `lv::gesture_limits`, given to `lv::pointer` (LVGL's defaults: 50 px, 3 px a read of 33 ms, 10 px, 400 ms); a token group `gesture` is such a struct. `tests/gesture.cpp` drives a pointer read by read and says each rule.
- Input comes through an object tied to a display, as the display is tied to the application: `lv::pointer` (`include/lv/pointer.hpp`, not part of `<lv/lv.hpp>`). The platform pushes samples into a queue inside it (`detail::input_queue`, which a keypad and an encoder will use too), and each sample makes LVGL read at its next step, so no press is lost between two reads. An example's platform is a file of its own that feeds `lv::display` and `lv::pointer` (`examples/platform/sdl_window.hpp`); the application knows no platform, and no example calls LVGL's input or SDL driver. A platform object joins the application's loop with a member `lv::in_loop` (polled before each step, the first that can wait waits), so every window program ends with `return app.run();`; only a host that owns the loop steps (the browser page, `examples/wasm/platform.cpp`). `lv::display` carries a `visible` state that the platform writes; while it is false LVGL draws nothing (`set_visible` draws what was marked, then turns marking off). A task that belongs to a screen is spawned with its root as owner; one without an owner belongs to the whole program and says so in a comment.
- What a factory starts a widget with (`lv::group` fills its parent, `lv::row` and `lv::column` lay out) is one constant style, `start_style` in `style.hpp`, added right after the widget is made and before its own style. LVGL's cascade decides against it, so no code predicts what a look or a style sets. A new default of a factory goes into that style, never into a local property. It is no local style, and `lv_obj_remove_style_all` removes it as LVGL's own theme styles: anything of ours that removes all styles of a widget (a theme swap in place) adds it back first.
- What a tree keeps at run time lives inside its `lv::mounted`, which is never copied or moved (`docs/research/node-storage.md`). A node kind that keeps data declares `template <const auto& Tree, std::size_t... Path> struct storage`, checked by `detail::storage_kind`, and finds it with `detail::storage_of<Tree, Path...>(st)` in its `build`. The tree's storage is made from these by reflection, value-initialised before the widgets are built and destroyed while they all still exist: a storage destructor never deletes a widget. lvgl26 allocates nothing for it. A mounted tree ends in one place (`end()`, from the root's delete handler), whoever deletes the root: `unmount()`, the destructor, the parent or C code. Afterwards the object is not mounted and `mount(parent)` builds it again in the same memory. `unmount()` is for the life of a root (a window that closes); pages inside a tree are switched by `lv::shown`, so examples and docs show no pager made of `unmount()` and `mount()`. `lv::each` builds its rows outside it, so a row template holds no such node yet.
- `lv/one_of.hpp` is not part of `<lv/lv.hpp>`: a program that uses a slot includes it, because it cost every other file 0.12 G instructions. One of several objects in one place is `detail::one_of_region` (`include/lv/detail/one_of_region.hpp`): the largest size and alignment, one object alive, built in place, never moved. `lv::one_of` keeps its trees there; `lv::shown` keeps its pages in slots of the largest page (below). `lv::one_of::visit` chooses an arm by its parameter type (`detail::arm_pattern`). An arm gets an `lv::held<T>`, never a reference to the tree: a handle that checks at each use that the slot holds T, so an arm may remount its slot; `slot.get<T>()` gives the same handle. A handle must not outlive its slot; its arms stay unconstrained by a concept, so that the computed messages that list the slot's trees are what the user sees. The result is the `std::common_type` of the arms' results, as `std::visit` allows, or the `R` of `visit<R>`, to which every result converts (`detail::one_result`); arms without one are a compile error that names the types. A tree listed twice in one slot is a compile error.
- `lv::shown(state, {.alive, .enter, .leave})[lv::is<v>(tree), ..., lv::otherwise()]` (`include/lv/shown.hpp`, opt-in) is a storage kind. Its storage is a `detail::shown_core` (its place in the state's watcher list and in the queue of waiting work, the holder, the glass, `.alive` slots) and the room for the slots. An arm shows a tree of the same file, held by pointer, or an `lv::page` of another file; both are run through one table (`lv::page`: size, alignment and the functions that build and end an `lv::mounted` in a slot), so the engine is no template. A slot has the size of the largest page: with every tree visible the compiler sizes the slots in the storage; with a page of another file the node takes one block of two slots from LVGL's heap at mount, and the mount fails with `out_of_memory` without it. The user writes no size: `extern const lv::page home_page;` and `constinit const lv::page home_page = lv::page_of<ui>;` (a page's hooks are named as an arm's: `lv::page_of<ui, {.on_shown = &start, .on_front = &look}>`, the arm's options as a template argument). `.on_shown` runs when a page is built; `.on_front` each time it comes to the front, built or kept, after `.on_shown` on a build, and when a leaving page turns round. Without a transition the old page is deleted before the new one is built; with `.enter` or `.leave` (`lv::transition`, the arguments of `lv::animate` for the page's root) the new page is built in the free slot first, a page asked for again while it leaves turns round where it stands, a change during an enter makes the entering page leave, and a third page cuts the one that leaves. A transition runs through one step of ours (`transition_step`, which records where it stands) and is found by a plain step that does nothing (`transition_tag`): LVGL finds an animation by its widget and its plain step. The direction of a change is the order of the state's enumerators (`enumerator_index`, `include/lv/detail/enumerators.hpp`; the core keeps the rank of the value shown last and `back` for the change that runs): a change to an earlier value plays `.enter_back` and `.leave_back`, which are `.enter` and `.leave` when left out (`back_or` in the plan), so a program with no back transitions is unchanged; a bool state has no direction. A drag that follows the finger will pick the transition of the neighbour's direction from the same four specs. A leaving page takes no clicks: a transparent widget, the glass, lies over it. `.alive` is the number of slots, 2 by default; above 2 the pages shown most recently stay built and hidden (`slot_phase::kept`), a kept page comes back without being built again (`.on_front` runs, `.on_shown` does not), and the kept page at the front longest ago goes when a slot is needed. A kept page comes back before the old page is kept, so that it cannot push itself out. An arm's tree is a constexpr variable of its own; a tree made in the arm is a compile error. The arms are the patterns of `lv::match` with a call operator, and `lv::match` refuses a value of another enum. `lv::shown` is the only child of its widget (`detail::sole_child`). A switch waits for the start of the display's next refresh (`detail::deferred`, drained by the handler that `lv::display` adds, and by `lv::settle()`). The storage's destructor lets the pages go without deleting their widgets (`page::abandon`): LVGL deletes them with the holder right after.
- Styles take design tokens only. Add the token to the token file; use `lv::raw(...)` only for a
  reasoned exception. Use semantic or component tokens in UI code, not primitives.
- A look shared by all widgets of a kind is a theme (`lv::theme`, `include/lv/theme.hpp`), given to
  the display. A theme's styles are built in constant evaluation with `std::define_static_array`,
  so they need no template argument and no heap. A style for one part in one state is `lv::on(...)`
  merged into `.style` (also in a theme): a constant `lv_style_t` the style keeps in its last member,
  `layers`. Do not add a style member per part and state. Every style spells its shorthands out with one function, `with_long_forms` in `style.hpp` (a theme, `lv::on`, a node's `.style`, a look): a long form beats a shorthand, and a narrower shorthand (`pad_hor`) a wider one (`pad_all`), in any order. Do not expand shorthands anywhere else. `include/lv/signal/*.json` is written by
  `tools/signal_tokens.py` from `design/signal/tokens.design.json`: change the design file, then
  run the tool.
- An element names its look; the token file says how it looks. `lv::look<"queue.card">` (`include/lv/look.hpp`) is a group of the project's tokens as a constant style, by entry name (a style property, with its token type checked), so a style written out entry by entry in a tree is a look. The token file stays plain Design Tokens Format 2025.10, which design tools open: no custom `$type` and no bare custom key. What the standard has no type for (an LVGL value by name, a percentage) goes in the group's `$extensions` under `io.github.filipsajdak.lvgl26` (`vendor_key` in `tokens.hpp`, the one place it is written). A color written `#rrggbbaa` sets the opacity of its property too (`bg_color` with `bg_opa`). The project names its token file once: `lv::design<lv::override>` specialized with `lv::design_of`, like `lv::cfg`; a part library passes the design on as a template parameter. A group nested in a look is a layer (an LVGL part or state) and nothing else; any other nested group is a compile error; `main` and `default` add no selector, so their entries join the group they are in (`order_rank` in `look.hpp`), where a factory's default sees them. A variant is a sibling group, `tile-print` beside `tile` (a nested or self-extending group is circular in the standard), stacked as `lv::look<"tile tile-print">` or named by `lv::look_of<"tile", E>`. `$extends` is not read yet: a look of a group that has it is a compile error, until the reader implements the standard. The node's own properties beat a look (LVGL's local styles). A shorthand entry (`pad_all`) is stored as its long forms. A new mistake in a look gets a message that lists what is valid and a compile-fail test `tests/fail/look_*.cpp`; `tools/look_groups.py` lists groups that no look names.
- `<lv/lv.hpp>` is the core: application, display, tree, view, state, style, tokens, task, animate, computed, match, options, owned. A feature that costs every file compile time has a header of its own, which a program includes when it uses the feature: theme, look, font, scaled_font, image, encoded_image, list, repeat, runtime, one_of, shown, pointer. `<lv/all.hpp>` includes the core and those; Signal, `host.hpp`, `ui_scheduler.hpp`, `mcp.hpp` and `png.hpp` are included one by one. No core header includes an opt-in header. Which header declares each public name is `docs/reference/headers.json`, printed by reflection (`tools/reference/headers.cpp`, test `headers_data`); `tools/check_includes.py` reads it. An example includes exactly the headers it uses (`tools/check_examples.py`; the quick start includes `<lv/all.hpp>`, as the code on the docs pages does), and every test and fuzz file includes the header of each name it uses (test `includes_check`). Every target that links lvgl26 through our CMake checks its includes at build time, with no line of its own (`cmake/check_includes.cmake`, option `LVGL26_CHECK_INCLUDES`, default on): a call deferred to the top directory finds the targets, each source file is checked by a command of its own with a stamp and a depfile, and with Ninja only the link waits for it. The path to the script is in the cache, because the deferred call runs in the program's top directory, not in lvgl26's. `lvgl26_editor_support()` puts a file that includes `lv/all.hpp` into the compile database and the stub directory on clangd's search path, so that clangd's quick fix names the header. The table of `docs/which-header.md` is written by `python3 tools/check_includes.py --page docs/which-header.md` (test `which_header_page`). A template or a function of an opt-in header is declared in the core too (`include/lv/detail/missing.hpp`), as a fallback whose message names the header; the header specializes `detail::included<"lv/x.hpp">` to true and defines the real template as a specialization that requires it, so the fallback is reached only without the header. A function's fallback requires `detail::not_read<"lv/x.hpp", its parameters...>`, so it never takes a call that the real function refuses with a message of its own; the condition names the call's parameters so that it is checked at the call, not where the fallback is declared (Clang then refuses the header's specialization of `included`). A constraint that the fallback and the real template share is a concept (`detail::tree_nodes`, `detail::pointer_capacity`): written twice it is two constraints, and the specialization would not be the more constrained one. Every opt-in header includes `detail/missing.hpp` first, and every fallback has a compile-fail test `tests/fail/missing_header_*.cpp`. The editor stubs repeat the fallbacks (`tools/editor/stubs/lv/detail/missing.hpp`, test `editor_missing_header`); there a function's fallback returns `stub::anything`, so that the deleted call is the only mark on its line. A style reaches a font that follows an `lv::scaled_font` through `detail::font_resolver`, the font's first member, so that the core needs nothing of `scaled_font.hpp`.
- A file that includes only `<lv/lv.hpp>` builds under a fixed operation limit (`lvgl26_include_only_ops` in `tests/CMakeLists.txt`, `tests/include_only.cpp`): its largest evaluation must stay small, and a header that scans LVGL's names or the global namespace where it is read breaks the build. `tools/check_eager.py` finds the source patterns. GCC 16.2 works out a constant expression in the body of a function with no template parameter where the function is read: the range of a `for` loop, or the initializer of a local variable, that calls a consteval function with constant arguments. So `detail::api(name)` takes the name it searches for, unread, and `detail::api_table<D>` is reached through `std::meta::substitute`; `tests/fail/eager_scan.cpp` is the shape that broke.
- A consteval function with no template parameter whose body calls a scan with constant arguments (`std::meta::members_of(^^::, ...)`) runs the scan where the header is read, in every file: `enum_of_property` in `look.hpp` cost 0.8 G instructions in a file that names no look until the scope became a parameter. Measure a new header's include cost against main with a file that only includes `<lv/lv.hpp>` (`tools/compile_report.py` takes the flags of a file of the build; copy a command for a new one), not through a test that already uses styles.
- `lv/signal.hpp` draws no font and reads no typography token. Signal's typefaces, their tokens (`include/lv/signal/type.json`) and the themes with fonts are in `lv/signal_type.hpp`. Both headers build their themes from the one description in `lv/signal.hpp`. Measure a change to `lv/signal.hpp` against main in instructions: it must not cost more. For that reason a colour named `lamp-...` in the design file is not in `dark.json` and `day.json` but in `include/lv/signal/lamps.json`, which only `lv/signal_lamps.hpp` embeds (`tools/signal_tokens.py` knows it by the prefix). So is the motion section, durations named `duration-...` and easings named `ease-...`: it is in `include/lv/signal/motion.json`, which only `lv/signal_motion.hpp` embeds.
- `design/signal/tokens.design.json` is the copy of Signal's design system in Claude Design, and `design/signal/source.json` records its version and hash. Change the copy, run `python3 tools/signal_sync.py --record`, then `tools/signal_tokens.py`: the tests `signal_sync_check` and `signal_tokens_check` fail otherwise. The check sees no change made in the design tool; the skill `skills/sync-signal` compares with it and writes back, and writing back needs the person's yes.
- A header embeds only a file that lies next to it. `include/lv/signal/fonts/` holds copies of the files in `site/media/fonts/`, and the tests `signal_type_file_*` fail when a copy differs.
- An array that a header embeds is `constexpr`, not `inline constexpr`. Under a sanitizer GCC 16.2 does not know that the address of an inline variable is not null. Reading its text in constant evaluation then fails to compile.
- A font that several files use is drawn in one: `lv::font_of<"text.title">` (`include/lv/look.hpp`) is the address of `lv::font_object<"text.title">`, an extern variable template with no definition in the header, so a file that names it draws nothing (the address of an object is a constant, in a tree and in a theme, also under the sanitizers; `examples/fonts_split` and the test `fonts_split` are the proof). The name is checked against the token file by `detail::typography_at`, a compile error with the names there, about 0.08 G for each distinct name in a file. The file `font_<name>.cpp` embeds the TTF and defines it with an explicit specialization, `template <> const lv::drawn_font lv::font_object<"text.title"> = lv::draw<"text.title">(type);`, and the design header it includes holds no typeface (a typeface in the design would embed the bytes in every file). A file defines a font before it names it: after, GCC stops with "specialization after instantiation". `cmake/check_fonts.cmake` and `tools/check_fonts.py` fail an executable that names a font no file defines, before the link (`LVGL26_CHECK_FONTS`, `LVGL26_FONTS` on a library target). The in-place `type(token)` stays for one-file programs. The linked programs of `examples/fonts` and `examples/fonts_split` have equal sections at `-Os` and the same picture.
- A display follows a state: `display.follow(mode, {day, dark})` (`include/lv/display.hpp`, `docs/research/theme-follow.md`, `tests/theme_follow.cpp`). The themes are indexed by the enumerators of the state's enum in order; they have one shape (same widget class and selector in each position: `detail::same_shape`, a contract in `follow()`, since comparing the classes in constant evaluation is not reliable under the sanitizers). The display holds one `detail::watch_node` on the state's list (`follower_`), and a change runs `switch_to(index)`: `detail::restyle` walks the active screen and the three layers and puts each style of the new theme where the old one stood with `lv_obj_replace_style`, which keeps the position in the widget's list (a look or a node style added after the theme still wins) and allocates nothing (the first change of a drawn screen makes a few things of LVGL's own, so the test measures the heap after one pair of switches); the lv_theme's user data is swapped for widgets made later. Only what a theme gives follows: a node's `.style` with a token and a look are constant. A group holds no theme style. The theme's first entry list is `current_`; a display made with a theme of the same shape follows from it.
- `lv::animate` moves local style values too (`include/lv/animate.hpp`, `tests/animate_style.cpp`, `docs/research/animate-style.md`). `animatable<W>` lists the widget's setters of one number, then the style properties of one number that no such setter has the name of (`.width` is the widget's); `animatable<"">` is the struct of `lv::in<"part", "state">(view)`, the style properties alone. A style step is `style_step<setter, selector>`: LVGL identifies an animation by the widget and the step function, so one instantiation for each property and selector gives two parts side by side and a newer animation of one selector replacing the older; the plain call and `lv::in<"main">` are the selector 0. The first step sets `from` at once, the local value stays, and it beats a theme, a look and `.style`. The ends of an opacity are `0 * pct`, not integers. The scan of the 139 style members is behind a template parameter, as `animatable_of` is.
- Signal's fonts have two forms that a program takes one of: `lv/signal_type.hpp` draws them in each file that includes it, and `lv/signal_fonts.hpp` names them as `lv::font_of<"text.body", lv::signal::type_design>`, objects that the optional library `lvgl26::signal_type` (`lib/signal_type`, one file per font, `LVGL26_SIGNAL_TYPE`, off by default and on for the tests) defines. `lv::font_object<Name, D>` carries the design as `lv::design<D>` does, so Signal's names are not the project's. The design of Signal's fonts is in `lv/signal_type_design.hpp` and nothing there names a font: a file that defines a font defines it before any namespace-scope constant names it, which `lv/signal_fonts.hpp` does for `body_font` and `label_font`. The first of the two headers included stops the second with a message (`signal_fonts_and_type`, `signal_type_and_fonts`). `tests/signal_linked.cpp` is `tests/signal_type.cpp` with the linked fonts and the same reference pictures.
- A font in a style comes from a typography token through `lv::typeface`; `lv::raw(font)` marks one
  that no token names. `include/lv/detail/coverage.hpp` is derived from font-rs and stays under the
  Apache License 2.0: keep its notice, and list any other borrowed part in `NOTICE.md`.
- A part of LVGL that costs the device code or heap is behind a CMake option of its own, off by
  default (`LVGL26_TTF`, `LVGL26_PNG`, `LVGL26_JPEG`). So is leaving something out: `LVGL26_NO_DEFAULT_THEME`
  drops LVGL's own theme (`host-v96-no-theme` tests it). The tests build with them. Its C functions are found by reflection
  (`detail::find`), and a `static_assert` names the option when they are missing.
- lvgl26 sets no `-fconstexpr-ops-limit`: GCC's default of 33,554,432 operations covers what a user
  writes. The limit counts one constant evaluation, so work that needs more is split into evaluations
  of its own: a font draws each glyph, each row of kerning pairs and its bitmap in variable templates
  (`glyph`, `kerning_row`, `bitmap` in `include/lv/font.hpp`), reached with `std::meta::substitute`.
  The tests `font`, `font_bytes` and `signal_type` compile with no option, and are the guard. A program
  of the repository that needs more gets a fixed budget of its own, at about twice its need
  (`large_tree`, `lv_reference` in `tests/CMakeLists.txt`). A single loop in constant evaluation must
  still stay under 262144 rounds.
- Code that runs in the compiler is written for its interpreter: look codes up in tables, give a
  buffer its size once and write through pointers, avoid a call for each byte
  (`include/lv/detail/png_reader.hpp` needs an eighth of the work of its first form).
- In constant evaluation GCC 16.2 charges about 0.7 M instructions for the first `std::meta::identifier_of` of a declaration and about 0.37 M for each `string_view::starts_with`. A scan of the global namespace asks the line first (`detail::declared_in_a_file`: GCC's builtins, half of the functions there, are at line 0 or 1), and a loop over names tests a prefix with `detail::has_prefix` (`include/lv/detail/api.hpp`). `tests/api_table.cpp` checks that the line filter loses no `lv_` function.
- In consteval code a `std::string_view` is never made from a `const char*` alone: that constructor counts the characters in a loop, about 25,000 instructions each. A literal compared with a view is written `"..."sv` (`std::string_view_literals` is used in `lv::detail`), and a name kept as `const char*` travels with its length. `identifier_of` pays this loop inside GCC on every call, so a scan asks it once per declaration.
- A large buffer in constant evaluation is not made with `std::vector<T>(n)`, `resize` or `push_back`: libstdc++ initialises each element through calls, about 75,000 instructions an element. `new T[n]()` costs a quarter of that. `inflate` in `include/lv/detail/png_reader.hpp` still returns a vector; its cost there is not measured yet.
- GCC 16.2 caches no call that reaches a metafunction, so the same `substitute`, `members_of` or scan costs as much the second time. Its result is kept in a variable template (`std::define_static_array`), and the table is read.
- Names are sorted with `detail::sort_by_name`, a stable merge sort of indices: the cost is the comparisons, and an insertion sort did n * n / 4 of them. Two tables made from one scan read one static table (`theme_table` in `theme.hpp`, `member_table` in `props.hpp`), never the scan function twice.
- A large result of constant evaluation is named by what made it, never by its value. `std::define_static_array` makes an object whose bytes are part of its mangled name, six characters for each byte, and Apple's linker refuses a name of more than 1,048,575 characters. So the pixels of a picture are a `std::array` in a variable template named by the file (`stored_pixels` in `include/lv/image.hpp`), and so is a font's bitmap (`bitmap` in `font.hpp`). Each is filled through a pointer, from rows or glyphs that a variable template of its own holds, so the file is decoded once. `std::define_static_array` stays for small constants, such as the descriptor that points at the pixels. A call with constant arguments inside a template runs twice: each step takes its arguments as parameters of a consteval function, and the variable template only calls it. `tests/image_symbols.cpp` and `tools/check_symbols.py` keep every symbol of a large picture's object file under a budget, on every platform.
- `docs/reference/lvgl-<version>.json` is the reference as data: widgets, properties, what each takes,
  enumerator names, style members. `tools/reference/reference.cpp` prints it by reflection, and the
  test `reference_data` fails when a committed file is stale. Write it again with the program the
  test names; never edit it by hand.
- Work that goes on while a part of the screen exists is started with `lv::spawn(task, owner)`: it
  ends when `owner` is deleted. A task without an owner lives until it returns, or until a widget it
  has waited on is deleted.
- No macros and no `#if` in our code. Settings go in `lv::config` (see `include/lv/config.hpp`);
  optional LVGL features are detected with `requires` or reflection. `#embed`, `#include` and
  `#pragma once` are the only preprocessor lines we write.
- A program's settings are one JSON file, named by `lvgl26_settings(target file)` (`cmake/settings.cmake`) and read into `lv::config` by reflection (`include/lv/detail/settings.hpp`). The library reads them as `detail::project<O>`, through a template parameter. A setting is one of two kinds. A restriction only forbids: it changes no generated code (`settings_zero_cost` compares the objects), and each of its messages gets a compile-fail test; there may be many. A variant changes what is generated (a layout, code that always runs): both of its states keep passing the tests, and it needs a measured reason; there are few. `allow` is permitted by default, `rules` is strict by default, and a limit is a value and a unit.
- A part that needs a feature of `allow` calls `detail::require<^^allow_settings::threads, O, "lv::ui_scheduler">()` in the body of a function template, so the check fires where the part is used, not where its header is read. Every key has an `lv::described` annotation: `tools/reference/settings.cpp` prints `docs/reference/settings.json` and its JSON Schema, and the tests `settings_reference` and `settings_schema` fail when either is stale.
- `include/lv/config.hpp` is included first into every file of a program that has a settings file, so it holds types and nothing that costs the compiler work. The reader returns its first mistake as text for a `static_assert`, which GCC prints once; a failed constant evaluation it prints three times.
- Application values are strongly typed: a measure is an mp-units quantity (`120 * km / h`), a
  count is `lv::counter`, anything else a one-member struct. Do not add a unit system to lvgl26.
- No handwritten per-property code. Properties, styles and events come from reflection.
- A unit that LVGL documents only in a comment (degrees or tenths of a degree) is read from the
  embedded LVGL headers (`include/lv/detail/angle_docs.hpp`). A hand-written list of setters is not
  allowed. A new header with an angle goes into `angle_headers`; `tests/angle_units.cpp` checks the
  reading against the pixels.
- A list of options for a dropdown or a roller is written `.options = {"a", "b"}` or
  `lv::options_of<E>`, not as a text with line breaks: examples and docs use the list. Which setters take a
  list is found by name (`include/lv/detail/option_docs.hpp`), never listed by hand, and `tests/options_docs.cpp`
  checks the names against LVGL's doc comments: the library does not read the headers, because an `#embed`
  and a search in every file cost 0.2% of the compile time of `tests/large_tree.cpp`. A state of an enum follows `.selected` through
  `lv::options_of<E>` in a constexpr tree only (the observer's data is in read-only memory); at run time
  `set()` refuses it with a contract. `lv::but` lives in `include/lv/but.hpp`, one definition for every
  user of it.
- Anything a header does at namespace scope costs compile time in every file. Keep reflection
  scans lazy: put them behind a template parameter (see `props_of` in `include/lv/props.hpp`).
- Inside a template, name what is generated for one widget through a template parameter
  (`members_of_props<"obj", D>`, `getter_table<"obj", D>`), never plainly. A name that depends on
  nothing is generated where the template is read: in every file that includes the header.
  A state type reads `lv::cfg`, so a class template names it through its parameter
  (`count_state` in `include/lv/list.hpp`); otherwise a project's settings come too late.
- Under a sanitizer GCC 16.2 does not compare two different function addresses, nor the address of
  an inline variable or a template parameter object with null, in a constant expression. Carry a
  number instead (`follows` of `lv::arg` says what a property follows), or compare
  `std::meta::reflect_constant` of both (see `detail::is_null` in `include/lv/values.hpp`).
- GCC 16.2 runs a call whose arguments are all constants twice when it is written inside a
  function template: once when the template is instantiated, once when the function runs. Hand
  the result of a scan in as an argument (see `scan_base` in `include/lv/props.hpp`).
- Repetition (`lv::for_each`, `lv::for_each_field`, `include/lv/repeat.hpp`) is expanded by the compiler into the children of the parent: a `detail::repeated` value is flattened by `splice` in `include/lv/tree.hpp`, so the tree is the one written by hand. A part sets no `.id`. A caption is a string literal, since an annotation holds a structural type and GCC 16.2 refuses `std::string_view` there. A record with a text literal is taken by reference (`[]<const auto& R>`), because a literal is no template argument. Clang reads no annotation, so `tests/editor/api_uses.cpp` repeats the members of a row type in a template that is never instantiated.
- A node of a tree keeps `detail::kept<W>`, not `props<W>`. A props struct has thousands of
  members, and the compiler walks every one each time it copies a node.
- A run-time `view.set({...})` takes `detail::set_props<W>`: the members of `props<W>`, with each of the 16 style slots a `detail::style_in_set` (one name, in `props.hpp`) that points at a style living until the end of the call. A whole style in each slot made one `set` 45 KB of stack. The properties are written in place: a named variable and an inline style (`{.radius = ...}` instead of `lv::style({...})`) are compile errors. One `set` must stay within 4 KB of stack on the Cortex-M4 (`SET_STACK_BUDGET` in `tools/stack_budget.py`, run by `tools/arm-run.sh` every night on `examples/mcu/set_probe.cpp`).
- `lv::arg` and what it holds have no default member initializer, so that an unset property is
  all zeros. A constructor to run there is paid for every unset member of every node.
- `tests/large_tree.cpp` is compiled under a fixed `-fconstexpr-ops-limit`. It stops compiling
  when a node becomes expensive for the compiler again. Raise the limit only with a measurement.
- A header that every file includes pays for what it declares: measure `tests/large_tree.cpp` with
  `tools/compile_report.py`, at least five runs alternating with main, and compare the minimum and the median.
  A `constexpr` variable or a search that is evaluated where it is written is paid by every file.
- Measure before claiming: compile time with `/usr/bin/time`, heap with `lv_mem_monitor`,
  pixels with `lv_test_screenshot_compare`.
- Compare compile times in instructions, not in seconds: on a busy machine the same compile takes
  several times longer. `/usr/bin/time -l` prints "instructions retired", but for the process it
  starts, so start `cc1plus` itself: `g++ -###` prints its command.
  `tools/compile_report.py <build dir> <file>...` does this and prints one JSON row per file (see `docs/research/compile-time.md`).

## The nightly compile cost

`tools/metrics/probes.json` lists the probes of the nightly run; a new probe is one line there (`docs/research/compile-time.md`, "The nightly measure"). Each is a file, a run-time probe or a stack size.

- The number is Valgrind's instruction count of one `cc1plus` (`tools/compile_cost.py`): the hosted runners have no hardware counter, and this count is the same on every run. `tools/compile_report.py` on a Mac counts another thing: bisect with it, compare trends, not numbers. Peak memory is never a threshold.
- Two rules judge the cost, with their numbers in `tools/metrics/probes.json` (a probe may name its own). **A change**, per probe: up to +0.25 percent (`warn_percent`) it merges with no comment; from there up to +1.0 percent (`block_percent`) it merges only when the pull request carries the cost table and a one-line reason; above +1.0 percent it waits until it is fixed or the owner accepts it (`python3 tools/cost_history.py judge-pr --base <rows> --head <rows>` prints the table and exits 1 for a block). **The drift**, every night: a probe fails the job and opens the issue of the night when it is more than +2.0 percent (`drift_percent`) above its baseline, and stays a failure until it is back under +1.0 percent (`clear_percent`). The baseline moves only when a probe is new, got cheaper than the baseline by more than the warn limit, or the owner accepts a rise (below); it never creeps up, so small rises add up until they cross the drift limit. Only a run on `main` writes the history on the branch `metrics`. A probe runs under 90 minutes: callgrind is 50 to 60 times slower than the compile.
- A rise that the owner accepted is recorded with its reason and pull request, never allowed as a standing margin: run the nightly on main by hand with `cost_only=true`, `accept_cost=<probe>` and `accept_reason=<why, PR>`. The baseline of that probe moves to the value of the run and the night's line of the history carries the reason (`--accept` of `tools/cost_history.py`; the history only grows, and the limits do not change).

## The experimental browser target

`examples/wasm/` is an interactive application built for WebAssembly by GCC trunk with a third-party back end (`docs/research/web-target.md`). It is a nightly check and not a supported target. The pull request run builds and runs its host form (`wasm_host_counter`, `wasm_host_bounce` in `tests/CMakeLists.txt`); only the WebAssembly build covers the page (`tools/wasm/page`), the exports of `tools/wasm/link-demo.sh` and the module in a browser.

- The compiler is built by `.github/ci-image/wasm/Dockerfile` from commits and hashes that it names. Nothing of the compiler or of the back end is in this repository. Move a pin only with a measurement: the build of the image and the nightly run.
- `-O2` only: GCC fails at `-Os` with "taking address of an immediate function" (`std::optional` of a type with a `consteval` constructor).
- A task hands control to `detail::idle()`, never to `std::noop_coroutine()`: libstdc++'s frame holds functions of the wrong type, and WebAssembly traps on the call (GCC bug 128871). The nightly job `web` puts the unpatched header back before it builds. `tools/wasm/fix-coroutine.sh` leaves with the next rebuild of the WebAssembly image.
- `app.hpp` is written with lvgl26 only. `platform.cpp` holds what every host needs: a clock, an `lv::pointer` fed with each event of the page as it comes (`web_pointer`), with no hold of a short press, a frame, one step. The page and the host program call the same functions, and the desktop window (`examples/window/main.cpp`) runs the same `app.hpp`.
- The host build of the example and the WebAssembly build replay `script.json`: the last frame must be equal byte for byte. Change the script and the page together, never the expected hash alone.
- The module is a reactor. GCC ignores `export_name`, so the exports are linker options in `tools/wasm/link-demo.sh`: add a function to the list there.

## Test adequacy

| Check | How | Limit |
|---|---|---|
| Complexity | `lizard`, through `tools/crap.py` | 8 for every function, also compile-time ones |
| CRAP | `tools/crap.py build/host-v96-coverage` after the coverage preset | 8 for every function that runs at run time |
| Mutation | The full run of the changed lines is nightly: one shard for each changed header (`tools/mutation_plan.py`, the tests of each in `tools/mutation_tests.json`). Locally `tools/mutation.py <header> --tests '<ctest -R>' --since origin/main --sample 40` is enough for a quick look | Every survivor gets a test, or a reason in `tools/mutation_accepted.json` |
| Fuzzing | `fuzz/json_reader.cpp` (libFuzzer, Clang); `fuzz/runtime_document.cpp`, `fuzz/mcp_message.cpp` and `fuzz/mcp_stream.cpp` (AFL++ with its GCC plugin: `fuzz/afl.sh`) | Every free input has a target; every crash becomes a corpus file |

- Mutation is nightly: the run takes the lines of `include/lv` changed since the tag `nightly-mutation-base` (moved by the run after a clean night), and opens an issue with the survivors. Run it by hand with `since=<ref>` (workflow_dispatch) to cover an older change. An MR description says `Mutation: nightly, see run <url>`, or the line with counts when they are known.
- Every header of `include/lv` has an entry in `tools/mutation_tests.json` with the tests that judge its mutants (`tools/test_mutation.py` fails for a header without one): a header that falls back to all tests judges about six mutants an hour. A new header gets an entry in the same pull request: the tests that include it or are named after it, and a small screen (`smoke`, `l1_views`) when nothing is.
- `tools/mutation_tests.json` also says, for a header, `"logic": true` (only mutants that swap an operator or a number) and `"parts": n` (n shards that each judge every n-th mutant). `look`, `tree`, `one_of`, `style` and `values` run logic-only: on 2026-11-20 each ran out of its 4.8 hours with 172 to 5844 mutants not judged (326 to 726 logic mutants each, 0.7 to 3.3 hours), and the mutants of messages and deleted statements are what a test of the message or a compile-fail test already judges. `look`, `props` and `tokens` have two parts: props and tokens judge the tests of the headers they feed too (`options`, `look`), which adds 25 to 45 seconds to each mutant that builds.
- `tools/mutation_accepted.json` is one sorted entry per line, so that two branches that add entries merge without a conflict. Add an entry, then run `python3 tools/mutation_accepted.py`; the tool test fails for a file in another form. After a merge conflict in it, take both sides' entries and run the tool.
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
- No target of the repository precompiles a header that includes lvgl26's headers. With GCC 16.2 and trunk a precompiled header silently drops the `pre` and `post` of templates, depending on addresses (GCC bug 128909). The README, the quick start and the status page warn users.
- GCC 16.2 cannot compile `pre` on a function with a deduced return type, or a fold over the
  parameters inside `pre`. Use `contract_assert` as the first statement there, with a comment that
  says so.
- Every new error message gets a compile-fail test in `tests/fail/`.
- GCC 16.2 folds a call with constant arguments without its contracts when the call is part of
  a larger expression. A constexpr function with a contract on its values repeats the condition
  as its first statement with `detail::keep_checked(...)` (see `include/lv/detail/checked.hpp`),
  and gets two compile-fail tests: the call as a whole initializer, and inside an expression.
- To test that a valid value passes a precondition, make the call the whole initializer of a
  `constexpr` variable, before any other use of the same arguments. A `static_assert` does not
  evaluate the precondition.

- A part (a function that returns a piece of tree) constrains its parameters with the public concepts of `docs/concepts.md`: `lv::tree_handler` (a handler in a constexpr tree), `lv::event_handler` (run time), `lv::dependency`, `lv::tree_node`, `lv::view_value`, `lv::style_value`, `lv::font_value`, `lv::token_value`, and `lv::repeat_part` for the part of `lv::for_each`. Never `void (*)()` for a handler, and never a bare `template <class Count>` for a state. A new concept is public (`lv::`, not `lv::detail::`), has `static_assert`s in `tests/concepts.cpp`, a compile-fail test in `tests/fail/part_*.cpp`, and its stub in `tools/editor/stubs/lv/`.
- Clang 21 does not read `template <lv::dependency auto& Count>`: write `template <auto& Count> requires lv::dependency<std::remove_cvref_t<decltype(Count)>>` in code that the editor stubs or `tests/editor/api_uses.cpp` read.
- A handler in a constexpr tree is a function pointer: a function or a lambda without captures. A part that needs state names it as a template argument.

## Definition of done for a feature

A feature pull request also:

- Moves the feature's line from "planned" to "works today" in `site/status.html` (the comment at the top of that file says how).
- Adds the feature to the feature list in `site/index.html`, and gives it a page under `docs/`. `README.md` changes when one of its feature sections changes, and always when the syntax it shows changes; it links to the site's list.
- Checks `README.md` and `site/index.html`: syntax, feature list, requirements, counts (`readme_check`).
- Adds or updates an example, and the site media in `site/media/` when the feature is visual.
- Records any new rule in `CLAUDE.md`.
- Updates `docs/migration/coverage.json` when it changes what a port can use: a row of a toolkit, or a gap that closes. The migration table of the status page is written from it: run `python3 tools/migration_matrix.py`. The tests `migration_matrix` and `migration_tools` fail when the page is out of date.

## Documentation

`docs/` is the one source of the documentation: Markdown, read on GitHub, rendered into `site/docs/` and collected into `site/llms.txt` and `site/llms-full.txt` for agents. `docs/research/` is not part of it.

- `python3 tools/docs.py check` must pass. `python3 tools/docs.py build` renders the pages (needs pandoc); then `python3 tools/check_site.py`.
- A code block that shows a file has the line `<!-- snippet: path name -->` before it and equals that part of the file. A test builds the file. `tools/docs.py sync` rewrites the blocks.
- `README.md` is checked and synced as a page, and is never rendered: every C++ block in it is a snippet of a built file (`tools/docs.py`, `README`). It is a short front door: a hero, three claims, a few features with one snippet each, each linking its page. A feature's details go to its page under `docs/`, not into the README.
- A page names no `lv_` function, `lv_..._t` type or `LV_` name in its code. Only a page about `raw()` may, and it says `<!-- c-api: shown -->`.
- A new page goes into `docs/docs.json`, or it has no place in the sidebar and in `llms.txt`.
- One line per paragraph. A picture on a page is written by a test (see `quick_start_picture`).
- `site/docs/`, `site/llms.txt` and `site/llms-full.txt` are generated: do not commit them.
- The rules of good code are data, `docs/reference/rules.json`; `python3 tools/rules.py --page docs/rules.md` writes the page "Writing good lvgl26 code" (test `rules_page`). An id never changes and is never used again. A rule names a check only where the compiler or the build enforces it in a program, and a rule of run time or compile time names its measurement and date, or says "not measured yet".
- A guide follows the outline of `docs/README.md` once `docs/docs.json` lists it under `outline`; `tools/docs.py check` holds it there. A message on a page is `<!-- message: <test> -->` before a `text` block, and must show what that compile-fail test pins. A guide's rules are a block `<!-- rules: ids -->` that `tools/docs.py sync` writes from the data.
- `docs/research/documentation.md` holds the owner's decisions for the documentation and the plan of every page: read it before writing or rewriting a page.

## Editor stubs

clangd cannot parse the headers, so editors read stubs: the public API as plain C++. `tools/editor/generate.py` writes the generated part from `docs/reference/*.json` and from a project's token files. `tools/editor/stubs/lv/` is the hand-written part. `lvgl26_editor_support()` in `cmake/editor.cmake` runs it for a project. `docs/editor-support.md` is the page for users.

- A change to the public API changes the stubs in the same pull request. Declare a new public name in `tools/editor/stubs`, and use it in `tests/editor/api_uses.cpp`.
- The test `editor_names` lists the public names by reflection (`tools/editor/names.cpp`) and fails for a name that the stubs or `api_uses.cpp` lack. It needs no Clang, so it runs under every preset.
- GCC compiles `tests/editor/api_uses.cpp` with the real headers, and Clang reads it with the stubs (`editor_api_uses`). Write a `static_assert` on a type there where both must agree, as for `root->get<"id">()`.
- The `editor_*` tests that read the stubs need clang++ 21 or newer, and `editor_support` runs clangd when it finds one. `editor_quick_fix` starts clangd with its background index on a project with editor support and checks the quick fix that adds a missing header (`tools/editor/quick_fix.py`); it has the ctest label `nightly`, which the pull request run leaves out. In CI they run in the job `editor`, in an image of its own (`.github/ci-image/editor/Dockerfile`): the main image has no Clang.
- A stub repeats a check only when the editor can show it at the user's line: a `requires` on the call, or a `consteval` constructor. `tests/editor/wrong_*.cpp` and `unknown_member.cpp` hold those cases, in the form of Clang's `-verify`.
- Every function of a template in a stub has a body, `{ __builtin_unreachable(); }` when nothing else fits. Clang warns about a function without one when its arguments have no linkage. The stub tests run with `-Werror`.
- The stubs are read by Clang only, and may use what our own code may not (`__builtin_unreachable`, an include guard in `tools/editor/fallback/contracts`).
- A new member kind or value text in the reference data needs a rule in `tools/editor/reference_stubs.py`: `editor_tools` fails when a value becomes `stub::anything`.
- A token stub holds the value of a length in pixels, a time, a number and a font weight, through its aliases (`initial_value` in `tools/editor/token_stubs.py`), so that code which needs a token's value while it compiles, such as the size of a frame, reads in the editor. A colour, a curve and the composite tokens hold their default: a stub that cannot hold a value exactly holds none.
- `lv::like` is a `stub::member_of_scope`: it converts to the type of any property but not to `lv::arg`, so a property reads it one way. `lv::name_of` is an empty text, a constant as an id or a text made from it must be. `tests/editor/api_uses.cpp` uses both in properties.
- A file under `examples/` that needs a build setting the editor check lacks, such as the SDL driver, says `// editor-check: skip, <reason>` in its first ten lines. The check prints it as skipped.

## Research

`docs/research/` holds what has already been looked up, with sources. Its `README.md` lists the topics. Some are about the platform: LVGL upstream, C++26, a browser target, fonts, images, editor support, components and lists. The others are the examples of other frameworks, device UI standards, the instrument cluster, 3D and effects, and documentation for agents.

- Read `docs/research/README.md` and the topic file before a web search.
- Search only for what the notes do not answer, or for a claim marked `summary` or `recalled` that the work depends on.
- Add every new finding to its topic file in the same change, with its link, its mark and the date.
- A decision made from research goes into the code, the design system or this file. The notes hold the evidence, not the rule.
- A hook in `.claude/settings.json` repeats the first three rules before every `WebSearch` and `WebFetch`.

## The plugin

The repository is a Claude Code plugin: `.claude-plugin/` holds the manifests and `skills/` the skills.

- A skill points to files of the repository with relative links. It never copies their content.
- `tools/check_plugin.py` fails when a skill links to a file that does not exist. Run it after moving a file a skill names.

## Known tool limits

- clang-tidy cannot analyze this code: Clang 23 has no reflection and no `<meta>` header.
  GCC's warnings, analyzer and sanitizers stand in for it.
- Reference screenshots are only written with `-DLVGL26_UPDATE_REFERENCES=ON`.

## Git

Feature branches, atomic conventional commits, ASCII only in commit messages.
