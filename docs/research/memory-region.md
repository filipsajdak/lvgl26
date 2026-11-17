# A memory layout reserved once at the start

Research and design of 2026-11-18, against `origin/main` at `747c741`.
Marks follow `docs/research/README.md`.
Every mark is dated 2026-11-18 unless its row says otherwise.

The owner's words: "Eventually, we should allow designing memory layout that can be allocated once on the beginning of the program (eg. Using placement new) and later it might be used to build everything else inside it - research how that can be done."
His standing goals apply too.
Write less code with more meaning, give the compiler facts, size buffers at compile time, avoid run-time allocation, stay open-closed.

This note designs the layer above the storage of one tree.
[node-storage.md](https://github.com/filipsajdak/lvgl26/blob/docs/design-node-storage/docs/research/node-storage.md) (pull request 101, branch `docs/design-node-storage`) designs that storage.
It recommends a non-movable `lv::mounted<Tree>` whose size is a constant, `lv::storage_size<Tree>`, placeable on the stack, as a global, or by placement new.
Its section "A region for the whole program" leaves six questions open; this note answers them.
[memory.md](memory.md) holds the per-call heap costs and the survey of other frameworks; they are not repeated here.

How the work was done.

- Sketches live in a scratch directory outside the repository, as in `memory.md`.
- Host: GCC 16.2 on macOS arm64, the `host-v96` flags with `-Werror`, LVGL v9.6.0, `-O2`.
- Arm: arm-none-eabi GCC 16.2, Cortex-M4, `-Os`, LVGL v9.6.0, run in QEMU `mps2-an386`, built from the `arm-m4` preset's library.
- LVGL's heap was counted by a replacement of `lv_mem_core_builtin.o` with the same TLSF calls plus counters, linked ahead of `liblvgl.a`.
  It uses LVGL's private `lv_tlsf.h`, so it is a measuring tool only.
- The C++ heap was counted by a replaced `operator new`, the process heap by `malloc_zone_statistics` (host) and `mallinfo` (Arm).
- A deliberate `new int` and `malloc(100)` moved both counters on both targets, so their zeros below are real.
- "Bytes" of LVGL's heap are payload bytes, as `lv_mem_monitor` counts them: without block headers and without TLSF's own tables.

## Short answers

| Question | Answer | Evidence |
|---|---|---|
| 1. Who allocates in an lvgl26 program today? | In the measured program, only LVGL's heap: no `operator new`, no `malloc`. The hosted tools `lv::mcp` and `lv::encode_png` use the system heap. | Inventory, both targets; grep of `include/lv` |
| 1. When? | At start (6.1 KB on the M4), at mount, per list row, and on every frame that redraws something. A steady screen allocates nothing. | Inventory |
| 2. Can widget memory be computed at compile time? | Not from the public API: every widget struct is an incomplete type there, on v9.6.0 and the v10 pin. A measured bound, checked by a test, is the way. | Reflection probe, bisection |
| 3. How should a program declare its memory? | As one typed layout: `lv::memory<parts...>`, its size a constant, each part a type of its own, LVGL's heap one of them. | Sketch (b), (c) |
| 3. Can LVGL's heap live inside that layout without macros in our code? | Yes, on both versions: `LV_MEM_POOL_INCLUDE` and `LV_MEM_POOL_ALLOC` set from CMake point LVGL's own memory core at our region. | Sketch (c), source |
| 4. Can coroutine frames come from a fixed pool? | Yes: three frames from a pool, nothing of them on LVGL's heap. GCC gives no frame size in the language, but writes it into DWARF. | Sketch (d), probes |
| 5. Fonts, images, draw buffers? | Fonts and images the compiler makes are constants in flash. TinyTTF's cache and a PNG decode need LVGL's heap: 1.3 KB kept, 164 KB for a moment. | Inventory |
| 6. How does a project prove "no allocation after start"? | Two claims, not one: no system heap (checked at link time), and LVGL's heap a fixed part whose peak a test bounds. | Inventory, link map |
| 7. First slice? | LVGL's heap as a part the program declares and `lv::application` takes. It moves 16 KB out of LVGL's hidden static into a named object. | Sketch (c) |

## The inventory

One program runs four screens in turn.
They are the slice (column, label, arc, bound label, a task) and the list of `examples/lists`.
Then come a label with a TinyTTF font, and a PNG kept as a file next to an animated arc.
Cortex-M4, LVGL v9.6.0, `LV_MEM_SIZE` 256 KiB.
"Kept" is the change in LVGL's heap over the phase; "peak" is the highest point above its start; "calls" are `lv_malloc` plus `lv_realloc`.
`operator new` and `malloc` were 0 in every row.
A counter of failed allocations stayed at 0.

### By phase

| Phase | Kept | Peak | Calls | Mark |
|---|---|---|---|---|
| `lv::application` (`lv_init`) | 2104 | 2104 | 8 | `measured` |
| `lv::display`, 320 x 240 frame | 3988 | 3988 | 316 | `measured` |
| First frame of an empty screen | 0 | 176 | 1 | `measured` |
| Mount the slice | 1032 | 1032 | 50 | `measured` |
| First frame of the slice | -28 | 1476 | 34 | `measured` |
| Ten more frames, nothing changes | 0 | 0 | 0 | `measured` |
| `lv::spawn` of a task: frame 92, sleep timer 36 | 128 | 128 | 2 | `measured` |
| `lv::spawn` with an owner widget | 168 | 168 | 5 | `measured` |
| 500 ms: three sleeps, three changes, redraws | -124 | 1544 | 122 | `measured` |
| One change of a bound text of the same length | 12 | 12 | 3 | `measured` |
| Redraw after the change | -12 | 432 | 15 | `measured` |
| Mount the empty list | 1020 | 1020 | 63 | `measured` |
| `push_back` of one row (button, two labels) | 764 | 764 | 45 | `measured` |
| `update` of one row's bound fields | 36 | 36 | 4 | `measured` |
| `move` of a row | 0 | 0 | 0 | `measured` |
| Select a row (checked state) | 164 | 804 | 8 | `measured` |
| Erase every row, then unmount | -3928 | 0 | 9 | `measured` |
| Mount a label with a TinyTTF font | 884 | 884 | 35 | `measured` |
| Its first frame: glyphs drawn into the cache | 1296 | 5316 | 75 | `measured` |
| Resize the font, redraw | 468 | 4648 | 80 | `measured` |
| Mount an image kept as a 953-byte PNG (128 x 128) | 604 | 604 | 36 | `measured` |
| Its first frame: the PNG decoded, drawn, freed | 0 | 164400 | 58 | `measured` |
| Its second frame, nothing changed | 0 | 0 | 0 | `measured` |
| `lv::spawn` of a task that runs `lv::animate`: frame 536 | 684 | 684 | 5 | `measured` |
| The 300 ms animation runs and ends | -684 | 1108 | 162 | `measured` |
| An arc at half opacity, and its frame (a layer) | 4 | 1104 | 28 | `measured` |
| Whole run: 794 malloc, 643 realloc, 861 free | | 173560 | | `measured` |

The host gives the same calls with larger blocks: `lv_init` 3272, the display 6176, the slice 1576, a task frame 176.

### By allocator

| Who allocates | Bytes (Cortex-M4) | When | Size known at compile time? | Mark |
|---|---|---|---|---|
| `lv_init`: LVGL's globals, timer list, the first pool's records | 2104 | Start | Constant per LVGL version and configuration: measured once | `measured` |
| The PNG decoder's registration (`LVGL26_PNG`) | 36, one call | Start | Constant | `measured`: the one difference between two builds, 2068 and 2104 |
| The display, with LVGL's default theme (2532 of it) | 3988 | Start | The same; `LVGL26_NO_DEFAULT_THEME` removes the theme | `measured`; theme from `memory.md` |
| The frame or band to draw into | 153600 or 6400 | A `constinit` object of the program | Yes: `sizeof(lv::frame<...>)` | `measured` |
| Widgets: struct, `spec_attr`, child, style and event lists | About 200 to 250 per widget | Mount, and per list row | No: the structs are private. Bounded by measurement | `measured`; per kind in `memory.md` |
| Text copies of constant labels | 16 or more per label | Mount | The text length is; the copy can go (`memory.md` row 1) | `source` |
| LVGL's subject and observer of a bound state | 44 per state, about 64 per bound label | First binding, mount | Removed by the spike in `mounted-storage` | `memory.md`, spike |
| Bound text formatted on the heap | 12 kept, 3 calls per change | Every change | Removed by node storage: the buffer is sized from the format | `measured` |
| Drawing: draw tasks, masks, layers | 176 to 1544 peak, 0 kept | Every frame that redraws | No: depends on what is invalid. Bounded by measurement | `measured` |
| A layer for opacity or a transform | 1104 peak | While drawn | No | `measured` |
| `lv::task` frame | 56 to 536 (ports: 80 to 424) | Spawn, and each awaited task | Not in C++; GCC writes it into DWARF | `measured` |
| Sleep timer, widget watch, delete handler of a task | 36, 16, about 24 | Spawn and each wait | Constant per kind | `measured` |
| Animation descriptor | 104 | While it runs | Constant | `memory.md` |
| `lv::list` model | 128 for 8 rows | A `constinit` object of the program | Yes | `measured` |
| TinyTTF font object and glyph cache | 884, then 1296 kept and 5316 peak | First use, first draw, resize | The cache size is a setting of LVGL; the peak is measured | `measured` |
| PNG decode (lodepng), no image cache | 164400 peak for 128 x 128 | Each draw of the picture | Width x height x 4, plus the inflate buffers: measured | `measured` |
| `operator new`, `malloc` in lvgl26 | 0 | Never | | `measured`, with a probe that moved both counters |
| newlib's `malloc` | Linked into the slice | First `printf` (stdio's buffers) | Not ours: `findfp.o` refers to it | `source`, `slice.map` |
| `lv::mcp::server`: requests and answers as `std::string` | Per request | While an agent is attached | No | `source`, `include/lv/mcp.hpp` says "Hosted: it allocates" |
| `lv::encode_png`, `lv::write_png`: a `std::vector` of the encoded file | Per call | Tests and screenshots | No | `source`, `include/lv/png.hpp` |
| `lv::runtime<...>::dump`, `lv::json_writer` | None of ours: they write into the caller's sink | Per call | | `source` |
| Everything else in `include/lv` that names `std::vector` or `std::string` | None at run time: consteval code | Compile time | | `source`, a grep of the headers |

