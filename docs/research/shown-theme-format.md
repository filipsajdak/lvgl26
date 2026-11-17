# Shown trees, looks that change, and formatted text

Research and design for four planned features, written 2026-11-17.

Marks follow `docs/research/README.md`.
Every mark is dated 2026-11-17 unless its row says otherwise.

- `source`: LVGL's code read in the local trees, or our own code, or a page read by a research agent.
- `measured`: compiled or run in this session.
  The method is under "Measured here".
- `summary` and `recalled`: as the README says.

LVGL v9.6.0 was read in `build/_src/lvgl-v9.6.0`.
The v10 pin is commit `53819fe` (`CMakePresets.json`), read in `build/host-v10/_deps/lvgl-src`.
The sketches live outside the repository, in a scratch directory; this note quotes what they showed.

The four features:

- F1, `lv::shown`: a node that follows a state and shows the tree of the current value.
  The owner chose this shape.
- F2: a look that changes at run time, and a style value that follows a state.
- F3: text made from several values, written into a fixed buffer, with no heap on a change.
- F4: a handler whose parameters the library reads by reflection and fills from the event, so that it stays without captures.
  The owner added it during the work.

## Short answers

| Question | Answer | Evidence |
|---|---|---|
| Can `lv::shown(state)[lv::is<v>(tree), ...]` be written in GCC 16.2? | Yes, with arms as constexpr trees and exhaustiveness by reflection | Sketch 1 |
| Where does a shown node keep its state? | In one block on LVGL's heap, freed with the holder, as `lv::each` does | Sketch 1, `include/lv/list.hpp` |
| What does a switch cost? | The widgets of the new arm; the heap stops moving after the first 100 round trips | Sketch 1 |
| May the state change inside a click of the arm being replaced? | LVGL's code handles it; a plain handler ran correctly on v9.6; coroutines are not checked | Sketch 1, `lv_event.c` |
| Can a theme change at run time? | Yes: swap each theme style in place with `lv_obj_replace_style` | Sketch 2 |
| Can a node's own style follow a mode? | Yes: one tree written for a token set, built once per mode at compile time, swapped in place | Sketch 2 |
| Can `lv::bind("{}% - {}", progress, word)` be checked at compile time? | Yes, by our own consteval parser; `<format>` costs too much | Sketches 3 and 4 |
| Can a bound text change without the heap? | Yes: a block per label, sized at compile time, shown as static text | Sketch 3 |
| Can a handler's parameters, types and names, be read and filled from an event? | Yes, for a lambda without captures and for a function | Sketch 5 |
| Can the handler's type be found again from one pointer kept in a constant tree? | Yes, with `std::meta::reflect_function`, also under a sanitizer | Sketch 5 |

## Measured here

GCC 16.2 on macOS arm64, LVGL v9.6.0 built with AddressSanitizer and UndefinedBehaviorSanitizer (`build/host-v96-asan`).
The sketches used the `host-v96` warning flags with `-Werror`.
LVGL's own heap is its built-in TLSF pool, one static array, so ASan does not see a read of a freed widget.
ASan covers our code and the C library, not LVGL's widgets.
Heap is `lv_mem_monitor`, `total_size - free_size`.
Pointers are 8 bytes here, so a Cortex-M4 needs about two thirds of each heap figure.

| What | Result | How |
|---|---|---|
| Sketch 1: `proto::shown(active_page)[proto::is<page::home>(home_ui), ..., proto::is<page::ink>()]` | Compiles, mounts, switches, shows an empty arm | ASan build, run |
| Heap of the shown node with an empty arm | 160 bytes, with the `spec_attr` that the holder's first event adds | Host alone, then host with the node |
| Heap of the home arm (column, label, button) | 1,040 bytes | Before and after a switch to the empty arm |
| Heap before, after 100, after 1,100 round trips | 12,440, 12,544 and 12,544 bytes: a step of 104 bytes, not explained, then flat | Two loops of switches |
| Home arm shown again after the empty arm | 16 bytes less than the first time, not explained | `lv_mem_monitor` |
| A real click on a button of the arm, whose handler switches the page | The page switches; the button is deleted in its own click; the program goes on correctly | `lv_test_mouse_click_at`; ASan cannot see LVGL's pool |
| An id inside an arm | `lv::row_view<home_ui>{root}.get<"title">()` reads it | Sketch 1 |
| Missing enumerator | `static assertion failed: lv::shown does not cover: ink. Add an arm for each; lv::is<value>() shows nothing for it.` | `-fsyntax-only` |
| Duplicate arm | `static assertion failed: lv::shown has two arms for home. Each value needs one; one arm may name several: lv::is<a, b>(tree).` | same |
| Arm of the wrong type | `static assertion failed: lv::shown follows a state of page, and an arm names a value of look. Name a value of the state's type: lv::is<page::...>.` | same |
| `lv::bind(&song::title)` inside a shown arm | Compiles, and would never be applied | same |
| `lv::match(p)(lv::is<look::light>.then(...), ...)` on a `page` | Compiles today, and `page::home` takes the arm of `look::light` | `static_assert(pick(page::home) == 0)` passes |
| Sketch 2: `same_shape(lv::signal::dark, lv::signal::day)` at compile time | True: same widgets and selectors, entry by entry | `static_assert` |
| Swapping Signal dark for day on live widgets | Button, card and label take the day colors | ASan build, run |
| Heap of a theme swap | 80 bytes until the next refresh, then none; 50 round trips: no growth | `lv_mem_monitor` around the swaps |
| `card<dark_tokens>` and `card<day_tokens>` | One type: a shape check is `std::same_as` | `static_assert` |
| Sketch 3: two labels, `"{}% - {}"` and `"{:02}:{:02}"`, two states each | "63% - printing", "00:07"; 512 bytes for both, made once | ASan build, run |
| 200 changes of the states | No heap | `lv_mem_monitor` |
| LVGL's `lv_label_bind_text` with one value, for comparison | 112 bytes, plus a text block freed and allocated on every change | same; `lv_label.c` |
| More `{}` than values | `constexpr message: lv: the text format has more {} than values.` | `-fsyntax-only` |
| A `const char*` value without a bound | `constexpr message: lv: a text value has no bound on its length. Give it one: {:.12} shows at most 12 characters.` | same |
| A consteval `bind(format, states...)` that keeps one pointer to a constant descriptor | Compiles; the descriptor needs `std::array`, not `std::tuple` (not structural) | same, no sanitizer |
| Sketch 4: parsing `lv/lv.hpp` with and without `<format>` | 33.78 G and 35.29 G instructions: `<format>` adds 1.5 G to every file | `cc1plus` under `/usr/bin/time -l`, twice each |
| `<format>` alone, and an empty file | 4.72 G and 0.24 G instructions | same |
| Cortex-M4, `-Os`, a program that writes "63% - PRINTING" | `std::format_to_n` 346,718 bytes of text; `snprintf` 5,492; written by hand 3,147 | `arm-none-eabi-size`, repository sysroot |
| `std::format_to_n` on that sysroot | Does not link: it needs `swprintf`, which the C library lacks; a stub was added to measure | link error |
| Sketch 5: the parameters of `[](std::int32_t value, lv::view<"label"> readout, lv::event e)` | Types and the names `value`, `readout`, `e` | `parameters_of`, `identifier_of`, ASan build, run |
| The same of a plain function `reset_speed(lv::view<"slider"> self, lv::view<"label"> readout)` | Types and the names `self`, `readout` | same |
| A function kept as a pointer in a constant, `reflect_function(*pointer)` | The function, with its parameter names | same |
| A lambda converted to a function pointer, the same way | Its types, but no names: the pointer leads to the lambda's invoker | same |
| A generic lambda `[](auto value)` | Its `operator()` is a function template: no parameter list to read | `is_function_template` |
| A default argument, `int step = 5` | Seen: `has_default_argument` is true | same |
| An unnamed parameter | Seen: `has_identifier` is false | same |
| A handler kept as one pointer to `trampoline<F>`, in a `constexpr` object | `template_arguments_of(reflect_function(*h.fire))[0]` is `F` again | `static_assert` |
| A concept "captureless, and every parameter can be built" | Holds for the good lambda; fails for a `double` and for an unnamed view | `static_assert` |
| The trampoline on a slider, `LV_EVENT_VALUE_CHANGED` | The label shows "42 (code 39)": value, widget by name, event | run |
| The plain function's body, through an object with the same `operator()` | The slider resets, the label shows "reset" | run |

