# Storage of structural nodes, without the heap

Research and design for the run-time storage of lvgl26's nodes, written 2026-11-17 and 2026-11-18.

Marks follow `docs/research/README.md`.
Every mark is dated 2026-11-17 unless its row says otherwise.

- `source`: LVGL's code read in `build/_src/lvgl-v9.6.0` and in the v10 pin `53819fe`, or our own code.
- `measured`: a sketch compiled and run in this session; "Measured here" says how.
- `summary` and `recalled`: as the README says.

The question comes from the owner's answers about `lv::shown(state)[...]` and `lv::when(flag)[tree]`.
The last two quotes are his two principles for all of lvgl26's design.

- On storage: "I want to avoid allocation - can we research the best solution that will also be extendable, maintainable, and follow open-close principle."
- On the time of a switch: "We should avoid allocation. Research better approaches."
- On buffers: "if we can calculate something in compiletime it should not allocate but it should use calculated buffers that can be created on stack."
- On the aim: "create expressive abstraction that helps to write less code but as expressive as possible and that provide information for compiler that it can use to optimize it."

`docs/research/shown-theme-format.md` (pull request 99) proposed a heap block per shown node, as `lv::each` has one.
The owner rejected that.
`docs/research/mounted-storage.md` on the local branch `spike/non-movable-mounted` measured bindings kept inside a non-movable `mounted`.
This note builds on both and does not repeat them.

## The design rule

What the compiler can size, it sizes, and the caller decides where it lives.

- The run-time storage of a tree is one plain object, whose type and size come from the tree at compile time.
- Every buffer whose size the compiler knows is part of it.
  That is a formatted text, the rows of `lv::each` up to its capacity, the arms of `lv::shown`, an animation in flight.
- lvgl26 allocates nothing of its own for a tree: not at mount, not per change, not per switch.
- The caller places the object: on the stack, as a `constinit` global, as a member, or with placement new in a region reserved at start.
- No placement is assumed by the library, and none is hidden: there is no static per tree.
- LVGL's widgets stay on LVGL's heap.
  Whatever else LVGL allocates because of us is counted and named.

## Short answers

| Question | Answer | Evidence |
|---|---|---|
| Can one mechanism give every node kind its storage, sized at compile time? | Yes: each kind declares a nested `storage<Tree, Path...>`; the tree's storage is their aggregate, made by reflection | Sketch (i) |
| Can a new kind be added without touching the mount code? | Yes: `proto_stamp.hpp` added a kind; the mount code's hash did not change | Sketch (i) |
| Does a shown node switch without lvgl26 allocating? | Yes: 4,000 switches, nested shown nodes and bindings in the arms, no call of ours that allocates, the heap flat | Sketch (ii) |
| What does the mount cost beyond the widgets? | With a handler on the root: 48 bytes in 2 blocks on the host. Watched through the parent: nothing after the parent's first tree | Sketches (ii), (vi), (ix) |
| Can the whole storage live on the caller's stack? | Yes: `auto root = proto::mount<ui>(parent);`, built in place, 224 bytes in the frame | Sketch (vi) |
| Can it be placed with placement new in a region laid out at compile time? | Yes: two screens in a `constinit` region of 296 bytes, its size a constant expression | Sketch (vi) |
| Can `mount` still return `std::expected<mounted, mount_error>`? | Not with the storage inside: failure is known only after the build at the final address. `mounted` carries the failure itself | Sketch (vi), reasoned |
| Arms: the sum or the largest? | The largest arm, as a union. With a leave played backwards: the largest plus the second largest | Sketch (ii), reasoned |
| When does a switch happen? | Recommended: deferred to just before the next frame, drained from the display's refresh. No allocation per switch | Sketch (iii) |
| What does code after `state = x;` see? | Immediate: the new page. Deferred: still the old page; the drain builds the new one before the next frame | Sketch (iii) |
| Three changes in one handler? | Immediate: three builds. Deferred: one | Sketch (iii) |
| A coroutine resumed by a click of the page that goes? | Immediate: its own widgets are already deleted when its code goes on. Deferred: they still exist | Sketch (iii) |
| A style value that follows a state, without heap? | Yes: an `lv_style_t` and its property array in the node's storage, changed in place: 0 bytes per change | Sketch (v) |
| Does LVGL run out of memory cleanly during a mount? | Ours does, at all 342 points. LVGL itself crashes in `lv_obj_add_style` when its theme's realloc fails | Sketch (vii) |
| Does the mechanism make an ordinary tree dearer to compile? | No: the same instructions and the same largest evaluation for the 112-widget tree of `large_tree` | Measurement (iv) |
| Does it remove the heap block of `lv::each`? | Yes, its 200 bytes on the host move into the tree's storage. The migration is small; see "Migrating lv::each" | Reasoned |

## Measured here

GCC 16.2 on macOS arm64, the `host-v96` warning flags with `-Werror`, against the headers of `origin/main` at `747c741`.
Each sketch also ran against the v10 pin, linked with the LVGL library of the main checkout's `host-v10` build.
Every number below was the same there, except the absolute heap of sketch (ii).
Pointers are 8 bytes: a Cortex-M4 needs about two thirds of each byte figure.
Heap is `lv_mem_monitor`: `total_size - free_size` in bytes, and `used_cnt` in blocks.
Apple's linker has no `--wrap`, so LVGL's allocations cannot be counted by symbol here.
Instead each sketch counts its own calls that make LVGL allocate: `lv_obj_add_event_cb`, `lv_timer_create`, `lv_display_add_event_cb`.
It also compares the heap with a control: the same widgets built by `lv::detail::build` alone.
The sketches live outside the repository, in a scratch directory; this note quotes what they printed.
`proto_storage.hpp` is the mount code, `proto_kinds.hpp` the kinds `text` and `shown`, `proto_stamp.hpp` a kind added later, `proto_mounted.hpp` the storage inside the returned object.

The sketch's state is not `lv::state`.
`lv::state` on main makes an LVGL subject and an observer per binding, which allocate.
The sketch uses a state with an intrusive list of watchers, as the spike did, with a safe walk (see "The state's list").