### What the numbers say

- Everything lvgl26 allocated in this program was inside LVGL's heap.
  So placing that one heap places the program's dynamic memory; the hosted tools stay outside.
- A steady screen allocates nothing: ten frames without change made 0 calls.
- A frame that redraws allocates and frees: 15 to 122 calls, nothing kept.
  "No allocation after start" can therefore not mean "no call of `lv_malloc`".
- The smallest pool the slice still passes with is 10240 bytes, found by bisection in QEMU (fails at 10176).
  The slice checks its own results and prints "ok".
- The whole program without the picture needs 15488 bytes with no failed allocation; its payload peak is 13196.
- With the picture it needs 189760; its payload peak is 173560.
- At 100000 bytes exactly one allocation of the decode failed.
  The frame finished, the program went on, and nothing retried the picture: the next frame made no call.
  Whether any of its pixels showed was not checked.
- TLSF keeps its tables in the pool too: about 1.1 KB at 16 KiB, 1.6 KB at 256 KiB.
  Both come from the formula in `lv_tlsf.c`.
  The region sketch saw `lv_mem_monitor` report 15264 of a 16384-byte pool, which fits.

## 2. Can a tree's widget memory be bounded at compile time?

| Finding | Mark |
|---|---|
| `lv_label_t`, `lv_obj_t` and `lv_obj_class_t` are incomplete types in LVGL's public headers, v9.6.0 and the v10 pin. A `static_assert` on `std::meta::is_complete_type` says so. | `measured` |
| `instance_size` exists only in `src/` (`lv_obj_class.c`, private headers), on both versions. `lv_obj_get_class` returns an opaque pointer. | `source` |
| So reflection cannot read a widget's size through the public API. The project rule forbids the private headers. | `source` |
| What else a widget costs is not a fixed sum either: `spec_attr` on the first child or event, lists grown by `realloc`, a style per selector, TLSF headers and rounding. | `memory.md`, `source` |
| The smallest working pool is above the payload peak: 15488 against 13196, 189760 against 173560. Headers, TLSF's tables and fragmentation make the difference. | `measured` |