## What the code has today

| Finding | Where | Mark |
|---|---|---|
| `lv::each` keeps its view in one `lv_malloc` block, freed on the holder's `LV_EVENT_DELETE`. It is not in `mounted` | `each_view::start` in `include/lv/list.hpp` | `source` |
| `mounted` is movable and `mount()` returns it by value. A buffer inside it moves | `include/lv/tree.hpp` | `source` |
| `lv::place` and `detach()` hand the widgets to the parent; `mounted` then dies first | same | `source` |
| `lv::arg` holds `follows` and three pointers: `bound`, `bound_subject`, `bound_format`. A new binding must fit them | `include/lv/values.hpp` | `source` |
| A widget holds one structural kid at most; the message names `lv::each` | `node::operator[]` in `include/lv/tree.hpp` | `source` |
| Ids below a structural node are not collected, so `root->get<"id">()` on them is a compile error | `collect` in `include/lv/tree.hpp` | `source` |
| `structural_above` is true for any structural node, so a row field binding passes in any of them | `apply_node` in `include/lv/tree.hpp`; Sketch 1 | `measured` |
| `tree_child_of` assumes each tree kid is one LVGL child after LVGL's own children | `include/lv/list.hpp` | `source` |
| `lv::match` casts each pattern value to the subject's type, so a value of another enum passes | `add_values` and `matches` in `include/lv/match.hpp` | `measured` |
| A text binding calls LVGL's `lv_label_bind_text` with a printf format of one `%d` | `bind_member` in `include/lv/props.hpp` | `source` |
| A row field with a format is written into a stack buffer and copied by `lv_label_set_text` | `apply_field` in `include/lv/list.hpp` | `source` |
| Constant text uses `lv_label_set_text_static`, except in long mode "dots" | `set_property` in `include/lv/props.hpp` | `source` |
| `lv::computed` keeps no value; it computes on `get()` | `include/lv/computed.hpp` | `source` |
| A state may hold a `const char*`; the treadmill points three of them at global buffers | `include/lv/state.hpp`, `examples/ports/qt/treadmill/logic.cpp` | `source` |
| A tree's constant style is one object per value, made with `std::define_static_object` | `shared_const_style` in `include/lv/style.hpp` | `source` |
| A theme is a null-ended array of `{widget class, selector, style}` in read-only memory | `include/lv/detail/theme_styles.hpp` | `source` |
| Signal already has two modes: one design file with a value per mode, written into `dark.json` and `day.json` | `design/signal/tokens.design.json`, `tools/signal_tokens.py` | `source` |
| `lv::signal::dark` and `lv::signal::day` are one description, `themed<Tok>()`, built for two token sets | `include/lv/signal.hpp` | `source` |
| `obj_bind_style.cpp` says "A style cannot follow a state here" and uses `.checked` with `.style_checked` instead | `examples/lvgl/widgets/obj_bind_style.cpp` | `source` |

## Findings in LVGL

| Finding | Link | Mark |
|---|---|---|
| `lv_theme_apply(obj)` calls `lv_obj_remove_style_all(obj)` first, in v9.6 and the v10 pin. It would wipe a node's own styles | [lv_theme.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/themes/lv_theme.c) | `source` |
| `lv_obj_replace_style` swaps a style in the widget's list in place, keeps its selector, and refreshes. It allocates nothing | [lv_obj_style.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/core/lv_obj_style.c) | `source` |
| `lv_obj_replace_style` returns false when the old and new style are the same pointer | same | `source` |
| `lv_obj_enable_style_refresh(false)` stops refreshes; `lv_obj_report_style_change(NULL)` refreshes every screen once | same | `source` |
| `lv_obj_set_style_enabled` turns a style of the list off and on, in both versions | same | `source` |
| `lv_obj_bind_style(obj, style, selector, subject, ref)` adds the style and enables it while the int subject equals `ref`. Same code in both versions | same | `source` |
| It costs one `lv_malloc` of a small record and one observer per widget and style | same | `source` |
| `lv_obj_bind_style_prop` sets a local style property from an int, color or pointer subject, in both versions | same | `source` |
| `lv_obj_bind_flag_if_*` and `lv_obj_bind_state_if_*` exist in v9.6 and not in the v10 pin. v10 has `lv_obj_bind_bool` and the like | `include/lvgl/core/lv_observer.h` of each tree | `source` |
| `lv_image_bind_src` exists in both versions | `include/lvgl/widgets/lv_image.h` | `source` |
| `lv_label_bind_text` with a format calls `lv_label_set_text_fmt`; without one, `lv_label_set_text`. Both copy into the label's heap | [lv_label.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/widgets/label/lv_label.c) | `source` |
| A string subject writes into buffers its owner gives (`lv_subject_set_string_buffer_static`), in both versions | `include/lvgl/core/lv_observer.h` | `source` |
| `lv_async_call` allocates a record and creates a timer for each call | [lv_async.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/misc/lv_async.c) | `source` |
| Deleting a widget marks the events in flight for it (`lv_event_mark_deleted`); the input device stops when `send_event` returns invalid | [lv_event.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/misc/lv_event.c), [lv_indev.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/indev/lv_indev.c) | `source` |
| `lv_obj_delete` resets the input devices that point at the widget (`lv_indev_reset`) | [lv_obj_tree.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/core/lv_obj_tree.c) | `source` |

## Other frameworks

Rows from `docs/research/components.md` are not repeated.
These answer the three needs directly.