| Sketch | What | Result |
|---|---|---|
| (i) | Storage of a tree with a shown node, a text node and a new kind `stamp` | 200 bytes; arm a 88, arm b 28; a stamp 28 |
| (i) | The new kind, in its own file | Compiles and runs; `proto_storage.hpp` and `proto_kinds.hpp` kept their SHA-256 when it was added |
| (i) | A tree without any kind that keeps storage | `storage_t<plain>` is the empty type: `static_assert` |
| (i) | Stamps alive after the root is deleted | 0: the storage ends with the root |
| (ii) | Storage of a shown node of three arms (home 64, settings 272, empty), one level nested | 360 bytes for the tree; the union holds 272, the sum would be 336 |
| (ii) | Heap of the mount, against the control | 2,192 bytes in 36 blocks against 2,144 in 34: the root's handler |
| (ii) | 4,000 switches, a change of `ticks` every tenth, the inner arm every seventh | 5,528 arms built; 0 calls of ours that allocate |
| (ii) | Heap over the switches | 11,808 bytes before, 11,800 after 2,000 and after 4,000; 138 blocks throughout |
| (ii) | A binding inside the settings arm to the state that switches it | Shows "page 1"; the walk told a probe watcher exactly once for each of 4,004 changes |
| (ii) | The root deleted with `lv_obj_delete` from the C API, then the state changed | Only the probe is told: no node of the tree is left on the state |
| (iii) | The five policies, with a real click, a task, and holders that cannot be drawn | See "B. When a switch happens" |
| (viii) | A watcher unlinks the one the walk visits next; unlinks itself; changes the same state inside the walk | Told 1, 0, 1; then 2, 2, 2; then 1, 2, 2: as wanted |
| (ix), (x) | Four ways to watch the root | See "The root watch without an allocation" |
| (iii) | The drains' own setup | One timer and one display handler, made once at the first mount |
| (iii) | 2,000 deferred switches drained by the refresh | 13,024 bytes and 142 blocks before and after; 0 calls of ours |
| (v) | Five ways for a style value to follow a state | See "A style value that follows a state" |
| (vi) | A tree mounted with its storage on the stack, `watch::root` | 224 bytes in the frame; heap 1,624 bytes in 29 blocks against the control's 1,576 in 27 |
| (vi) | The same, `watch::none` | Heap 1,576 bytes in 27 blocks: exactly the control; 0 calls of ours |
| (vi) | 2,000 switches on the stack-mounted tree | `watch::root`: 0 bytes, 0 blocks. `watch::none`: 16 bytes more, 0 blocks more, not explained |
| (vi) | The scope ends | The heap is back where it was before the mount, in all three runs |
| (vi) | Two screens placed with placement new in one `constinit` region | 296 bytes, its offsets computed as constant expressions; both mount and switch |
| (vii) | The mount with 8 more bytes of heap each time, until it succeeds | 342 failures; none leaves storage alive, a watcher on a state, or heap behind |
| (vii) | The same loop with the default theme, with our mount or main's `lv::mount` | Crash in `lv_memset(NULL, 0, 16)` from `lv_obj_add_style`, called by the theme |

### Compile cost, measurement (iv)

The tree is the 112-widget tree of `tests/large_tree.cpp`, copied with its tokens.
Instructions come from `tools/compile_report.py` with a compile database of our own, two rounds each.
The operations limit was bisected with `-fsyntax-only` to within 20,000.

| File | What it compiles | Instructions (G) | Smallest `-fconstexpr-ops-limit` |
|---|---|---|---|
| `iv_tree_only_bind` | The tree as in `large_tree`, no mount | 82.07 to 82.21 | 5,750,000 to 5,761,718 |
| `iv_base` | That tree, `lv::mount` of main | 92.01 to 92.24 | 5,750,000 to 5,761,718 |
| `iv_storage` | That tree through the storage mechanism: `storage_for<ui>::place` | 91.91 to 92.13 | 5,750,000 to 5,761,718 |
| `iv_tree_only` | The tree with its 18 bound labels as 18 text nodes that keep storage, no mount | 82.06 to 82.28 | 5,750,000 to 5,761,718 |
| `iv_size_only` | The same, plus `sizeof(storage_t<ui>)`: 1,008 bytes | 84.51 to 84.64 | 5,750,000 to 5,761,718 |
| `iv_nodes` | The same, mounted | 97.20 to 97.47 | 5,750,000 to 5,761,718 |
| `e_empty` | Only the headers: `proto_kinds.hpp`, which includes `lv/lv.hpp` | 36.85 to 36.90 | |
| `e_tokens` | The headers and the tokens of `large_tree` | 43.09 to 43.20 | |

What the numbers say.

- An ordinary tree costs nothing more: `iv_storage` and `iv_base` differ by less than the noise between rounds.
- The check that a tree keeps no storage is one variable template per node type, `any_storage<N>`.
  A tree of repeated parts asks once per part.
- Sizing 18 storage nodes costs about 2.4 G instructions, 3 percent of the file.
  It is one `define_aggregate` per widget that has storage below it.
- The largest single evaluation is the same in all six files, 5,750,000 to 5,761,718: the tree's own.
  The storage computation stays under it.
- A `large_tree` that used the mechanism would stay under its fixed limit of 6,500,000.

## Findings in LVGL

All rows are `source`, read in v9.6.0 and in the v10 pin, unless the row says otherwise.
Where the two versions differ, the row says so.

| Finding | Where | What it means for us |
|---|---|---|
| `lv_subject_notify` marks every observer "not notified", calls each, and restarts the walk when an observer was deleted meanwhile (`notify_restart_query`) | `src/core/lv_observer.c`, identical in both | Deleting observers inside a notification is safe in LVGL |
| On a restart, the observer that deleted another one is called again: its `notified` flag is set only after the check | same | A switch built on a subject must ignore a second call with the same value |
| `lv_obj_delete` sends `LV_EVENT_DELETE` to a widget before its children are deleted | `src/core/lv_obj_tree.c`, `obj_delete_core` | Our storage can end in the root's handler while every widget still exists |
| Deleting a widget marks the events in flight for it; `lv_event_send` stops its loop and keeps the old handler array alive until the end | `src/misc/lv_event.c` | Deleting the arm in which a click is being handled is safe for LVGL |
| `lv_obj_delete` resets input devices that point at the widget and cancels its `lv_obj_delete_async` calls | `src/core/lv_obj_tree.c` | A deleted arm leaves no input device on freed memory |
| `lv_obj_add_event_cb` allocates a descriptor, and the event array grows by realloc | `src/misc/lv_event.c`, `lv_event_add` | Every handler we add costs heap: one at mount is the floor through the public API |
| `lv_async_call` allocates a record and creates a timer on every call | `src/misc/lv_async.c` | Deferring with it allocates per switch: rejected |
| `lv_timer_create` inserts at the head of the timer list; the handler restarts from the head when a timer is created or deleted | `src/misc/lv_timer.c` | The order in which a drain timer runs depends on when it was made |
| `lv_timer_pause`, `lv_timer_resume` and `lv_timer_ready` exist in both versions and allocate nothing | `include/lvgl/core/lv_timer.h` | One timer made once can be woken for each switch |
| The refresh timer pauses itself after drawing; `lv_obj_invalidate` wakes it, but only for a visible widget | `src/core/lv_refr.c`, `lv_obj_pos.c` (`lv_obj_area_is_visible`); `measured` in sketch (iii) | The drain wakes the refresh timer directly |
| `lv_display_get_refr_timer`, `lv_display_get_event_count` and `lv_display_get_event_dsc` are public in both | `include/lvgl/display/lv_display.h` | The wake needs no invalidation, and a display's handler can be found again |
| `LV_EVENT_REFR_START` is sent before the refresh looks at what is invalid | `src/core/lv_refr.c` | The drain runs even when nothing else changed |
| `lv_test_wait` ends with `lv_refr_now(NULL)` | `src/debugging/test/lv_test_helpers.c` | A test of a deferred switch must step the loop itself, or it cannot fail |
| In v9.6, `lv_obj_is_valid` calls `lv_obj_is_in_widget_tree`, which reads the widget | `src/core/lv_obj.c`; `measured`: it crashed on a deleted label | It cannot test whether a pointer we hold is still a widget |
| `LV_EVENT_REFR_START` comes before the layout and before drawing | same | A drain there builds the new page before the frame that shows it |
| `lv_indev_get_active_obj` and `lv_indev_active` are public in both | `include/lvgl/indev/lv_indev.h` | A hybrid switch can see a click of the outgoing page, but no event sent by code |
| `lv_obj_class_t` is defined only in the private `src/core/lv_obj_class_private.h` | both | We cannot make a widget class whose instance holds our storage, or whose destructor tells us |
| `lv_obj_set_external_data(obj, data, free_cb)`: `free_cb` runs in `lv_obj_destruct`, without an allocation | `src/core/lv_obj_class.c` | A free deletion hook per widget, but only if LVGL is built with `LV_USE_EXT_DATA`, default 0, which grows every widget |
| `lv_obj_add_style` grows the style list with `lv_realloc` and does not check the result | `src/core/lv_obj_style.c`, line 148 in both | LVGL crashes when the heap runs out while a theme adds a style; `measured` in sketch (vii) |
| `LV_USE_ASSERT_MALLOC` is 0 unless the configuration sets it; lvgl26 builds with `LV_CONF_SKIP`, so it is 0 | `include/lvgl/config/lv_conf_internal.h` | No assertion stops the crash; with it on, LVGL would halt instead |
| A style with `prop_cnt` 255 is constant: `lv_style_copy`, `lv_style_merge` and `lv_style_remove_prop` refuse to write it | `include/lvgl/core/lv_style.h`, `src/misc/lv_style.c` | Its property array may be in writable memory of ours, changed by us only; `measured` in sketch (v) |
| `lv_obj_refresh_style(obj, part, prop)` refreshes one widget; `lv_obj_report_style_change(style)` walks every screen of every display | `src/core/lv_obj_style.c` | Refresh the one widget that uses a node's style |
| `lv_obj_bind_style_prop` and `lv_obj_bind_style` exist in both versions | `include/lvgl/core/lv_obj_style.h` | LVGL's own binding of a style value. It allocates; see sketch (v) |
| `lv_obj_get_event_dsc`, `lv_event_dsc_get_cb` and `lv_event_dsc_get_user_data` are public in both | `include/lvgl/core/lv_obj_event.h`, `lv_event.h` | Our root handler's `user_data` can be found again from the root, without reserving `user_data` |