Three ways to a bound, compared.

| Way | What it gives | Cost | Verdict |
|---|---|---|---|
| Exact sizes by reflection | Nothing: the types are incomplete | | Not possible |
| A table of per-kind sizes, written by a tool from the private headers into a header | A worst case from the tree at compile time | A tool that reads private headers per version and configuration; the sum still misses fragmentation and drawing | Possible, but the number is not the one that matters |
| A measured budget, checked by a test on the host and in QEMU | The real smallest pool, with drawing and fragmentation in it | One test per program; numbers differ by target | Recommended |

The budget test is the bisection above, run in the nightly QEMU job.
It fails when the program no longer passes with the pool it declares.
A cheaper form runs on every pull request: `lv_mem_monitor`'s `max_used` after a scripted walk through the screens, against the declared part.

## 3. How a program declares its memory

### Where LVGL's heap can come from

| Mechanism | Works without macros in our code? | What happens | Mark |
|---|---|---|---|
| `LV_MEM_POOL_INCLUDE` and `LV_MEM_POOL_ALLOC`, set from CMake | Yes: two compile definitions of the `lvgl` target | `lv_init` calls our function once and builds its one TLSF pool there. LVGL's static array is gone from the image. | `measured`, both targets |
| `lv_mem_add_pool` after `lv_init` | Yes: a public function | LVGL's static `LV_MEM_SIZE` array stays, and is used first. The M4 image grew from 24396 to 40780 bytes of `.bss`. | `measured` |
| `LV_MEM_ADR` | No: a number in an `#if`, so a fixed address in the linker script | One pool at that address | `source` |
| `LV_STDLIB_CUSTOM`, our own `lv_malloc_core` | Needs a C or C++ file that defines eight functions | Any allocator: arenas, several pools | `memory.md` measured an arena |

Details that bind the design.

- `LV_MEM_POOL_INCLUDE` and `LV_MEM_POOL_ALLOC` are read only in `lv_mem_core_builtin.c`; `lv_conf_internal.h` does not touch them, so `LV_CONF_SKIP` keeps them.
  The same holds on both versions (`source`).
- TLSF sizes its tables from `LV_MEM_SIZE`, and refuses a pool of `2^ceil(log2(LV_MEM_SIZE))` bytes or more.
  `source`, `lv_tlsf.c`
- So the heap's size has one source, the CMake setting `LVGL26_LVGL_HEAP_BYTES`, and the layout reads `LV_MEM_SIZE` as a constant, as `display.hpp` reads `LV_DRAW_BUF_ALIGN`.
  A part that states another size is a compile error.