| Finding | Link | Mark |
|---|---|---|
| Qt Quick `Loader`: when `source` or `sourceComponent` changes, the previous item is destroyed. The page does not say whether before the new one is made | [Loader](https://doc.qt.io/qt-6/qml-qtquick-loader.html) | `source` |
| Qt Quick `Loader`: `active: false` releases the item; `asynchronous` builds it over several frames | same | `source` |
| Qt `StackView`: the outgoing item is hidden when the exit transition ends; items it created itself are destroyed by it | [StackView](https://doc.qt.io/qt-6/qml-qtquick-controls-stackview.html) | `source` |
| Qt for MCUs: `Loader` takes its items from a separate allocator | `docs/research/components.md` | `source` 2026-11-16 |
| Slint: `if cond : Element {}` instantiates the element only while the condition holds. Destruction time and exit animation: not documented | [Repetition](https://docs.slint.dev/latest/docs/slint/guide/language/coding/repetition-and-data-models/) | `source` |
| Slint: the `Palette` global has an in-out `color-scheme` (`unknown`, `dark`, `light`) that a program may set | [Palette](https://docs.slint.dev/latest/docs/slint/reference/std-widgets/globals/palette/) | `source` |
| Slint: `@tr("{}% - {}", a, b)` formats several values; `{0}` and `{1}` pick by position; more placeholders than values is an error | [Translations](https://docs.slint.dev/latest/docs/slint/guide/development/translations/) | `source` |
| Compose `AnimatedContent`: the target is drawn over the initial content; `KeepUntilTransitionsFinished` keeps the old one until both end | [Animation composables](https://developer.android.com/develop/ui/compose/animation/composables-modifiers) | `source` |
| SwiftUI: a `.task` is cancelled when its view is removed | [task](https://developer.apple.com/documentation/swiftui/view/task(name:priority:file:line:_:).md) | `summary` |
| SwiftUI keeps a removed view until its removal transition ends | none found | `recalled` |
| DTCG 2025.10: modes are not a token feature. A separate resolver document has `sets` and `modifiers`; a modifier maps `contexts` such as light and dark to files | [Resolver](https://www.designtokens.org/tr/2025.10/resolver/) | `source`, read through a fetch tool |
| Figma variables: a collection has `modes`; each variable has `valuesByMode`, from mode id to value | [Variables endpoints](https://developers.figma.com/docs/rest-api/variables-endpoints/) | `source` |
| `std::format_string` checks the format at compile time through a consteval constructor | [basic_format_string](https://en.cppreference.com/w/cpp/utility/format/basic_format_string) | `source` |
| With `-fno-exceptions`, libstdc++ replaces its throws by calls that abort | [libstdc++ manual](https://gcc.gnu.org/onlinedocs/libstdc++/manual/using_exceptions.html) | `source` |

What the frameworks agree on.

- One of several trees is a library element with its own lifetime: Loader, StackView, Slint's `if`, Compose's `AnimatedContent`.
- Only a transition keeps the old tree alive; without one, it goes at once.
- A look that changes is a global that every style reads (Slint's palette, Figma's modes), not a rebuilt screen.
- Text with several values is formatted by the framework, with positional placeholders.

## F1. Show one of several trees

### The alternatives

A1: A structural node in the constant tree, with its arms as constexpr trees.

```cpp
constexpr auto window = lv::group({.id = "main"})[
    lv::shown(active_page)[
        lv::is<page::home>(home_ui),
        lv::is<page::settings>(settings_ui),
        lv::is<page::ink>()]];  // shows nothing
```

A2: The same node, with an arm that calls a page function from another file.

```cpp
lv::shown(active_page)[lv::is<page::home>(&home_page), lv::is<page::settings>(settings_ui), ...]
// home_page.cpp:  lv::obj home_page(lv::obj parent) { return lv::place<home_ui>(parent); }
```

A3: Build every arm at mount and hide all but one.

```cpp
lv::group({.hidden = lv::bind(not_home)})[home_ui], lv::group({.hidden = lv::bind(not_settings)})[settings_ui]
```

| | A1 trees | A2 page functions | A3 all built, hidden |
|---|---|---|---|
| Checked at compile time | Values, arms, ids, bindings | Values and arms; the page is checked in its own file | Only the bindings |
| Heap | The shown arm, plus about 160 bytes | Same | Every arm, all the time |
| Allocations per switch | The new arm's widgets | Same | None |
| Compile cost | Every arm is code in the file that mounts | Each page in its own file, built in parallel | Every arm, in one file |
| Widgets keep their state over a switch | No | No | Yes |

Recommended: A1, with A2 accepted as an arm.

- A1 is the owner's sketch, and the only form that checks ids and bindings across the arms.
- A2 matters for compile time: the two ports cost 657 s of CPU in 16 files (`docs/research/compile-time.md`).
  One file holding every page would serialise that.
- A3 is what the printer's side bar does with six computeds; it is right for small parts, not for pages.

### How the recommended node works

Names.

- `lv::shown(state)[arms]` follows a state, as the owner wrote.
  `lv::when(flag)[tree]` is short for `lv::shown(flag)[lv::is<true>(tree), lv::is<false>()]`.
- An arm is `lv::is<values...>(tree)`, `lv::is<values...>()` or `lv::otherwise(tree)`.
  These are the patterns of `lv::match` with a call operator added.
  They do not clash.
- `lv::is<a, b>(tree)` gives one tree to two values.
  A change between `a` and `b` keeps the tree.

Exhaustiveness and errors.

- The check reuses `match.hpp`: `all_values`, `missing_values` and `value_text`.
  Sketch 1 printed the three messages under "Measured here".
- An arm of another enum must be refused.
  `lv::match` accepts one today, which is a bug of its own: fix it in the same change.
- A type that cannot be listed (a named value, an integer quantity) needs `lv::otherwise`.
  An `otherwise` after full coverage is an error, as in `lv::match`.
- The state may be an `lv::state` or an `lv::computed` of an enum or a bool.
  `optional` and `variant` cannot be states today, so they are out.

Storage.

- The node keeps one block on LVGL's heap: the kind table, the holder, the current root and the current arm.
  It holds an observer on the state's subject.
- The block is freed on the holder's `LV_EVENT_DELETE`, as `each_view` is.
  So it works under `lv::place`, in a row of `lv::each` and in another arm.
- A slot in `mounted` would need `mounted` to stop moving: an open choice of `docs/research/memory.md`.
  The block needs nothing.
- Sketch 1 measured 160 bytes for it on a 64-bit host, with the holder's `spec_attr`.
  Its size on a Cortex-M4 is not measured; the rule of thumb of this note, two thirds, gives about 100.

Order of a switch.

- Delete before build: the old root is deleted, then the new arm built.
  The peak is the larger arm, never both.
- Sketch 1 ran 2,200 switches: the heap rose by 104 bytes in the first 100 round trips, then stayed flat.
- A switch from inside a click of the deleted arm ran correctly.
  The evidence that it is safe is LVGL's code: `lv_event_mark_deleted` and the input device's early return.
- The switch runs inside the state change, so two changes in one handler build twice.
  Deferring would merge them, at the cost of `lv_async_call`'s allocation per switch.

The holder.

- `lv::shown` is the only tree kid of its widget, refused at compile time otherwise: "give lv::shown a widget of its own".
- Then nothing shifts: `tree_child_of`, `child_on_path` and `each_view::first_` keep their meaning for every other widget.
- Siblings would need the number of widgets that each structural node shows.
  That is bookkeeping for little gain: the ports already use a host group.

Animation.

- `.enter` alone keeps delete before build: the new root animates in.
- `.leave` makes the old root outlive the switch.
  The block keeps it as "leaving", and a task tied to it deletes it at the end.
- If the state changes again mid-leave, the leaving root is deleted at once, and the current one starts to leave.
  So at most two arms exist.
- If the holder is deleted mid-animation, LVGL deletes both roots with it; the tasks end with their widgets, and the block is freed.
- This mirrors Compose (`KeepUntilTransitionsFinished`) and StackView, which keep the old item only for the transition.

Ids, tasks and focus.

- Ids inside an arm are not ids of the outer tree; asking for one stays a compile error, with a message that names `.on_shown`.
- Each arm takes `{.on_shown = &start}`: a function called with a view of the arm's root, after it is built.
- The view is `lv::row_view<home_ui>`, which reached the id in Sketch 1.
  It needs a more general name.
- A task started there with `lv::spawn(work, root)` ends when the arm goes.
  Nothing new is needed for that.
- LVGL adds the new widgets at the end of the default group.
  If the old arm held the focus, the node should focus the new arm's first widget.

Cost for the compiler.

- An arm is instantiated only by `build<Tree, Path..., K, 0>`, inside the node's table.
  A tree without `lv::shown` pays nothing.
- `tests/large_tree.cpp` has no structural node, so its cost should not move.
  The slice measures it, as `docs/research/lists.md` did.
- The engine is one class that is no template, as `each_view`.
  Each node adds a table and one build function per arm.

Both LVGL versions.

- The node uses `lv_subject_add_observer_obj`, `lv_obj_add_event_cb`, `lv_obj_delete` and `lv_malloc`: all in v9.6 and the v10 pin.
- Sketch 1 ran on v9.6 only.

### The smallest slice

- `lv::shown` over an enum or bool state, arms as trees or empty, `lv::otherwise`, `.on_shown`, delete before build, no animation.
- The holder rule, the fix of `structural_above`, and the type check of `lv::match`, in commits of their own.
- The printer's main pager moves to it; the sub-page with its slide follows when `.enter` and `.leave` land.

Tests.

- Run time: switch, empty arm, two values on one arm keep the tree, `.on_shown` task ends with its arm.
- Heap: 1,000 switches with `lv_mem_monitor`, then the same number as after 100.
- A real click with `lv_test_mouse_click_at` whose handler switches, and the same from a coroutine.
  ASan sees LVGL's widgets only with LVGL on the C library's `malloc` (`LV_USE_STDLIB_MALLOC` set to `LV_STDLIB_CLIB`).
- Compile-fail: missing value, duplicate, wrong enum, needless `otherwise`, no `otherwise` for a named value, row field in an arm, a sibling of `lv::shown`.
- Compile cost: `tools/compile_report.py` on `tests/large_tree.cpp` before and after.

Risks.

- A coroutine resumed by a click of a widget that the switch deletes.
  Sketch 1 used a plain handler only.
- The `structural_above` hole: without the fix, `lv::bind(&T::field)` in an arm compiles and does nothing.
- Users put whole pages into one file to use A1.
  A2 must be in the first slice, or the ports' compile time grows.

What it removes from the ports.

| File and lines | What goes |
|---|---|
| `examples/ports/slint/printer/pager.hpp:12-55` | `pager`, `slide_pager`, `slide_in`, `slide_out` |
| `examples/ports/slint/printer/shell.cpp:62-100` | The two pagers, `page_maker`, `subpage_maker`, `show_page`, `show_subpage` and two effects |
| `examples/ports/slint/printer/shell.cpp:103-111` | `hide_when_empty`, which waits out the slide by hand |
| `examples/ports/slint/printer/shell.cpp:118-123` | Wiring the pagers to their hosts |
| `examples/ports/qt/treadmill/shell.cpp:17-40` | `hand_over`, `main_maker`, `follow_look` |
| `examples/ports/qt/treadmill/shell.cpp:42-47` | `remove_splash` becomes `lv::when` over a state |

## F2. A look that changes at run time

### The alternatives

B1: Several constant themes on the display, swapped in place.

```cpp
lv::display display{app, frame, lv::themes(active_mode)[lv::is<mode::dark>(lv::signal::dark),
                                                        lv::is<mode::day>(lv::signal::day)]};
```

B2: A tree written for a token set, built once per mode, restyled in place.

```cpp
template <const auto& Tok>
constexpr auto main_ui = lv::group({.style = {.bg_color = Tok.color.background}})[...];

lv::restyle(active_look)[lv::is<look::light>(main_ui<tok.theme.light>),
                         lv::is<look::dark>(main_ui<tok.theme.dark>), ...]
```

B3: LVGL's own binding: one constant style per mode on each widget, enabled by `lv_obj_bind_style`.

B4: Tokens that resolve through the active mode: `tok.color.surface` holds one value per mode.

B5: Rebuild: `lv::shown(active_look)` over three trees.
This is what the treadmill does by hand today.

| | B1 themes | B2 restyle | B3 LVGL bind | B4 mode tokens | B5 rebuild |
|---|---|---|---|---|---|
| In flash | One theme per mode | The styles that differ, per mode | Same as B2 | Same as B2 | Every tree, per mode |
| RAM | None | One block per restyle node | A record and an observer per widget, style and mode | As B2 | The tree |
| A switch | `replace_style` per themed style | `replace_style` per node that differs | One observer call per widget and style | As B2 | Delete and build everything |
| Allocations per switch | None after a refresh | None expected | None | None | The whole tree |
| Widgets keep state, tasks, focus | Yes | Yes | Yes | Yes | No |
| Checked at compile time | Same shape of the themes | Same type of the arms | Nothing | Same token paths per mode | Each tree |
| New code in the library | Small | Medium | Small | Large: tokens and styles | None beyond F1 |

Recommended: B1 for themes, then B2 for a node's own look.
B4 can come later as syntax over B2.

- B1 and B2 keep every style a constant in read-only memory, and swap pointers.
  Sketch 2 did both.
- B3 costs RAM per widget and mode: three looks on 40 styled nodes is thousands of bytes.
- B4 changes what a token is, and every style that reads one.
  Its benefit over B2 is shorter source, not a smaller program.
- B5 needs nothing new, but deletes and builds the screen, with its tasks, animations and focus.

### How B1 and B2 work

B1, themes.

- The themes of one display must have one shape: the same widgets and selectors, entry by entry.
  A consteval check refuses others.
- Signal's `dark` and `day` pass it, because both come from `themed<Tok>()` (Sketch 2).
- A switch turns refreshes off, walks the screen, replaces each style of the old theme by the new one, and refreshes once.
- The display's theme then points at the new entries, so a widget made later gets the new look (Sketch 2).
- `lv_theme_apply` must not be used: it removes every style first, also the node's own.

B2, a node's own look.

- The arms must have one type.
  Sketch 2 showed that one tree for two token sets is one type, so the check is `std::same_as`.
- At compile time the node lists the members whose values differ between arms: styles, `lv::on` layers, and plain properties such as an image.
- A switch walks those paths from the root and swaps each constant style, or calls the setter with the arm's value.
- The table lives in read-only memory.
  The node needs a block and an observer, as `lv::shown` does.
- The treadmill's pictures per look are plain property differences.
  `lv_image_bind_src` would be the alternative, with a state per picture.
- Rows of an `lv::each` inside the node have no fixed path.
  The first slice refuses an `lv::each` whose row template differs between arms.

What a switch costs.

- No allocation: `lv_obj_replace_style` keeps the widget's list entry.
  Sketch 2 saw 80 bytes held until the next refresh.
- One refresh of every widget's style and a redraw of the screen.
  Its time on a Cortex-M4 is not measured.
- Equal styles are one object (`define_static_object`), so a value equal in two modes swaps nothing.

How it composes.

- `lv::on` layers are constant styles with their own selectors: they enter the same table.
- `.style_checked` and the `checked` state still work: the treadmill's active look button needs only `.checked`, not two trees.
- A per-row look, such as the printer's printing card, is already `.checked = lv::bind(&job::printing)` in `lv::each`.

How token files express modes.

- Today: one DTCG file per mode, with the same paths.
  Signal writes two from one design file; the treadmill nests three under `theme`.
- DTCG 2025.10 keeps modes out of the token format: a resolver document maps a modifier's contexts to files.
- Figma keeps one value per mode in `valuesByMode`.
- Recommended: one DTCG file per mode, and a consteval check that every mode has the same token paths and types.
- Reading a resolver document can come later.
  It names the files; `#embed` still has to bring each one.

A style value that follows a state.

- `lv_obj_bind_style_prop` exists in both versions: `.style = {.bg_color = lv::bind(accent)}` could use it.
- It makes a local style on the heap, and it bypasses the token rule.
  It is not needed by either port.

### The smallest slice

- B1: themes chosen by a state on `lv::display`, the shape check, the swap.
  Signal's example switches dark and day.
- B2 follows as its own slice, with the treadmill as its test case.

Tests.

- Run time: colors before and after, a widget made after the swap, a widget with `lv::on` layers.
- Heap: 50 round trips, measured after a refresh.
- Compile-fail: themes of two shapes, a missing mode, arms of B2 with different types.
- A pixel test: `lv_test_screenshot_compare` of Signal in day after a switch from dark.
- Compile cost: `tools/compile_report.py` on `tests/large_tree.cpp`, which B1 should not touch.

Risks.

- The refresh of a large screen on a slow device: one frame may be long.
  Measure in QEMU.
- A user style added with `raw()` after the mount is not in the table and keeps its look.
- B2 tables grow with the tree.
  The compile cost of comparing three arms of the treadmill screen is not measured.

What it removes from the ports.

| File and lines | What goes |
|---|---|
| `examples/ports/qt/treadmill/main_light.cpp`, `main_dark.cpp`, `main_sporty.cpp` | Three files of 19 lines become one tree |
| `examples/ports/qt/treadmill/main_page.hpp:15` | The parameters `Incline`, `Speed` and `Active` |
| `examples/ports/qt/treadmill/parts.hpp:125-138` | Two trees of `look_button`, replaced by `.checked` |
| `examples/ports/qt/treadmill/shell.cpp:19-40` | The rebuild on a look change, also covered by F1 |

## F3. Text from several values in a fixed buffer

### The alternatives

C1: A format with several values in the binding, a buffer per label sized at compile time.

```cpp
lv::label({.text = lv::bind("{}% - {}", progress, word)})
lv::label({.text = lv::bind("{:02}:{:02}", minutes, seconds)})
lv::label({.text = lv::bind(elapsed, lv::formatter<6>(&mm_ss))})  // void mm_ss(std::span<char>, seconds_t)
```

C2: A text value with its own buffer: a state or computed of `lv::text<N>`, shared by every label.

```cpp
constinit lv::computed status{[](lv::counter p, job j) { return lv::text<24>::format("{}% - {}", p, j); }, progress, word};
lv::label({.text = lv::bind(status)})
```

C3: LVGL's string subject: a buffer of ours, filled with `lv_subject_snprintf`.

C4: `std::format_to_n` into a buffer, with `std::format_string` for the check.

| | C1 in the binding | C2 text value | C3 string subject | C4 `<format>` |
|---|---|---|---|---|
| Checked at compile time | Count, types, widths | Same | No: printf | Yes |
| Where the bytes are | One block per label, made at mount | One buffer per text, in `.bss` | Ours, and a copy in each label | Wherever we say |
| Heap per change | None | None | A label text block, freed and allocated | None |
| Code | A few hundred bytes (hand-written: 3,147 for a whole program) | Same | `snprintf`: 5,492 for a whole program | 346,718 bytes |
| Compile cost | Our parser, in consteval | Same | None | 1.5 G instructions in every file |
| Lists | A row keeps today's stack buffer and copy | Not for rows | Not for rows | Same as C1 |

Recommended: C1, with a formatter function for what a format cannot say.

- C1 removes the heap churn where it happens: each change of a bound label.
- Its block is made once, with the label's other memory, and freed with it.
- C2 is better when many labels show one text.
  Neither port has that, so it waits.
- C3 still copies into each label: `lv_label_bind_text` calls `lv_label_set_text`.
- C4 failed on both counts: 347 KB of code on a Cortex-M4, and it did not even link there.

### How C1 works

The format.

- `{}`, `{:02}` (a width, filled with zeros) and `{:.12}` (at most 12 characters).
  Nothing else, so the parser stays small.
- `format_for<value types...>` has a consteval constructor, as `std::format_string` does.
  A wrong count or spec fails with our message.
- The room is computed: 11 characters for a number, the longest enumerator name for an enum, the precision for text.
- A `const char*` without a precision is refused: nothing bounds it.
  `{:.12}` cuts it.
- A quantity shows its number in the state's unit; the format writes the unit, as `"{} km/h"` does today.
- An enum shows its enumerator's name (Sketch 3: "printing").
  The port's "PRINTING" needs a name table or a formatter.

Where the binding fits.

- `lv::bind(format, states...)` is consteval and keeps one pointer to a constant descriptor: the format, the room, the states.
- So `lv::arg` keeps its size.
  A new value of `follows` says what `bound` points to.
- The descriptor must be a structural type: `std::array` of addresses compiled, `std::tuple` did not.

The buffer and the label.

- At mount the binding allocates one block: the room, and a link per state.
  Each state gets an observer on the label.
- A change writes the block and calls `lv_label_set_text_static` with it again.
  LVGL measures the text then.
- A label in long mode "dots" gets `lv_label_set_text` instead, as constant text does today.
- The block is freed on the label's `LV_EVENT_DELETE`.
  It never lives in `mounted`, so `lv::place` is safe.
- Sketch 3: two labels with two states each took 512 bytes once, and 200 changes took none.

A formatter function.

- `lv::formatter<N>(&f)` takes a function `void f(std::span<char, N>, T)` that writes the text.
  N is the room.
- The treadmill's pace clamps to "99:60", and its speed shows tenths.
  A format cannot say either, a function can.
- The function runs with the label's own block; it captures nothing, so the binding stays a constant.

### The smallest slice

- `lv::bind(format, states...)` on labels, with `{}`, `{:02}` and `{:.N}`, for states and computeds of numbers, named values, enums, bools and text.
- The block per label, the static text, the "dots" exception.
- The formatter function and row templates come next.

Tests.

- Run time: the texts of Sketch 3, a negative number with a width, a text cut by its precision, an empty text.
- Heap: 200 changes with `lv_mem_monitor`.
- A label in "dots" mode, cut on screen with `lv_test_screenshot_compare`.
- Compile-fail: too many and too few `{}`, an unknown spec, a text without a bound, a value of a type with no rule.
- `tests/no_exceptions.cpp` binds one format, so the parser stays free of `throw`.
- Compile cost: `tests/large_tree.cpp` under its budget, since F3 adds a value of `follows`.

Risks.

- Two values that change together give two writes: "64% - waiting" may show for one frame between them.
  There is no transaction in LVGL.
- An observer per state per label: a label of three states costs three observers.
- The enumerator name is code-like text.
  Users will want display names, which is a table per enum.

What it removes from the ports.

| File and lines | What goes |
|---|---|
| `examples/ports/qt/treadmill/logic.cpp:117-146` | Three buffers, three `snprintf` functions, three effects |
| `examples/ports/qt/treadmill/logic.cpp:244` | `start_readouts` |
| `examples/ports/qt/treadmill/state.hpp:42-45` | Three states of `const char*` |
| `examples/ports/slint/printer/queue.cpp:59-62` and `queue.hpp:41` | `status_line` with `std::string`, once rows take a format |
| `examples/ports/slint/printer/queue_view.cpp:76-81` and `print_page.cpp:77-82` | Texts set by hand after the mount, once the queue is an `lv::list` |

## F4. Handler arguments injected by reflection

The owner's words: "in .on_clicked when I assign a lambda or function we can inspect arguments, to keep it stateless, and inject whatever it needs".

Today a handler takes nothing or an `lv::event` (`lv::event_handler` in `include/lv/view.hpp`).
`lv::handler` keeps two function pointers, and `add_handler` in `include/lv/props.hpp` passes one of them in `user_data`.
The branch `feat/public-concepts` adds `lv::tree_handler`: convertible to `void (*)()` or `void (*)(event)`.
Anything else a handler needs, it reaches through a global `lv::weak`, a `root->get<"id">()` after the mount, or a task.

### What can be injected

| Parameter | Built from | Verdict |
|---|---|---|
| `lv::event e` | The event | Yes; today's case |
| `lv::view<"slider"> self` | `lv_event_get_current_target_obj`, typed by the node's kind | Yes; the kind is checked at compile time |
| `lv::view<"label"> title` | The widget with the id `title` in the same tree, by a path known at compile time | Yes; the name is the id |
| `std::int32_t value`, or the property's own type | The widget's getter, found by reflection as `sets_and_gets_integer` does | Yes, for value events |
| `const track& row` | The row of `lv::each` that holds the widget | Yes, inside a row template |
| `lv::display` or the screen | `lv_obj_get_display`, `lv_obj_get_screen` | Possible; no port needs it |
| An `lv::state` | By type, or by name | No; see below |

States.

- By type is ambiguous as soon as two states share one, which every port has (`lv::counter`, `bool`).
- By name would need a scan of a namespace for a variable with that name: fragile and costly, and a rename breaks it silently elsewhere.
- A state is a `constinit` global, so a lambda without captures names it directly: `[] { ++clicks; }` captures nothing.
- So no state injection; a part that needs one keeps taking it as a template argument, as the concepts branch does.

### How it works

Reading the handler.

- A lambda's parameters are those of its `operator()`: `parameters_of(^^F::operator())`, with types and names (Sketch 5).
- A function's parameters are read through `reflect_function` of the function, with names (Sketch 5).
- A lambda turned into a function pointer has lost its names.
  So a lambda must be kept by its type, not converted.
- A generic lambda has no list to read.
  It is refused: "write the parameter types".
- A default argument and an unnamed view are seen, and refused: each parameter is filled, and a view needs its name.

Keeping it in the constant tree.

- `lv::handler` keeps one pointer to `trampoline<F>`, the function that builds the arguments and calls `F{}`.
  A lambda without captures is empty, so it is made, not stored.
- `apply_node` reads `F` back from that constant pointer: `template_arguments_of(reflect_function(*h.fire))`.
  Sketch 5 did it, under ASan and UBSan.
- There `Tree` and `Path` are known, so the ids are looked up then, and the trampoline gets their paths as constants.
- A function kept as a pointer is found again the same way, with its names.
  Storing a pointer of any signature in one member is the open part: a static object per function would do, not compiled.
- A plain function cannot go through `trampoline<F>`, and today's two forms stay.
  So the handler holds three pointers, or two and a tag.
  It grows by a pointer in every event member of every node: a cost for `tests/large_tree.cpp`, not measured.

Reaching the widget by name at fire time, without a capture.

- From the handler's widget, go up as many parents as the node is deep, then down the path of the id.
  Both counts are constants.
- No memory is kept: no root in `user_data`, no list of nodes.
  It costs a few pointer reads per event.
- A structural node builds no widget, so the depth counts widgets, not tree levels.
- Inside a row of `lv::each` or an arm of `lv::shown`, the ids are those of the row or arm.
  The outer tree's ids are reached across it.
- The row value of `const track& row` comes from the view's block, found on the holder; its index is the row root's child number minus `first_`.

Names as an API.

- A parameter's name must be `self`, the id of a widget, or nothing for `lv::event` and the value.
  A name that is no id is a compile error that lists the ids, as `get<"id">()` does today.
- No fallback by type when the name fails: a rename then picks a different widget without a word.
  The error is the safe outcome.
- Renaming an id breaks the handlers that name it, at compile time.
  That is the same rule as `get<"id">()`.
- The order of parameters is free; a widget named twice gets two views of it.
- Views by value, a row by `const&`.
  A non-const reference is refused: a row changes through `update()`.

The concepts.

- `lv::tree_handler` becomes "empty, default constructible, and every parameter can be built".
  Sketch 5 expressed it as a concept, and it holds and fails as it should.
- Whether an id exists depends on the tree, which the concept does not see.
  That check is a `static_assert` in `apply_node`, with the ids in its message.
- A concept failure prints only the failed expression: `the expression '(handler_problem<F>)().empty() [with F = ._anon_350]' evaluated to 'false'` (Sketch 5).
  The `static_assert` in the trampoline printed: `lv: a handler cannot be given a double. It takes lv::event, the widget (...), a widget by its id, or the value of the widget.`
  So the handler's constructor accepts any empty callable, and names the bad parameter itself.
- The concepts branch names its expression `without_captures__name_the_state_as_a_template_argument`; that message stays right.
- `lv::event_handler` and `view.on<>()` are unchanged: a handler there may capture one pointer, so it needs no injection.

Cost.

- The scan runs where a handler is applied, once per handler, in a template: a tree without handlers pays nothing.
- A handler that takes nothing or `lv::event` can keep today's path, so `tests/large_tree.cpp` keeps its two handlers as they are.
- On the device each handler with injected parameters is a trampoline of its own.
  Its size on a Cortex-M4 is not measured.
- Today all handlers share two small callbacks, so the code grows with the number of such handlers, not the number of widgets.
- The calls it makes (`lv_event_get_current_target_obj`, `lv_obj_get_parent`, `lv_obj_get_child`, the getters) are in v9.6 and the v10 pin.

### The alternatives

| | D1 by name and type | D2 by type only | D3 explicit list | D4 none |
|---|---|---|---|---|
| Example | `[](lv::view<"label"> title)` | `[](lv::view<"label"> l)`: the only label | `lv::with<"title">([](lv::view<"label"> t) {...})` | `[](lv::event e) { e.target<...>(); }` and `get<>()` |
| Checked | Name, kind, tree | Kind; fails with two labels | Id and kind | Nothing about other widgets |
| Rename safety | A compile error | A second label breaks it | A compile error | Run time |
| Reads well | Yes | Until a second widget | Noisy | Globals and tasks |

Recommended: D1, with `lv::event`, `self`, ids, the value and the row; no states.

### The smallest slice

- Lambdas and functions with `lv::event`, `self` and ids of the same tree, outside structural nodes.
  The value and the row follow.
- Parts that take `void (*)()` today take an `lv::tree_handler auto` instead.

Tests.

- Run time: a click whose handler reads a label by id, a value event, a handler in a part used twice.
- Compile-fail: an unknown id (with the list), a view of the wrong kind, a generic lambda, an unnamed view.
- Compile-fail: a default argument, a capture, a type that cannot be built.
- Compile cost: `tools/compile_report.py` on `tests/large_tree.cpp`, and on a tree with 50 injected handlers.
- Code size: a handler with three parameters on Cortex-M4, against today's handler and a task that waits for the click.

Risks.

- The walk up and down assumes one LVGL child per tree kid.
  LVGL's own children are skipped as `tree_child_of` does, but a widget that adds children later breaks it.
- A function kept as a pointer of any signature has no constant-friendly home yet.
- An id in another tree, or across a page function arm of F1, cannot be reached.
  That stays `lv::weak` or a state.

What it removes from the ports.

| File and lines | What goes |
|---|---|
| `examples/ports/slint/printer/usb_page.cpp:40`, `46-48` and `94` | The global `lv::weak` `file_choice`, set after the mount: `start` takes `lv::view<"dropdown"> file` |
| `examples/ports/slint/printer/usb_page.cpp:81-87` and `95` | The task `show_chosen`: an `.on_value_changed` that takes `self` and `lv::view<"image"> photo` |
| `examples/ports/slint/printer/queue_view.cpp:62-70` and `83` | The task `open_on_tap` per card: an `.on_clicked` that takes `lv::view<"obj"> details` |
| `examples/ports/slint/printer/print_page.cpp:37-40` and `84` | The task `delete_on_tap`: an `.on_clicked` that takes the row, once the queue is an `lv::list` |
| `examples/ports/slint/printer/home_page.cpp:22` and `settings_page.cpp:45` | Parts that take `void (*)()` start to accept injected handlers |
| `examples/ports/qt/treadmill` | Nothing: its handlers change states only, and its one task (`follow_runner`) moves a widget on a timer, not on an event |

## What was compiled, and what was reasoned

| Claim | Compiled or run | Reasoned only |
|---|---|---|
| F1 syntax, exhaustiveness, three messages, the block, delete before build, ids through `row_view` | Yes, run on v9.6 in an ASan build | |
| F1 on the v10 pin | | Yes: the calls exist there |
| F1 `.enter` and `.leave`, deferral, focus, `lv::when`, A2 page functions | | Yes |
| F1 call operator added to `lv::is` itself | | Yes: the sketch used a copy of its shape |
| F2 B1 theme swap and shape check | Yes, run on v9.6 in an ASan build | |
| F2 B2 one type per token set, style per mode found and swapped | Yes, for one node and its child | The walk over a whole tree, plain properties |
| F2 B3, B4, B5 | | Yes |
| F3 consteval format, room, buffer, static text, no heap per change | Yes, run on v9.6 in an ASan build | |
| F3 `lv::bind(format, states...)` as a constant descriptor | Yes, `-fsyntax-only`, without a sanitizer | Its use through `lv::arg` and `apply_member` |
| F3 `<format>` cost | Yes: instructions on the host, size on Cortex-M4 | |
| F4 reading types and names of a lambda and a function; generic, default, unnamed | Yes, run on v9.6 in an ASan build | |
| F4 one pointer in a constant, its type found again; the concept; a call from a real event | Yes, run on v9.6 in an ASan build | |
| F4 error text of the concept and of the `static_assert` | Yes, `-fsyntax-only` | |
| F4 ids by a path from the tree, the row, the value of other widgets, a function of any signature in one member | | Yes |
| F4 code size per handler, compile cost | | Yes: not measured |

## Not confirmed

- Whether a coroutine resumed by a click survives a switch that deletes the clicked widget.
  The treadmill waits 1 ms (`shell.cpp:17`) without a recorded reason.
- The heap of a shown node on a Cortex-M4.
- The time of a theme swap on a Cortex-M4.
- Whether SwiftUI keeps a removed view alive until its transition ends.
- Whether Qt's `Loader` destroys the old item before it creates the new one.
- The code size of an injected handler on a Cortex-M4.

## Decisions for the owner

1. What are the names?
   Options: `lv::shown` and `lv::when`; `lv::show` and `lv::show_if`; `lv::switch_on`.
   Recommendation: `lv::shown` and `lv::when`, as sketched.
   Cost: none; `lv::signal::detail::shown` is an opacity constant in another namespace.
2. Are the arms `lv::is` and `lv::otherwise` from `match.hpp`, or new names such as `lv::arm`?
   Options: add a call operator to `lv::is`; new names.
   Recommendation: reuse, and fix `lv::match` accepting a value of another enum in the same change.
   Cost: one operator per pattern type; the fix may break user code that relied on the cast.
3. May an arm be a page function from another file, beside a constexpr tree?
   Options: trees only; trees and functions `lv::obj (*)(lv::obj)`.
   Recommendation: both, from the first slice.
   Cost: a function arm is checked only in its own file, and its ids are reached there.
4. Where does a shown node keep its state?
   Options: a block on LVGL's heap freed with the holder; a slot in a non-movable `mounted`.
   Recommendation: the block, as `lv::each` does.
   Cost: about 160 bytes per node on a 64-bit host; one allocation at mount.
5. May `lv::shown` have siblings in its widget?
   Options: only kid, refused at compile time; siblings with bookkeeping; a hidden host widget of its own.
   Recommendation: only kid.
   Cost: a host group in the tree, which both ports already have.
6. When does a switch happen?
   Options: inside the state change; at the next turn of LVGL's loop, merging changes.
   Recommendation: inside the change, unless the coroutine test fails.
   Cost: two changes in one handler build twice; deferral costs an allocation per switch.
7. What happens with `.leave` when the state changes again mid-animation?
   Options: cut the leaving tree at once; queue; play backwards.
   Recommendation: cut it, so that at most two arms exist.
   Cost: the cut tree vanishes without its animation.
8. Does a change between two values of one arm keep the tree?
   Options: keep; rebuild.
   Recommendation: keep.
   Cost: an arm that shows the value must bind it, not read it once.
9. How are ids inside an arm reached?
   Options: a view handed to `.on_shown`; an optional view from the root; a compile error only.
   Recommendation: the view in `.on_shown`, with `lv::row_view` renamed for both uses.
   Cost: a rename of a public name, and its stubs.
10. Where does the focus go after a switch?
    Options: LVGL's default (end of the group); the new arm's first widget if the old arm had it.
    Recommendation: the new arm's first widget.
    Cost: a walk of the group after each switch.
11. How does a theme change at run time?
    Options: swap in place (B1); rebuild the screen (B5).
    Recommendation: B1, with themes chosen by a state on `lv::display`.
    Cost: themes must have one shape; a full refresh per switch.
12. How does a node's own look follow a mode?
    Options: B2 restyle node; B2 inside `lv::shown` when the arms share one type; B3 LVGL's bind; B4 mode tokens.
    Recommendation: B2 as a node with its own name, B4 later as syntax over it.
    Cost: a template over the token set in user code; a table per node.
13. How do token files express modes?
    Options: one DTCG file per mode; one file with a value per mode; a DTCG resolver document.
    Recommendation: one file per mode, with a compile-time check of equal paths.
    Cost: the treadmill's `theme.light` group splits into files.
14. May a style value follow a state, `.bg_color = lv::bind(accent)`?
    Options: yes, through `lv_obj_bind_style_prop`; not now.
    Recommendation: not now; `.checked` with `.style_checked` and B2 cover the ports.
    Cost: none now; later a local style per widget on the heap.
15. Where does a formatted text's buffer live?
    Options: a block per label made at mount; a state or computed of `lv::text<N>`; `mounted`.
    Recommendation: a block per label, with `lv::text<N>` later for shared texts.
    Cost: a block and an observer per state, made at mount; none per change.
    Sketch 3: 256 bytes per label of two states on the host, against 112 for LVGL's binding of one value.
16. Which format language?
    Options: our subset `{}`, `{:02}`, `{:.N}` checked in consteval; `std::format`; printf.
    Recommendation: our subset.
    Cost: users meet a smaller language than `std::format`; growing it is ours to maintain.
17. What bounds a text value?
    Options: a required precision `{:.N}`; a default room with silent cutting.
    Recommendation: a required precision.
    Cost: one more thing to write for every text value.
18. How is a custom text written?
    Options: a formatter function into a span of fixed size; a computed that returns `lv::text<N>`.
    Recommendation: the formatter function.
    Cost: a capture-free function per format, as the treadmill has today.
19. Which parameters may a handler take?
    Options: `lv::event`, `self`, ids, the value and the row; also states by type or by name; only `lv::event` as today.
    Recommendation: the first, without states: a lambda names a global state without capturing it.
    Cost: a part that needs a state keeps a template argument.
20. How is a widget parameter matched to a widget?
    Options: by name as an id, the kind checked (D1); by type alone (D2); an explicit list (D3).
    Recommendation: D1, with no fallback by type.
    Cost: parameter names become API; renaming an id breaks its handlers at compile time.
21. What is refused?
    Options: refuse generic lambdas, default arguments and unnamed views; or use the default and skip the unnamed.
    Recommendation: refuse all three, each with its own message.
    Cost: a handler cannot have an optional parameter.
22. How is a plain function with injected parameters kept?
    Options: through a static object per function, found again by reflection; only lambdas inject, functions keep today's two forms.
    Recommendation: lambdas and functions with `lv::event` and `self` first; functions with ids when the static object is proven.
    Cost: for a while, a function cannot name ids; a lambda that calls it can.
23. Does `lv::tree_handler` grow to "every parameter can be built"?
    Options: yes, and the handler's constructor names the bad parameter; keep it as two signatures.
    Recommendation: yes; the id check stays a `static_assert` where the tree is known.
    Cost: two places say what is wrong, the concept and the tree.

## What the owner decided

Decided 2026-11-18, in an interview over the list above.
The number is the decision it answers.

| # | Decision |
|---|---|
| 1 | The names are `lv::shown` and `lv::when`. |
| 2 | Arms reuse `lv::is` and `lv::otherwise`. The enum bug of `lv::match` is fixed in the same change. |
| 3 | An arm is a tree or a page function from another file, from the first slice. |
| 4 | No heap block. The node's storage must not allocate, and must be extendable and open for new node kinds. See [node-storage.md](node-storage.md). |
| 5 | `lv::shown` is the only child of its widget. Anything else is a compile error. |
| 6 | The switch must not allocate. Its timing is decided with the storage research. |
| 7 | A page that is asked for again while it leaves plays its leave backwards. Any other change cuts it. |
| 8 | Two values of one arm keep the tree. |
| 9 | Ids in an arm are reached through a view handed to `.on_shown`. |
| 10 | Focus goes to the first widget of the new page, if the old page had it. |
| 11 | The theme changes in place, chosen by a state on `lv::display`. |
| 12 | Tokens resolve through the active mode from the start. The tree is written once. |
| 13 | Modes are described by a DTCG resolver document. |
| 14 | A style value may follow a state in this round. An allocation-free form is researched first. |
| 15 | The buffer of a formatted text lives in the node storage of decision 4. |
| 16 | The format language is our own subset, checked at compile time. |
| 17 | A text value in a format needs `{:.N}`. |
| 18 | A custom text is a capture-free formatter function that writes into a fixed span. |
| 19 | A handler may take the event, its own widget, widgets by id, the value and the row. States are not injected. |
| 20 | The id is written in the type of the parameter: `lv::view<"label", "title">`, the kind and then the id. Parameter names are not read. |
| 21 | A generic lambda stays refused: it has no parameter list to read. With the id in the type, an unnamed parameter is fine. |
| 22 | Lambdas come first. A plain function takes the event and its own widget, until ids in the type are proven for functions. |
| 23 | `lv::tree_handler` means that the library can build every parameter. The check of an id stays where the tree is known. |

The order of building is the page node, formatted text, injection, and then looks.
The cost of a wider handler to `tests/large_tree.cpp` is measured before injection is built.
If a tree without injection gets dearer, the owner decides again.