## Other frameworks

Rows from `docs/research/memory.md` (section 5) and `docs/research/components.md` (section 4) are not repeated: TouchGFX's partition, Qt for MCUs' `Loader`, embassy's statics.

| Finding | Link | Mark |
|---|---|---|
| Xilem: a `View` has an associated type `ViewState`. `build` returns the element and the state; `rebuild`, `teardown` and `message` get `&mut ViewState` | [xilem_core View](https://docs.rs/xilem_core/latest/xilem_core/trait.View.html) | `source`, read through a fetch tool, 2026-11-18 |
| Xilem: the view state is internal to each view implementation and is not public API | same | `source`, 2026-11-18 |
| Compose: the slot table keeps what `remember` stores, by the position of the call, in arrays used as a gap buffer while writing | [how-compose-works.md](https://android.googlesource.com/platform/frameworks/support/+show/2435c8283887ff3b5fe818c44d03f7fc079e2c87/compose/runtime/design/how-compose-works.md) | `summary`, 2026-11-18 |
| Compose: a newer runtime replaces the gap buffer with a linked buffer, for fewer copies when groups move | [Compose runtime releases](https://developer.android.com/jetpack/androidx/releases/compose-runtime) | `summary`, 2026-11-18 |
| ETL: containers and `etl::pool` have a capacity fixed at compile time and no heap | [pool.h](https://sources.debian.org/src/etlcpp/20.44.2%2Bdfsg-2/include/etl/pool.h) | `summary`, 2026-11-18 |
| Qt for MCUs ships ETL among its third-party parts | [Qt for MCUs 2.5 snapshot](https://doc-snapshots.qt.io/qtformcus-2.5/quickultralite-automotive-src-3rdparty-etl-include-etl-pool-h.html) | `summary`, 2026-11-18 |
| SwiftUI keeps view state in an attribute graph owned by the framework, outside the view values | none found | `recalled` |
| Flutter keeps a `State` object per `StatefulElement`, on the heap, found by the element tree | none found | `recalled` |

What they agree on.

- The framework owns the per-view state, and the view's type says what the state is: Xilem's `ViewState` is our nested `storage`.
- Only TouchGFX and embassy size it at compile time; the others allocate it.
- None found lets the caller choose the memory of a screen's state.

## A. One storage mechanism

### How it works

A node kind that keeps run-time data says so in its own type.

```cpp
template <class E, class... Arms>
struct shown_node {
  const state<E>* source;
  d::kids<Arms...> kids{};

  template <const auto& Tree, std::size_t... Path>
  struct storage : watcher, pending {          // what one shown node keeps at run time
    lv_obj_t* holder = nullptr;
    lv_obj_t* root = nullptr;
    int arm = -1;
    alignas(room_align<Tree, Path...>()) std::byte region[room<Tree, Path...>()];  // the largest arm
    ~storage();                                // ends the shown arm's storage; never deletes a widget
  };

  template <const auto& Tree, std::size_t... Path>
  static lv_obj_t* build(lv_obj_t* parent, d::build_state& st);   // finds its storage, builds an arm
};
```

The mount code knows no kind.

- `any_storage<N>` is true for a kind that declares a member named `storage`, found by reflection, or for a widget with such a kind below it.
- `storage_t<Tree, Path...>` is the kind's `storage`, or, for a widget, an aggregate with one member `s<k>` per child that keeps storage, made by `define_aggregate`.
- A structural node opens a scope: what it builds (an arm, a row) keeps its storage inside the node's own storage.
- `locate<Tree, Path...>` gives the offset of a node's storage in its scope, from `offset_of` of the members on the way.
- The build hands the scope's address down.
  The sketch keeps it in a global; in lvgl26 it would be one member of `build_state`.
- A scope's storage is value-initialised before its widgets are built, and destroyed before its widgets are deleted.
- So a kind has two hooks: a `storage` whose destructor lets go of what the node holds, and `build`.

The life of a scope.

- Mount: construct the tree's storage, build the widgets, add one handler to the root.
- A switch: destroy the old arm's storage, delete its root, construct the new arm's storage, build it.
- The root deleted by anyone, through `lv::mounted`, its parent or the C API: the root's `LV_EVENT_DELETE` ends the storage while the widgets still exist.
- Out of memory during a build: the scope's storage is destroyed, then its partial widgets deleted.
  Sketch (vii) found nothing left at 342 points.

### The state's list

A state keeps its watchers in an intrusive list, as `lv::list` keeps its views (`watcher_chain` in `include/lv/list.hpp`).

- A switch destroys the old arm's storage while the state tells its watchers.
- That unlinks watchers of the same list: the arm's own bindings to the state.
- The walk keeps a record of the watcher it visits next.
  Unlinking that watcher moves the record on.
- Nested walks keep a chain of records, so a watcher may change another state, or the same one.
- A watcher linked during a walk is not told; it shows the value when it links.
- Sketch (ii) told a probe exactly once for each of 4,004 changes, with the arm's binding unlinked mid-walk 2,000 times.
- LVGL's subject restarts its whole walk instead, and calls the deleting observer twice (see "Findings in LVGL").

### Where the storage lives

| | S1 Heap block per node | S2 Inside a non-movable `mounted` | S3 Caller storage, `mounted` a handle | S4 A static per tree | S5 Inside a widget of our own class |
|---|---|---|---|---|---|
| What the user writes | `auto root = lv::mount<ui>(parent);` as today | The same line; or `constinit lv::mounted<ui> page;` and `page.mount(parent)`; or placement new | `constinit lv::storage_for<ui> s;` then `lv::mount<ui>(parent, s)` | `lv::mount<ui>(parent)`; a second mount of `ui` fails | Not possible |
| Allocations of lvgl26 at mount | One block per structural node, plus its handler | One root handler (48 bytes on the host), or none with `watch::none` | The same as S2 | The same as S2 | |
| Allocations per change and per switch | None | None, measured | None | None | |
| RAM | Heap, while shown | Exactly the tree's storage, where the caller puts it | The same, in a second object | Every tree ever mounted, all the time | |
| Open-closed | Each kind allocates itself | Kinds declare `storage`; mount knows none | The same | The same | |
| Two mounts of one tree | Yes | Yes | Yes, two storages | No, or a count in the type | |
| Address | Stable | Stable: not movable | Stable; the handle moves | Stable | |
| `std::expected<mounted, E>` | Kept | Lost: `mounted` carries the failure, with the same `->`, `*` and `operator bool` | Kept for the handle | Kept | |
| `lv::place`, page functions in other files | Work | Need storage from the caller | Take the storage | Work | |
| Compile cost | None | None for an ordinary tree, measured | The same | The same | |
| Maintainability | Each kind writes its own allocation, delete handler and failure path | Two hooks per kind; one place builds, sizes and ends scopes | The same, plus a handle to keep in step | The same as S2 | |
| Code size | One small allocation path per kind | Not measured; the generic part is a few functions per tree | Not measured | Not measured | |
| Debuggability | A block reached from a handler's `user_data` | lldb shows the storage by member: `s0`, the shown node's `arm`, `holder`, `src`; an arm's region shows as bytes | The same | The same | |
| Main risk | It is what the owner rejected | Code that moves a `mounted` stops compiling | Two objects whose lifetimes must match | Hidden static RAM | `lv_obj_class_t` is private |

Recommended: S2, with S3 as the form for page functions and `lv::place`.

- S2 is the owner's rule taken literally: the object is exactly sized, and the caller's variable is where it lives.
- `auto root = lv::mount<ui>(parent);` keeps compiling: the prvalue is built in place, guaranteed by the language (sketch (vi)).
- `mounted` keeps the shape of `std::expected`: `if (!root)`, `root->get<"id">()`, `root.error()`.
- S3 is the same storage type with no owner around it, for code that hands the widgets away.
- S4 is rejected as a default: its RAM is hidden, and a tree mounted twice needs a count.
  It stays possible as one placement: a `constinit` global.

What S2 asks of the API and of lifetimes.

- `mounted` cannot be moved or copied.
  `std::vector<mounted>` and returning a named one stop compiling.
  The spike found only one example and one test that did either.
- It stays an owner: when it goes out of scope, its widgets are deleted, as today.
- A screen replaced by assignment becomes a `std::optional<lv::mounted<ui>>` with `emplace`, or a page in `lv::shown`.
- `std::expected<mounted, E>` cannot be returned: the build happens at the final address, and its failure is known only after the object exists.
- Two other ways keep `std::expected`, both reasoned.
  The spike's two steps build on the stack, then link in the final object.
  Every kind then needs a third hook, `link`, and the mount needs twice the stack.
  The form `constinit lv::mounted<ui> page;` with `page.mount(parent)` returns `std::expected<void, mount_error>`.

### How a kind is found: the customization point

| Way | How a kind says it keeps storage | Open-closed | Cost for the compiler | Error when wrong | Mark |
|---|---|---|---|---|---|
| Nested template found by reflection (the sketch) | `template <const auto& Tree, std::size_t... Path> struct storage;` in the kind | Yes | `members_of` once per structural type | Ordinary, at the kind's `storage` | `measured` |
| A trait specialised outside | `template <> struct lv::storage_of<my_kind> {...}` | Yes, in the kind's header | One lookup | Forgotten silently: the kind has no storage | reasoned |
| A concept with required members | `requires { typename K::template storage<...>; K::template build<...>; }` | Yes | One check per use | The concept names what is missing | reasoned |
| A function found by argument-dependent lookup, as `tag_invoke` | `storage_for(tag, const my_kind&)` | Yes | Overload resolution per node | Long candidate lists | reasoned |
| A registry built with `std::meta` from annotations | `[[=lv::keeps_storage]]` on the kind | Yes | A scan of annotated types | Depends on GCC's annotation support, not checked | `recalled` |

Recommended: the nested template, checked by a concept at the place where a structural node is accepted.
It follows the idiom that `lv::each` uses for `build`, and Xilem's `ViewState` is the same shape.

### What each kind keeps

Sizes are for the host, from the sketches where marked.

| Kind | Its storage | Sized from | Mark |
|---|---|---|---|
| Binding to a state (`.value = lv::bind(speed)`) | A watcher: next, previous, function; the widget | The property | The spike measured 12 bytes on a Cortex-M4 |
| Formatted text (`lv::bind("{}% - {:.12}", progress, word)`) | A watcher per state, the label, and the text buffer | A consteval parse of the format: the widest result | `measured` for one `%d` (text node, sketch (ii)); several values reasoned |
| A custom formatter, a capture-free function into `std::span<char, N>` | The same buffer; N is the function's parameter type | The span's extent | reasoned |
| A style value that follows a state (`.bg_color = lv::bind(accent)`) | An `lv_style_t` and its property array, a watcher | The property | `measured`, sketch (v) |
| `lv::shown` / `lv::when` | Watcher, pending link, holder, root, arm, the arm region | The largest arm, or the two largest | `measured`, sketch (ii) |
| `lv::each` | Today's `each_view` and its keys; the rows' storage | The list's capacity | reasoned; see "Migrating lv::each" |
| An animation in flight (`.enter`, `.leave`) | Start time, duration, from, to, a link in a list of running animations | The animation | reasoned |
| A task spawned for a tree | A slot of declared size for the coroutine frame | The user's size, checked at run time | reasoned; `memory.md` measured a frame in caller memory |
| The ids of a tree, for `get<"id">()` and handlers | None needed: a path known at compile time | The tree | reasoned; `lv::row_view` follows one today |
| Mode-aware tokens and the theme | None per node | | reasoned; see "A style value that follows a state" |

### The arms of `lv::shown`: largest, pairs, or sum

| Layout | Bytes, the sketch's three arms (64, 272, 0) | When it is enough | Mark |
|---|---|---|---|
| Union of the arms | 272 | One arm alive at a time: no `.leave`, or a leave that is cut | `measured` |
| Largest plus second largest, a region used from both ends | 336 | Two arms alive: the leaving one and the entering one | reasoned |
| Two slots of the largest | 544 | The same, with simpler code | reasoned |
| Sum of all arms | 336 | Every arm kept alive, as when all are built and hidden | reasoned |

The owner chose that a leaving page asked for again plays its leave backwards.
Then at most two arms are alive: the leaving and the entering one.

- The region is used from both ends: one arm from the low end, the other from the high end.
- When one goes, the next arm takes the free end.
  Any two arms fit if the region holds the largest and the second largest.
- A leave played backwards swaps the roles of the two arms; nothing moves.
- With many arms of similar size this is about twice the union; with one large arm it is little more.
- The layout is chosen at compile time: a node without `.leave` keeps the union.

### Page functions from other files

The owner chose that an arm may be a tree or a page function from another file.
The page's storage then belongs to the parent's region, so its size must be a constant in the parent's file.

| Way | What the page's header holds | Cost in the parent's file | Mark |
|---|---|---|---|
| The page's tree is in its header; its build is compiled in its own file | The constexpr tree; `extern` build function taking the storage | Sizing only: about 2.4 G instructions for 18 storage nodes in 112 widgets | `measured` for the sizing |
| The page declares its size | `lv::page<&home_page, 256>`; the page's file checks it with `static_assert` | None | reasoned |
| The page keeps a static storage in its own file | Nothing | None, but every page's storage exists all the time | reasoned |

Recommended: the declared size, checked with `static_assert` in the page's file.

- Measurement (iv) shows that the tree constant, not the build, is the expensive part of a page.
- The headers cost 36.9 G instructions and the tokens 6.2 G more.
- The 112-widget tree adds about 39 G, and its build about 10 G.
- A tree in a header would move those 39 G into every file that shows the page.
- A declared size moves nothing: the parent reads a number, and the page's file checks it against `sizeof`.
- The message of the check can name the right number, so a stale size is a one-line fix.

### Handlers that find widgets by id

The owner chose handlers whose parameters name widgets by type, `lv::view<"label", "title">`, without a capture.

- A handler is generated for one node, at a path known at compile time.
- From the widget that fired, it goes up the length of that path to its scope's root, then down the path of the id.
- `lv::row_view` already goes down such a path (`child_on_path` in `include/lv/list.hpp`), and `lists.cpp` runs it.
- So the id needs no storage, no table and no allocation; the walk is a few pointer loads.
- Inside a scope of `lv::each` or `lv::shown`, the walk up ends at the scope's root.
  So an id is found in its own row or arm.
- If a table is wanted instead, the root's handler `user_data` leads to the storage: `lv_obj_get_event_dsc` and `lv_event_dsc_get_user_data` are public.

### Lifetime, the C API, `owned`, `spawn`, `weak`

- `lv::owned` and `lv::weak` add a delete callback each, which allocates.
  `mounted` no longer needs them: the root's one handler does their work.
- A widget deep in a tree, deleted alone through the C API, leaves its binding's watcher behind.
  The spike measured that.
- Through the public API, the only free hook per widget is `lv_obj_set_external_data`, which needs `LV_USE_EXT_DATA`.
- So the rule: "a widget of a mounted tree goes with its tree".
  A checked build can look the widget up by address below the root before each update; `lv_obj_is_valid` reads it.
- `lv::spawn(work, owner)` allocates the frame, a watch record and its handler.
  A task slot in the storage removes the frame; the record can be part of the slot.
- A task tied to an arm ends when the arm's widgets go, as today: sketch (iii) saw the task end once in every policy.
- Some kinds hold memory that LVGL points into: a text buffer shown with `lv_label_set_text_static`, a style of way B.
  Their destructors detach nothing: the scope's memory is reused only after `lv_obj_delete` of its root returns.
  In the root's handler the memory is the `mounted` object itself, which outlives the delete it is running.
- The focus and two values that share one arm need no storage: the first is read from LVGL's group, the second compares arm numbers.

### Migrating `lv::each`

- `each_view` is no template, and lives in one `lv_malloc` block with its keys: 200 bytes on the host for 20 rows (`docs/research/lists.md`).
- Its storage is `sizeof(each_view)` plus `capacity + 1` keys: both known at compile time from the list's type.
- The migration: `each_node` declares `storage` holding `each_view` and the keys, and `start` takes it instead of calling `lv_malloc`.
- `on_outlet_delete` goes: the storage ends in the root's handler, while the rows still exist, so `close()` works as today.
- Rows whose template keeps storage need a slot per row: `capacity` slots of the row scope, with a free list, as `lv::list` keeps its cells.
- The selection still adds one handler per row and an observer, which allocate.
  They go when the state has its own list and the outlet catches bubbled clicks.
- Estimated at about 60 changed lines in `include/lv/list.hpp` and the tests that measure the block.

## B. When a switch happens

### What is safe in LVGL

- Deleting a widget inside its own event: safe.
  LVGL marks the event deleted, stops calling handlers and frees the old handler array afterwards.
- Deleting the parent of the widget whose event runs: safe for the same reason; the input device is reset.
- Deleting observers of a subject while it notifies: safe.
  LVGL restarts its walk; the deleting observer is called a second time.
- What is not safe: code of ours that still holds a pointer to a deleted widget.
  A coroutine resumed by the click is such code.

### The options

| | B1 Immediate | B2 Deferred, a paused timer | B3 Deferred, the display's refresh | B4 Hybrid |
|---|---|---|---|---|
| How | The state's watcher builds the new arm at once | The watcher queues the node; one timer, made once and paused, is woken | The watcher queues the node and resumes the display's refresh timer; a handler on `LV_EVENT_REFR_START` drains | Deferred while the input device's active widget is in the outgoing arm, else immediate |
| Allocations once | None | One timer per program | One handler per display | As B2 or B3 |
| Allocations per switch | None | None | None | None |
| Code after `state = x;` sees | The new page | The old page | The old page | The old page after a click; the new page otherwise |
| Three changes in one handler | Three builds | One build | One build | One after a click; three otherwise |
| First frame after the change | The new page | The new page in the sketch; it depends on the order of the timers | The new page, always: the drain runs before the layout | As B2 or B3 |
| A coroutine resumed by a click of the outgoing page | Its widgets are deleted while its code goes on | Its widgets exist until it waits again | The same | Safe for a real click; not for an event sent by code |
| Tests | Simple | Need a step of the loop, or a call that drains | The same | Two behaviours to test |
| Risk | A coroutine touches a deleted widget; a page built twice for nothing | The order of timers | A program with no display, or one that never refreshes | Behaviour that depends on who sent the event |

Measured in sketch (iii), on v9.6 and the v10 pin, with the same results.
A real click goes through LVGL's test input device, and the loop is stepped with `lv_tick_inc` and `lv_timer_handler`.
`lv_test_wait` is not used there: it forces a refresh at its end, which would drain whatever the wake did.

In the first run a plain handler of the button changes the state three times.

| Policy | Code after `current = settings;` sees | Builds for three changes | First frame after the change | Calls of ours that allocate |
|---|---|---|---|---|
| Immediate | settings | 3 | settings | 0 |
| Deferred, paused timer | home | 1 | settings | 0 |
| Deferred, refresh start | home | 1 | settings | 0 |
| Hybrid, real click | home | 1 | settings | 0 |
| Hybrid, event sent by code | settings | 3 | settings | 0 |

In the second run a task waits for the click, then changes the state itself and looks for a label of its own page.
The label is looked up by address in the widget tree: `lv_obj_is_valid` reads the widget, and crashed on the deleted one.

| Policy | Task resumed by the click | Its page's label still there after `current = settings;` | Task ended afterwards |
|---|---|---|---|
| Immediate | yes | no: deleted while the task's code goes on | yes |
| Deferred, paused timer | yes | yes | yes |
| Deferred, refresh start | yes | yes | yes |
| Hybrid, real click | yes | yes | yes |
| Hybrid, event sent by code | yes | no | yes |

In the third run a deferred switch is asked for where its holder cannot be drawn, and the loop runs 200 ms.

| Wake | Hidden holder | Holder sized by its content, showing an empty arm | Screen not loaded |
|---|---|---|---|
| Refresh, woken by `lv_obj_invalidate(holder)` | Not drained | Drained | Not drained |
| Refresh, woken by `lv_timer_resume(lv_display_get_refr_timer(disp))` | Drained | Drained | Drained |
| Paused timer of our own | Drained | Drained | Drained |

`lv_obj_invalidate` does nothing for a widget that is not visible, so it must not be the wake.

Recommended: B3, deferred and drained at the start of the display's refresh.

- It allocates nothing per switch, and one handler per display, made at the first mount on that display.
- It wakes the display's refresh timer itself, which drains also for a hidden holder or a screen not loaded.
- Whether a display has the handler is read from its own event list, so no flag of ours is kept per display.
- Several changes in one handler build once, with the last value.
- The new page is built before the next frame, so no frame shows a page that the state no longer names.
- A coroutine resumed by a click finishes its step with its page intact; the drain then cancels it, as it cancels a task today.
- `lv::settle()` drains at once: for tests, and for code that needs the new page in the same step.
- B1 stays available as an option of the node for a program that wants it: `.switch_at = lv::at_once`.
- B4 is rejected: its behaviour changes with who sent the event, and coroutines and timers are not covered.

What "the old page after `state = x;`" means for users.

- The state has the new value at once: `get()`, bindings and computed values see it.
- Only the shown tree follows a little later, before anything is drawn.
- `.on_shown` is the place for code that needs the new page's widgets.

## A style value that follows a state

The owner wants `.bg_color = lv::bind(accent)` in this round, without heap of ours.
Sketch (v) ran five ways on a button, 1,000 changes each, on v9.6 and the v10 pin with the same numbers.

| Way | Heap to attach | Heap of 1,000 changes | Domain | Where its data lives |
|---|---|---|---|---|
| A: one constant style per value, swapped with `lv_obj_replace_style` | 16 bytes, 0 blocks | 0 | Finite: an enum or a bool | Flash: N styles made at compile time |
| B: one style of ours, its value changed in place, `lv_obj_refresh_style` | 16 bytes, 0 blocks | 0 | Any value, also a colour computed from a number | The node's storage: `lv_style_t` and a property array, 48 bytes |
| C: the same, refreshed with `lv_obj_report_style_change` | 16 bytes, 0 blocks | 0 | Any | The same; the refresh walks every widget |
| D: a local style property, `lv_obj_set_local_style_prop` | 64 bytes, 2 blocks | 0 | Any | LVGL's heap |
| E: LVGL's `lv_obj_bind_style_prop` with a subject | 288 bytes, 7 blocks | 0 | Any | LVGL's heap |

The 16 bytes of A, B and C are one entry in the widget's style list.
The list grows by realloc, in a block the widget already has.
Every constant style of a tree costs the same today.

Recommended: B for every bound style value, A only as an optimisation for a small finite domain.

- B covers a continuous domain without a second mechanism.
- Its storage is a node's `storage` like any other: a watcher, the style, its property array.
- The style has `prop_cnt` 255, so LVGL never writes it, and we change the value in our memory.
- A needs only the watcher and the current index, with the N styles in flash.

Mode-aware tokens (`tok.surface` resolving through the active mode) need no storage per node.

- Each mode's styles are constant, built at compile time, in flash: one table per mode with the same order.
- A swap walks the widgets once and replaces each mode style with its counterpart (`lv_obj_replace_style`, 0 bytes per change in sketch (v)).
- The state of the mode belongs to `lv::display`, with one watcher there.
- It shares `lv_obj_replace_style` with way A, not the node storage: nothing per node changes.

## A region for the whole program

The owner wants, later, a memory layout made once at the start, in which everything else is built.
This section hands that question to its own research; it is not answered here.

What this mechanism already gives.

- The size and alignment of a tree's storage are constants: `sizeof(lv::mounted<ui>)` and `alignof`, or `lv::storage_size<ui>` as a name for them.
- A program can add up its screens in a constant expression and declare one `constinit` region.
  Sketch (vi) placed two screens in 296 bytes.
- Nothing assumes a placement: the same type works on the stack, as a global, as a member, and with `::new (place) lv::mounted<ui>(parent)`.
- `lv::mount_in<ui>(place, parent)` would be the same placement new with a name.
- Screens alive one at a time can share one region, as the arms of `lv::shown` share theirs: the union is the largest.

What is still open.

- LVGL's own heap for the widgets: its `LV_MEM_SIZE` pool, or a custom allocator over a region of ours.
  `memory.md` measured a bump arena at 15 to 30 percent more.
- The bytes a widget of each kind takes on LVGL's heap, per LVGL version, so that a region can be sized from a tree.
- Which screens are alive at the same time: a stack of overlays needs the sum, pages need the largest.
- Coroutine frames: their size is not a constant in C++.
  A slot of declared size, checked at run time, is the known way.
- Fonts and images: already in flash; a cache or a decoded image needs its own region.
- LVGL's own allocations that we cause: the root's handler, the display's handler, timers.

## The root watch without an allocation

Researched 2026-11-18, after the owner asked for a watch that allocates nothing.
A mounted tree must notice when its root goes from outside: its parent deleted through the C API, a screen replaced, the root deleted alone.
Otherwise a state change later writes into freed widgets.

### What LVGL offers

All rows are `source` in v9.6.0 and the v10 pin, unless marked.

| Facility | What it gives | Allocation |
|---|---|---|
| An `LV_EVENT_DELETE` handler on the root | The root's deletion, before its children go | One descriptor per root, and the event array grows |
| `LV_EVENT_DELETE` reaching the parent by bubbling | Nothing: `event_is_bubbled` never bubbles `LV_EVENT_DELETE`, `LV_EVENT_CHILD_DELETED` or `LV_EVENT_CHILD_CHANGED` | |
| `LV_EVENT_CHILD_DELETED` on the parent | Sent by `lv_obj_delete` after the child is gone, with no parameter, and not when the parent itself is being deleted | One descriptor per parent |
| `LV_EVENT_DELETE` on the parent | The parent's deletion, while its children still exist | One descriptor per parent |
| An event of the display or the application when any widget goes | None exists | |
| `lv_obj_class_t` hooks: constructor, destructor, event | The struct is in `src/core/lv_obj_class_private.h`: not public | |
| `lv_obj_set_external_data(obj, data, free_cb)` | `free_cb` runs in `lv_obj_destruct`, after the children are deleted | None, but only with `LV_USE_EXT_DATA`, default 0; every widget grows by two pointers |
| `lv_obj_free_id(obj)` with `LV_USE_OBJ_ID` and our own id functions | One global call for every deleted widget | None, but it takes over the program's widget ids |
| `lv_obj_null_on_delete(&p)` | Sets a pointer to null when the widget goes | It adds an event handler: one descriptor |
| `lv_obj_is_valid`, alias of `lv_obj_is_in_widget_tree` | Reads the widget's parent chain | Reads freed memory for a deleted widget; `measured`, sketch (iii) |
| A list of every screen of a display | Not public: `screens` is in the private display struct | |

So no public facility notices a deletion with neither an allocation nor a configuration of LVGL.
The smallest cost is one allocation per parent, not per mount.

### The options, measured

Sketch (ix) mounts a tree of four widgets with one binding, twice under one parent, then deletes things from outside.
`proto_watch.hpp` holds the ways; sketch (x) is the external data, with LVGL built again with `LV_USE_EXT_DATA=1`.
Every row ran on v9.6.0 and on the v10 pin, with the same numbers.
Heap is compared with the same tree mounted with no watch.
That costs 1,176 bytes in 18 blocks the first time, 1,032 in 16 the second.

| Way | Calls of ours that allocate: first mount, second mount under the same parent | Heap more than no watch: first, second | Parent deleted | Root deleted alone | Root deleted in its own event | Screen replaced with auto-delete | Our handlers run in 100 frames |
|---|---|---|---|---|---|---|---|
| W1 a handler on the root | 1, 1 | 48 B in 2 blocks, 48 B in 2 blocks | Noticed | Noticed | Noticed | Noticed | 0 |
| W2 two handlers per parent, shared | 2, 0 | 112 B in 3 blocks, none | Noticed | Noticed | Noticed | Noticed | 0 |
| W3 `LV_USE_EXT_DATA`, LVGL built with it | 0, 0 | 80 B, 0 blocks: 16 bytes per widget of the tree | Noticed | Noticed | Noticed | Noticed | 0 |
| W4 no watch | 0, 0 | none | Missed | Missed | Missed | Missed | 0 |

After each noticed deletion the state changed, and only a probe was told: no storage of the dead tree was still linked.
The second mount under W2 cost 24 bytes less than with no watch, in the same number of blocks: the parent's lists, not the watch.

How each one works, and its limits.

- W1 is what sketch (vi) measured: one `LV_EVENT_DELETE` handler per root, 48 bytes in 2 blocks on the host.
- W2 adds `LV_EVENT_DELETE` and `LV_EVENT_CHILD_DELETED` handlers to the parent, once, found again in the parent's event list.
  The trees are in one intrusive list of ours; each entry is two pointers and the parent and root, inside the `mounted` object.
  On the parent's deletion every tree under it ends its storage while its widgets still exist.
  On a child's deletion a tree whose root is no longer among the parent's children ends its storage; only addresses are compared.
  The handlers stay on the parent after the trees go, so mounting under it again costs nothing.
  They run only for those two events of the parent: LVGL filters by event code before calling.
- W3 is free per mount and needs no handler, but it is a configuration of LVGL that every widget pays for.
  The data is one per widget, so it must be the root's, and a user who also sets it on the root replaces ours.
- W4 is the rule "the parent outlives the `mounted`", with nothing to check it.
- A global hook through `lv_obj_free_id` would be one function for the program.
  It needs `LV_USE_OBJ_ID` and takes the ids from the user.
  Reasoned, not run.
- A holder that lvgl26 creates anyway: `lv::display` creates or owns its screens.
  It can put W2's handlers on them once, when the display starts.
  Then a tree mounted on a display's screen costs nothing at mount, and the program pays two descriptors per screen at its start.

With W2 and W3, the storage of a tree whose root went alone ends after its widgets are deleted.
So a kind's destructor must not touch a widget: the rule of "How it works" holds without exceptions.
`each_view::close()` today removes handlers from its rows; in the migration it lets them go with the rows.

Recommended: W2, with its handlers put on `lv::display`'s screens and layers when the display starts.

- A tree mounted on a screen of `lv::display` allocates nothing at mount.
  The handlers were made once at the start, as the owner's region goal wants.
- A tree mounted under any other parent pays two descriptors once for that parent, never again for it.
- W3 is used instead when LVGL is built with `LV_USE_EXT_DATA`: found by reflection, as `detail::find` finds optional functions.
- `watch::none` then means W4: no handler, not even on the parent, under the documented rule that the parent outlives the `mounted`.
  A checked build adds W1 to such a tree and reports a root that went first.
  That costs one descriptor, in that build only.

## Recommendation

### A: storage

1. The storage of a tree is one plain object, `storage_t<Tree>`, made by reflection from what each node kind declares.
2. A kind keeps run-time data by declaring `template <const auto& Tree, std::size_t... Path> struct storage;`, checked by a concept.
3. `lv::mounted<Tree>` contains it, is not movable, owns its widgets, and carries its own failure in the shape of `std::expected`.
4. `lv::storage_size<Tree>` and `alignof` are constants; `lv::mount_in` and `lv::place` take caller memory.
5. The root is watched through its parent: two handlers per parent, made at the start for `lv::display`'s screens.
   `watch::none` drops the watch, under a documented rule.
6. Arms share a union; with `.leave`, a region of the largest and the second largest arm.

### B: switch timing

Deferred, drained at the start of the display's refresh, with `lv::settle()` to drain at once.

### The smallest first slice

- `build_state` gets the address of the scope's storage; `tree.hpp` gets `any_storage`, `storage_t` and `locate`.
- `lv::mounted` becomes non-movable with the storage inside; `mount` returns it as a prvalue.
- `lv::shown` over an enum or bool `lv::state`, arms as trees, union storage, deferred switch drained at the refresh.
- Before it: `lv::state` gets its intrusive list with the safe walk, as sketch (viii) ran it, and keeps a subject only for `raw()`.
- Without that step every `lv::bind` inside an arm allocates on each switch.
  That is an observer, a delete handler and the text: 68 bytes per bound label on a Cortex-M4 (`memory.md`).
- Tests: the 2,000 switches with `lv_mem_monitor`, the coroutine click, the out-of-memory loop without LVGL's theme, `large_tree` against its limit.

### The migration path

1. `lv::state` with its own list of watchers, and bindings as storage nodes: the spike's work, with the safe walk.
2. `lv::shown` and `lv::when` on the new storage.
3. `lv::each`: its block moves into the storage; rows with storage get slots.
4. Formatted text of several values, with custom formatters.
5. A style value that follows a state: way B.
6. Animations of `.enter` and `.leave`, and task slots.

## What was compiled, and what was reasoned

| Claim | Compiled and run | Compiled only | Reasoned |
|---|---|---|---|
| Storage sized from kinds, a kind added without touching the mount code | Yes, both versions | | |
| Shown node, nested, with bindings in its arms; 4,000 switches; safe walk | Yes, both versions | | |
| One root handler is the only allocation of ours at mount; `watch::none` has none | Yes, both versions | | |
| Storage on the stack; two screens in a `constinit` region by placement new | Yes, both versions | | |
| The five switch policies, with a real click and a coroutine | Yes, both versions | | |
| The five ways of a bound style value | Yes, both versions | | |
| Out of memory at every point of a mount, nothing left | Yes, both versions, without LVGL's theme | | |
| The safe walk: unlinking the next watcher, itself, a nested change | Yes, both versions | | |
| Four ways to watch the root, including LVGL built with `LV_USE_EXT_DATA` | Yes, both versions | | |
| A watch through `lv_obj_free_id`; W2's handlers put on `lv::display`'s screens | | | Yes |
| The wake of a deferred switch for a hidden holder and a screen not loaded | Yes, both versions | | |
| Compile cost and operations of the storage computation | | Yes, v9.6 | |
| Arms of a leave played backwards from both ends of a region | | | Yes |
| Page functions with a declared size | | | Yes; the cost of a tree in a header measured |
| Handlers finding ids by a path | | | Yes; `row_view` runs the downward half |
| Formatted text of several values, custom formatters | | | Yes; one value measured |
| Migrating `lv::each`, task slots, animations in storage | | | Yes |
| Mode-aware tokens without storage per node | | | Yes; the swap call measured |
| Annotations as a registry | | | `recalled` |

## Not confirmed

- The byte counts on a Cortex-M4: every number here is from a 64-bit host.
- The 16 bytes that `watch::none` gained over 2,000 switches in sketch (vi), with no new block.
- The 8 bytes that sketch (ii) lost once over its first 2,000 switches, then never again.
- Whether the order of LVGL's timers can make B2 show one frame of the old page; the sketch never saw it.
- Whether GCC 16.2 supports annotations well enough for a registry.
- Whether Qt for MCUs uses ETL's pools for its items, or only ships the library.

## Decisions for the owner

1. Where does a tree's storage live?
   Options: S2 inside a non-movable `mounted`; S3 caller storage with a handle; S4 a static per tree.
   Recommendation: S2, with S3 for page functions and `lv::place`.
   Cost: `mounted` cannot be moved; `std::vector<mounted>` and returning a named one stop compiling.
2. What does `mount` return?
   Options: `mounted` carrying its failure, shaped like `std::expected`; the spike's two steps keeping `std::expected`; `page.mount(parent)` returning `std::expected<void, mount_error>`.
   Recommendation: `mounted` carrying its failure, plus `page.mount(parent)` for globals.
   Cost: code that names `std::expected<lv::mounted<ui>, lv::mount_error>` changes; `auto root = ...` does not.
3. How does a node kind declare its storage?
   Options: a nested template found by reflection; a trait; a concept; a function found by lookup.
   Recommendation: the nested template, checked by a concept.
   Cost: a kind's author writes a template with `Tree` and `Path`.
4. How does a mounted tree notice that its root went from outside?
   Options: W1 a handler per root; W2 two handlers per parent, shared; W3 LVGL's external data; W4 no watch, a rule.
   Recommendation: W2, put on `lv::display`'s screens at the start; W3 when LVGL has it; W4 as `watch::none`.
   Cost: two descriptors per parent other than a display's screen, 112 bytes in 3 blocks on the host, once.
5. A widget of a mounted tree deleted alone through the C API?
   Options: a rule, checked in a checked build; a handler per bound widget; LVGL's external data when enabled.
   Recommendation: the rule, checked in a checked build by our own address lookup: `lv_obj_is_valid` reads the widget.
   Cost: such a deletion is a bug that a release build does not see.
6. How large is the region of `lv::shown`'s arms?
   Options: the union; the largest plus the second largest; two slots of the largest; the sum.
   Recommendation: the union without `.leave`, the largest plus the second largest with it.
   Cost: a node with `.leave` takes up to twice the RAM of one arm.
7. How does a page function from another file give its size?
   Options: its tree in a header and its build in its file; a declared size checked there; a static storage of its own.
   Recommendation: a declared size, `lv::page<&home_page, 256>`, checked in the page's file.
   Cost: a number to keep up to date by hand; the check says the right one.
8. When does a switch happen?
   Options: immediate; deferred to a paused timer; deferred to the display's refresh; hybrid.
   Recommendation: deferred to the display's refresh, with `lv::settle()`, and immediate as an option of the node.
   Cost: code after `state = x;` sees the old page; one handler per display.
9. How does a style value follow a state?
   Options: way B, a style of ours changed in place; way A for finite domains; LVGL's binding.
   Recommendation: B, with A as an optimisation later.
   Cost: 48 bytes of storage per bound style value on the host, and 16 bytes of LVGL's style list.
10. Does `lv::state` get its own list of watchers before `lv::shown`?
    Options: before, as the spike did; after `lv::shown` lands.
    Recommendation: before: without it every binding inside an arm allocates on each switch.
    Cost: C code reaches a state only through `raw()`, which makes a subject on demand.
11. Should we report `lv_obj_add_style`'s unchecked realloc upstream?
    Options: report it with the failing case; work around it in our tests only.
    Recommendation: report it, and keep our out-of-memory tests without LVGL's theme until it is fixed.
    Cost: an issue to follow on both LVGL versions.

## What the owner decided

Decided 2026-11-18, over the list above.
The number is the decision it answers.

| # | Decision |
|---|---|
| 1 | A tree's storage lives inside a non-movable `mounted`. Caller memory goes through `lv::place` and `lv::mount_in`. Page functions use storage they declare. |
| 2 | `mounted` carries its own failure: `if (!root)`, `root->`, `root.error()`. A global uses `page.mount(parent)`. |
| 3 | A kind declares a nested `storage<Tree, Path...>` template, checked by a concept. |
| 4 | Not decided: the owner asked for an allocation-free watch first. "The root watch without an allocation" is that research. |
| 5 | A widget deleted alone through the C API is under a documented rule, checked in a checked build with our own address lookup. |
| 6 | The arm region is the largest arm, or the largest plus the second largest with a leave animation. |
| 7 | A page function's size is declared by the page and checked in its own file. |
| 8 | The switch is deferred to the display's next refresh, with `lv::settle()`. A node may opt into immediate. |
| 9 | A bound style value keeps its `lv_style_t` in the node's storage, changed in place. |
| 10 | `lv::state` gets its own watcher list, as the first slice. |
| 11 | The LVGL bug is drafted as an upstream report for the owner to file: `docs/research/lvgl-upstream.md`. |