- `lv_mem_core_builtin.c` and `lv_tlsf.c` are identical in v9.6.0 and the v10 pin.
  `source`, `memory.md`
- The hook is one C function with C linkage.
  lvgl26 is header-only, so CMake adds a small C file to the `lvgl` target when the option is on.
  The function returns a pointer that `lv::application` sets before `lv_init`.

### Three alternatives

**A. A typed layout.**
The program lists its parts in a type, and the compiler computes the offsets and the total.
Each part is placed by placement new, at start or at mount.

```cpp
[[gnu::section(".bss.ui")]] constinit lv::memory<
    lv::within<48 * KiB>,                              // the RAM the board gives the UI
    lv::lvgl_heap,                                     // LVGL's own heap: LVGL26_LVGL_HEAP_BYTES
    lv::band<320, 240, 10>,                            // where LVGL draws
    lv::one_of<lv::page<home>, lv::page<settings>>,    // one screen at a time
    lv::tasks<4, 128>> memory;                         // four task frames of up to 128 bytes
```

**B. One arena with a budget, carved at start.**
The program declares bytes; a start function takes parts out of it in a fixed order, by placement new.

```cpp
[[gnu::section(".bss.ui")]] constinit lv::arena<48 * KiB> memory;
int main() {
  lv::application app{memory.take(lv::lvgl_heap{})};       // in order; checked at run time
  auto& band = memory.make<lv::band<320, 240, 10>>();
  auto& ui = memory.make<lv::mounted<home>>(display.screen());
}
```

**C. A per-screen arena, reset when the screen goes.**
LVGL's allocator is switched to the screen's bump arena while it is built, through `LV_STDLIB_CUSTOM`.

```cpp
[[gnu::section(".bss.ui")]] constinit lv::arena<12 * KiB> home_memory;
auto ui = lv::mount<home>(display.screen(), home_memory);    // LVGL's blocks for it come from here
```

### Compared against the owner's criteria

| | A. Typed layout | B. Arena carved at start | C. Per-screen arena |
|---|---|---|---|
| Allocations after start | None of ours. LVGL's heap is a fixed part: blocks come and go inside it. | The same as A | LVGL's blocks of a screen go to its arena. Frees while drawing are ignored, so a long-lived screen grows. |
| RAM | Parts plus padding. Screens alive one at a time share the largest. | Parts plus padding, and what the order wastes | 15 to 30 percent more than TLSF (`memory.md`); drawing still needs a common heap |
| At the use site | One declaration lists everything; `memory.mount<home>(...)` | Bytes, then a sequence of calls that must match the parts | One arena per screen; a size to find per screen |
| Facts the compiler gets | Every part's size and offset, the total, which tree goes where, the budget | Only the total | Only each arena's size |
| Too big | Compile error with every part named (measured) | A contract at start | A failed allocation inside LVGL while it builds |
| A tree without a place | Compile error (measured) | A contract | Falls back to the common heap, or fails |
| Open-closed | A part is any type with `bytes`, `align`, `name`; its own `start` hook. No central list. | Any type can be made | Only LVGL's allocations |
| Maintainability | One type to read; `lv::dump` prints the map | Order-dependent code to keep in step | A custom allocator to maintain, and sizes per screen |
| API change | `lv::application app{memory};` and `memory.mount<ui>(...)`; the old forms stay | The same calls, with an arena | A second argument to `mount`, LVGL built with `LV_STDLIB_CUSTOM` |
| Risk | Depends on node storage's non-movable `mounted`; a long template error if a part is wrong | Errors move from compile time to run time | `realloc` is a third of all calls; a mount inside a mount is untested |

### The layout, as sketched

| What | How it works | Mark |
|---|---|---|
| A part | A type with constants `bytes`, `align` and `name`, checked by a concept. | `measured` |
| The total | Offsets and the total are computed in a constant expression from the parts, in their order. | `measured` |
| Screens one at a time | `one_of<...>` is the largest part plus a pointer to the destructor of the one alive. Mounting another ends it first. | `measured`: settings took home's address |
| Screens alive at once | Listed one after the other: the sum. Overlays use this. | `measured` (sum of parts) |
| A screen's size | `sizeof` of what `lv::mount<Tree>` returns. With node storage, `lv::storage_size<Tree>`. | `measured` |
| Any object of the program | `object<T>`: a band, a frame, a model, built in its place at start. In the library a plain type that is not a part would mean `object<T>`. | `measured`; the plain form reasoned |
| The budget | `within<N>` has no bytes. A layout over it fails to compile, and the message names every part with its size and offset. | `measured` |
| A tree without a place | `mount<Tree>(memory, parent)` checks `has<screen<Tree>>` in a `static_assert`. | `measured` |
| Alignment | The region takes the largest alignment of its parts; each part is rounded up to its own. | `measured` |
| Where the region lives | A `constinit` global is NOBITS `.bss`: 23208 bytes, nothing stored in the image. The whole program had 24396 of `.bss` and 1252 of `.data`. A user's section `.bss.<name>` places it. | `measured`; section rule from `memory.md` |
| The stack of `main` | Works for a small layout; on a microcontroller 23 KB of stack is rarely there. | reasoned |
| A user's buffer (`std::span`) | Loses the constant size: it becomes design B. | reasoned |
| The map | A function prints each part, its offset and size: the base of an `lv::dump` of memory. | `measured` |

The two compile errors, as GCC 16.2 printed them.

```text
layout.hpp:157:21: error: static assertion failed: lv: the memory layout needs 8395856 bytes, but only 65536
are given (lvx::within). Its parts: budget 0 at 0; LVGL heap 8388608 at 0; object 6404 at 8388608;
one of 56 at 8395016; task frames 771 at 8395072;
layout.hpp:260:29: error: static assertion failed: lv: this tree has no place in the memory layout. Add
lvx::screen<tree> to lvx::memory<...>, or to an lvx::one_of<...> in it.
```

The second is followed by two errors from `std::tuple_element`; the library would stop after the first.
The sketch's `start` names the frame pool by an `if constexpr`; in the library each part brings its own `start` hook instead.

### Who ends the parts

A static layout outlives `main`, so it outlives `lv::application`.
The project's rule "declare the application first, what comes after ends before it" does not hold for it.
The region sketch ended no screen: `lv_deinit` took the widgets, and the screen's destructor never ran.

| Option | Order at the end | Cost | Mark |
|---|---|---|---|
| The application ends the layout | `lv::application app{memory};` keeps a pointer. Its destructor ends tasks, then screens in reverse order, then calls `lv_deinit`. The heap part is plain bytes and needs no end. | One pointer in the application; the layout gets an `end()` | reasoned |
| The layout is a member of the application | `lv::application<layout>`: the region lives where the application lives | The region on the stack of `main`, or a `constinit` application | reasoned |
| No end at all | Firmware that never returns from `run()` needs none; a host test does | Leaks under a sanitizer, and a screen's destructor never runs | `measured`: the sketch |

Recommended: the application ends the layout.
It keeps `constinit` placement, and a program that never ends pays one pointer.

### How it reads in a program

```cpp
#include <lv/lv.hpp>
using namespace lv::units;

constinit lv::state ticks{lv::counter{}};
constexpr auto home = lv::column({.size = {320 * px, 240 * px}})[
    lv::arc({.id = "speedo", .range = {0, 240}}), lv::label({.text = lv::bind(ticks, "{} ticks")})];
constexpr auto settings = lv::column({.size = {320 * px, 240 * px}})[lv::label({.text = "Settings"})];

[[gnu::section(".bss.ui")]] constinit lv::memory<
    lv::within<32 * KiB>, lv::lvgl_heap, lv::band<320, 240, 10>,
    lv::one_of<lv::page<home>, lv::page<settings>>, lv::tasks<2, 128>> memory;

lv::task<> drive();

int main() {
  lv::application app{memory};                       // LVGL's heap and the task frames live in memory
  lv::display display{app, memory.get<lv::band<320, 240, 10>>(), &send_to_panel};
  memory.mount<home>(display.screen());
  lv::spawn(drive());
  return app.run();
}
```

The walkthrough can then say: "this panel reserves `sizeof(memory)` bytes at start, and never asks for more."

## 4. Coroutine frames

| Finding | Mark |
|---|---|
| `lv::task`'s promise has `operator new`, which calls `lv_malloc`. A frame that does not fit makes `lv::spawn` return false. | `source`, `task.hpp` |
| A patched promise took its frames from a pool part instead. Three frames, one of them an awaited task, came from the pool; LVGL's heap grew only by the sleep's 36-byte timer. | `measured`, both targets |
| The timer, the widget watch and its delete handler still go to LVGL's heap. A pool removes the frame only. | `measured` |
| A task that awaits a task holds two frames. A pool counts frames, not spawns. | `measured` |
| An awaited `task<int>` whose frame was refused ends in a violated precondition of `await_resume`. | `measured` |
| An awaited `task<void>` without a frame is skipped: `await_ready` is true, and the awaiting task goes on as if it had run. | `source`, `task.hpp` |
| Both are what `task.hpp` does when `lv_malloc` returns null; a pool only makes it likelier. | `source` |
| GCC 16.2 never elided a frame: one allocation each at `-O0`, `-O2` and `-O3`, for a coroutine made and finished in one expression. | `measured` |
| GCC has no `__builtin_coro_size`; the only coroutine option is `-fcoroutines`. | `measured` |
| A GCC maintainer called HALO unsupported and the report an enhancement (bug 100579, 2021). Bug 115309 (2024) found the same for a generator. | `summary`: [100579](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100579), [115309](https://gcc.gnu.org/pipermail/gcc-bugs/2024-May/867270.html); bugzilla refused the fetch |
| GCC writes each frame into DWARF as a type `<mangled name>.Frame` with its `byte_size`. It equals what `operator new` receives: 176 for `run` on the host, 92 on the M4. | `measured` |
| So a frame's size is known after compiling with `-g`, not in the language: a tool can read it from the objects and fail the build. | `measured` |
| A frame's size depends on the target: `run` is 176 bytes on the host and 92 on the M4. | `measured` |
| `lv::animate` makes a frame large: 536 bytes for one arc. Its argument struct lives in the frame (`cpp26.md`). | `measured` |

Frames of the two ports on the Cortex-M4, `-Os`, read from DWARF.

| Port | Task | Frame bytes |
|---|---|---|
| Slint printer | `run_queue` | 84 |
| Slint printer | `delete_on_tap` | 80 |
| Slint printer | `rise` | 100 |
| Slint printer | `hide_when_empty` | 152 |
| Slint printer | `slide_in`, `slide_out` (animations) | 416 |
| Slint printer | `fill` (animation) | 424 |
| Qt treadmill | `run_clock`, `run_auto` | 84 |
| Qt treadmill | `remove_splash` | 88 |
| Qt treadmill | `follow_runner` | 96 |
| Qt treadmill | `follow_look` | 152 |

`usb_page.cpp` and `splash_page.cpp` need the JPEG decoder and were not compiled for Arm.

What follows for the design.

- A pool part `lv::tasks<Count, Slot>` has slots of one size and a flag per slot.
  Both are constants.
- A frame larger than its slot is a bug of the program: a contract in `operator new`.
- A full pool is a run-time condition: `lv::spawn` returns false, as it does today.
- An awaited task without a frame needs a defined result instead of a violated precondition.
- A build-time check reads the `.Frame` sizes from the objects and fails when one exceeds the slot.
  It runs on the `-g` build of each target.
- Slots of different sizes (`lv::tasks<2, 128, 1, 512>`) keep animations from setting the size of every slot.
- Shrinking `lv::animate`'s frame is worth doing first: it is the largest frame in both ports.

## 5. Fonts, images and draw buffers

| Kind | Where it is today | Part needed? | Mark |
|---|---|---|---|
| A font the compiler draws (`lv/font.hpp`) | Constants in flash | No | `source` |
| An image the compiler decodes (`lv/image.hpp`) | Constants in flash; LVGL draws in place | No | `source`, `images.md` |
| The frame or band | A `constinit` object of the program | Yes, as `object<T>`; it must stay where DMA reaches | `measured`, `memory.md` |
| TinyTTF (`lv::scaled_font`) | LVGL's heap: 884 for the font, 1296 of glyphs kept, 5316 peak while drawing | Inside the LVGL heap part, sized by the budget test | `measured` |
| A PNG kept as a file (`lv::encoded_image`) | LVGL's heap while drawn: 164400 for 128 x 128; no cache, so again at every draw | Inside the LVGL heap part; it dominates the budget | `measured` |
| A PNG that does not fit | One allocation fails, the frame ends, nothing retries; no error reaches the program | A rule: the budget test must draw every picture | `measured`; the pixels were not checked |
| Draw tasks, masks, layers | LVGL's heap, freed after each frame | Inside the LVGL heap part | `measured` |

A separate region for a decoder or a font cache would need LVGL to take an allocator per use, which it does not.
So these stay in the one LVGL heap part, and the budget test covers them.

## 6. Proving "no allocation after start"

| Claim | How it is checked | Can fail by | Mark |
|---|---|---|---|
| The system heap is not used | The Arm link map has no `_malloc_r` or `operator new`, or names only allowed referrers | stdio: the slice links `_malloc_r` through newlib's `findfp.o`, for `printf` | `source`, `slice.map` |
| lvgl26 calls no `operator new` | A test replaces `operator new` and fails on any call | A container in user code | `measured`: 0 calls |
| LVGL's heap is the declared part | The pool's address lies in the layout (`lv_malloc` of a probe) | A build without the hook | `measured`: yes |
| LVGL's heap stays inside its bound | `max_used` of `lv_mem_monitor` after a scripted walk, against the part; the nightly QEMU bisection for the true minimum | A heavier screen, a larger picture | `measured`: 10240 for the slice |
| No growth over time | The heap after one cycle of every screen equals the heap after two | A leak, a cache that grows | `measured`: unmount returns to the start, except caches |
| No allocation by lvgl26 after start | An LVGL hook cannot tell drawing from us, so it is not checked as a call count | | `measured`: drawing makes 15 to 122 calls per redraw |

A hook that fails on any `lv_malloc` after `app.started()` would fail on every redraw.
The sentence a port can honestly write is "reserves N bytes at start and never asks for more".

What QEMU gives: the real Arm sizes, the bisection of the pool, `arm-none-eabi-size` and `nm -S` of the region, and the link map.
It does not give timing or DMA.

## Other frameworks (gaps of `memory.md`)

| Finding | Link | Mark |
|---|---|---|
| AUTOSAR C++14 A18-5-7: without a known worst-case time, allocate and free only in non-realtime phases, "e.g. during initialization". | [A18-5-7](https://sbmueller.de/autosar_cpp_guidelines/A18-5-7.html) | `source` (an unofficial transcription) |
| AUTOSAR A18-5-5: memory management must be deterministic, without fragmentation, and not run out. | [A18-5-5](https://sbmueller.github.io/autosar_cpp_guidelines/A18-5-5.html) | `summary` |
| MISRA C++:2023 rule 21.6.1 (advisory): dynamic memory should not be used; standard containers count. | [Polyspace](https://www.mathworks.com/help/bugfinder/ref/misrac2023rule21.6.1.html), [Sonar](https://rules.sonarsource.com/cpp/tag/misra-c++2023/M23_003) | `summary` |
| ETL's `etl::pool` is a fixed-capacity pool sized by type parameters; an item larger than its slot is an assertion, not a fallback. | [pool.h](https://sources.debian.org/src/etlcpp/20.44.2%2Bdfsg-2/include/etl/pool.h) | `summary` |
| Qt for MCUs ships ETL in its automotive example. | [Qt snapshot](https://doc-snapshots.qt.io/qtformcus-2.5/quickultralite-automotive-src-3rdparty-etl-include-etl-pool-h.html) | `summary`; the page returned 404 |
| RTIC computes resource ceilings at compile time and argues against a heap for real-time systems. | [RTIC book](https://git.pfzetto.de/mirror/rtic/raw/branch/gh-pages/2/book/en/index.html) | `summary` |

The pattern across them is design A: sizes in types, one reservation at start, a slot size that is checked, no fallback to a heap.

## Recommendation

Design A, a typed layout, built on node storage's non-movable `lv::mounted<Tree>`.
LVGL's heap is one part of it, kept as TLSF, and bounded by a test.

Why A over B and C.

- It gives the compiler every size and every placement, so the two common mistakes are compile errors.
- It reads as one declaration: the program's memory is visible in one place.
- It keeps TLSF for LVGL, which `memory.md` measured as cheaper than any arena.
- It is open-closed: a part is a concept, and a new consumer brings its own type.

### Slices

| # | Slice | What a port's walkthrough can say then |
|---|---|---|
| 1 | LVGL's heap as an object of the program. CMake sets `LV_MEM_POOL_INCLUDE` and `LV_MEM_POOL_ALLOC` and adds the hook file; `lv::application app{heap};` takes `constinit lv::lvgl_heap heap;`, sized by `LVGL26_LVGL_HEAP_BYTES`. `app.heap()` gains the peak. | "The panel's memory is 16 KB of LVGL heap and a 6.4 KB band, both declared in `main.cpp`; nothing else is allocated." |
| 2 | The budget test: a scripted walk on the host checks the peak against the part; the nightly QEMU job bisects the smallest pool and fails above the part. A link check lists who refers to `_malloc_r`. | "It needs 10240 bytes at its worst, measured on a Cortex-M4 each night." |
| 3 | `lv::memory<parts...>` with `within`, `lvgl_heap`, `object<T>`, `page<Tree>` and `one_of`, after node storage's slice 1. `memory.mount<ui>(...)`. Both compile errors, with tests in `tests/fail/`. | "This panel reserves `sizeof(memory)` bytes at start and never asks for more." |
| 4 | The task pool part `lv::tasks<Count, Slot>`, the contract on the slot, a defined result for an awaited task without a frame, and the DWARF check of frame sizes. Smaller `lv::animate` frames first. | "Its four tasks live in 512 bytes the compiler reserved." |
| 5 | `lv::dump` of the layout: each part, its offset and size, its peak. The ports print it. | The walkthrough shows the map. |

Slice 1 needs nothing from node storage, and changes no existing program: without the option, LVGL keeps its own array.

## What was compiled, and what was reasoned

| Claim | Compiled and run | Compiled only | Reasoned |
|---|---|---|---|
| Inventory by phase, LVGL heap, `operator new`, `malloc` | Host and Cortex-M4 | | |
| Smallest pool by bisection, on zero failed allocations: slice 10240, program 15488, with the picture 189760 | Cortex-M4 | | |
| A decode that does not fit: one failure, the program goes on | Cortex-M4 | | Whether pixels showed |
| Widget structs incomplete in the public headers | | v9.6.0 and v10 pin | |
| (b) Layout of parts, total constant, placement new, `one_of` taking the old screen's place | Host and Cortex-M4 | | |
| (b) Compile errors for a budget and for a tree without a place | | Host | |
| (b) Region in NOBITS `.bss`: 23208 of 24396 bytes on the M4, with 1252 of `.data` | Cortex-M4 | | |
| (c) LVGL's heap in the region through `LV_MEM_POOL_ALLOC`, LVGL's own core unmodified | Host and Cortex-M4, v9.6.0 | | |
| (c) The same on the v10 pin | | | Yes: the two files are identical |
| (c) `lv_mem_add_pool` keeps LVGL's array and uses it first | Cortex-M4 | | |
| (d) Task frames from a pool, nothing of them on LVGL's heap | Host and Cortex-M4, with a patched `task.hpp` | | |
| GCC: no HALO, no frame-size builtin, frame sizes in DWARF | Host and Cortex-M4 | | |
| Frame sizes of the ports | | Cortex-M4, 9 of 11 files | |
| Design B and C | | | Yes; C rests on the arena measured in `memory.md` |
| A tool that reads `.Frame` sizes and fails the build | | | Yes |
| The 20-line program | | | Yes: its parts were each run |
| Who ends the parts at exit | | | Yes; the sketch shows the gap |

## Not confirmed

- The inventory runs on LVGL v9.6.0 only; the allocator files are the same on the v10 pin, but widget costs were not measured there.
- That the 36 bytes between the two `lv_init` numbers are the PNG decoder; it is the only other difference between the builds.
- Whether the DWARF `.Frame` type is a stable part of GCC's output across versions; it was seen in 16.2 only.
- The status of GCC bug 100579 today: bugzilla refused automated fetches.
- The official AUTOSAR and MISRA texts; only transcriptions and tool vendors' pages were read.
- How the hook behaves when LVGL is built as a shared library by a package manager.
- Whether `LV_MEM_POOL_ALLOC` stays in LVGL after v10: it is undocumented outside the source.

## Decisions for the owner

1. Which design?
   Options: A, a typed layout; B, an arena carved at start; C, a per-screen arena.
   Recommendation: A.
   Cost: depends on node storage's non-movable `mounted`; slice 1 does not.
2. How does LVGL's heap enter the layout?
   Options: `LV_MEM_POOL_ALLOC` with a hook file from CMake; `lv_mem_add_pool`; `LV_STDLIB_CUSTOM`.
   Recommendation: `LV_MEM_POOL_ALLOC`.
   Cost: a small C file in the `lvgl` target, and an undocumented LVGL setting.
3. Where does the heap's size come from?
   Options: `LVGL26_LVGL_HEAP_BYTES` in CMake, read by the layout as `LV_MEM_SIZE`; a number in the layout that CMake must match.
   Recommendation: CMake, because TLSF sizes its tables from it.
   Cost: the size of one part is not written in C++.
4. What does "no allocation after start" promise?
   Options: no call of `lv_malloc` after start; no system heap, and LVGL's heap a fixed part with a tested bound.
   Recommendation: the second.
   Cost: drawing still allocates inside the part, and the walkthroughs must say "never asks for more", not "never allocates".
5. How is LVGL's part sized?
   Options: a tool that sums per-kind sizes from LVGL's private headers; a measured budget, tested on the host and bisected in QEMU.
   Recommendation: measured.
   Cost: a number per program that a test keeps honest.
6. How are task frames sized?
   Options: one slot size; slots of several sizes; a check of DWARF `.Frame` sizes at build time in addition.
   Recommendation: slots of several sizes, a contract in `operator new`, and the DWARF check.
   Cost: a tool that reads debug information, and a `-g` build per target.
7. What happens to an awaited task whose frame does not fit?
   Today a `task<void>` is skipped silently and a `task<T>` violates a precondition.
   Options: keep that; a defined error the awaiting task receives; the whole spawned task ends.
   Recommendation: the whole spawned task ends, as when its widget is deleted.
   Cost: one more path in `task.hpp`, and code that relied on the silent skip changes.
8. What happens when a picture does not fit LVGL's heap?
   Today one allocation fails, the frame ends, and no error reaches the program.
   Options: keep that; a log line; a contract in a checked build.
   Recommendation: a contract in a checked build, and the budget test draws every picture.
   Cost: the test must reach every screen.
9. Are the names right?
   Options: `lv::memory`, `lv::within`, `lv::one_of`, `lv::tasks`, `lv::page`; or `lv::layout`, `lv::budget`, `lv::union_of`.
   Recommendation: the first set.
   Cost: none yet.
10. Who ends the parts of a static layout?
    Options: `lv::application` ends them before `lv_deinit`; the layout is a member of the application; nobody.
    Recommendation: the application, through the layout it was given.
    Cost: one pointer in `lv::application`, and an `end()` on the layout.

## What the owner decided

Decided 2026-11-18, in an interview over the list above.

| # | Decision |
|---|---|
| 1 | A typed layout, `lv::memory<parts...>`, whose total size is a constant of the compiler. |
| 2 | LVGL keeps its allocator and takes its pool from the layout, through CMake definitions. |
| 3 | The size of LVGL's heap comes from one CMake setting, and a test checks that the layout agrees. |
| 4 | After start the program uses no system heap, and LVGL's heap is a fixed part with a tested limit. A study of LVGL's allocations in each frame follows. |
| 5 | The heap part is sized by measurement and held by a budget test. A bound that the compiler computes is studied as well. |
| 6 | Not decided. Shrinking task frames and sizing them is studied first. |
| 7 | An awaited task that gets no frame ends the whole spawned task, and the program is told. |
| 8 | A checked build stops when a picture does not fit, and the budget test draws every picture. The layout also gets a part for decoding, sized from the largest picture. |
| 9 | The names are `lv::memory`, `lv::lvgl_heap`, `lv::band`, `lv::one_of` and `lv::tasks`. |
| 10 | `lv::application` ends the parts in reverse order, before LVGL shuts down. |

Three studies follow from these answers.
The first is where LVGL allocates while it draws, and whether layers and draw tasks can get fixed buffers.
The second is a bound on the widget memory of a tree that the compiler can compute.
The third is why some task frames are large, and how a frame gets a size that is known before it runs.
