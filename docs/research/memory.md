# Memory: duplicates, copies, and a layout planned at compile time

Research of 2026-11-16 for the checkout `build/wt/comp`. Marks follow `docs/research/README.md`. Every mark below is dated 2026-11-16.

How the work was done.

- The code audit and the LVGL reading were done on the local trees: `build/_src/lvgl-v9.6.0` and the v10 pin `b283a12` in `build/host-v10/_deps/lvgl-src`.
- `measured` rows come from programs built here. They are kept in a local scratch directory outside the repository: `armprobe/probe.cpp`, `armprobe/spike.cpp`, `probe/*.cpp`.
- The Arm numbers: arm-none-eabi GCC 16.2, Cortex-M4, `-Os`, LVGL v9.6.0 default configuration, run in QEMU `mps2-an386`. Heap is `lv_mem_monitor` (`total_size - free_size`) before and after one call.
- Web facts in sections 3 and 5 and the standard's status in section 4 were read by three research agents. Their marks are kept as they gave them.
- One agent could not use the fetch tool for some sites and downloaded the pages with curl instead. st.com could not be read at all.

## Short answers

| Question | Answer |
|---|---|
| 1. Where do we store twice or copy? | Eight places. The largest: constant text copied to the heap, one constant style per node instead of per value, a subject per state, the default theme's styles. |
| 2. Can a widget live in caller memory? | No. LVGL 9.6 and the v10 pin always `lv_malloc` a widget. Only the allocator as a whole can be replaced or moved. |
| 2. A pool sized by the compiler from the tree? | Not from the public API: the sizes are private. A per-screen arena works through the allocator hook, with a measured size. It needs more bytes than TLSF. |
| 3. How do platforms place data? | Linker regions plus a section attribute on the variable. DMA and cache rules decide where a frame may go. |
| 4. Can C++26 plan a layout? | Yes for our own data: a storage struct made by reflection, `constinit`, a section attribute. Not for coroutine frame sizes. No trivial relocation in C++26. |
| 5. What do others do? | TouchGFX and embassy size all storage at compile time. Qt for MCUs names a linker section per asset group. Slint and Embedded Wizard keep a heap. |
| 6. Zero-copy for a UI? | Text and images from flash, a frame drawn in place, and a text buffer the label points to. A widget's own value cannot be removed. |

## Measured here

All on Cortex-M4, LVGL v9.6.0, default configuration.

### LVGL struct sizes (bytes)

Both LVGL versions give the same numbers. Measured with `sizeof` in a C file that includes `lvgl_private.h`, compiled with `arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -DLV_CONF_SKIP -S`.

| Struct | Bytes | Struct | Bytes |
|---|---|---|---|
| `lv_obj_t` | 56 | `lv_obj_spec_attr_t` | 56 |
| `lv_label_t` | 120 | `lv_arc_t` | 108 |
| `lv_button_t` | 56 | `lv_bar_t` | 128 |
| `lv_slider_t` | 176 | `lv_image_t` | 104 |
| `lv_textarea_t` | 136 | `lv_dropdown_t` | 92 |
| `lv_obj_style_t` (one entry of a widget's style list) | 8 | `lv_style_t` | 12 |
| `lv_style_const_prop_t` | 8 | `lv_event_dsc_t` | 12 |
| `lv_subject_t` | 36 | `lv_observer_t` | 24 |
| `lv_timer_t` | 28 | `lv_anim_t` | 96 |
| `lv_display_t` | 812 | `lv_theme_t` | 40 |
| `lv_image_dsc_t` | 28 | `lv_font_t` | 44 |

The allocator adds 4 bytes to each block and rounds a block up to at least 12 bytes ([lv_tlsf.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/stdlib/builtin/lv_tlsf.c)).

### Heap cost of one call (bytes of LVGL's heap)

| Call | Bytes | Note |
|---|---|---|
| `lv::application` (`lv_init`) | 2080 | |
| First `lv::display` | 3988 | |
| Second `lv::display` | 1456 | The difference, 2532, is the default theme's styles. |
| `lv_obj_create` | 80 | Plus 68 once for the parent's `spec_attr`. |
| `lv_label_create` | 200 | 120 struct, 56 `spec_attr`, a text block, three block headers. |
| `lv_arc_create` | 212 | |
| `lv_button_create` | 200 | |
| `lv_label_set_text`, 40 characters | 32 more than a 5 character text | `lv_label_set_text_static` gives all of it back. |
| `lv_obj_add_style` of a constant style | 12, then 4 | One 8-byte entry in the widget's list. |
| First local style property of a selector | 32 | An `lv_style_t`, its property array, a list entry. |
| `pad_all` as a local style, same selector | 20 more | Four properties. |
| First local property of a second selector | 44 | |
| `lv_obj_add_event_cb` on a widget without `spec_attr` | 80 | Then 20 and 12 for the next two. |
| `lv_obj_add_delete_cb` | 24 | What `lv::owned` and `lv::weak` add. |
| `lv_subject_create` | 44 | |
| `lv_subject_add_observer` | 32 | |
| `lv_label_bind_text` | 68 | Observer, delete event, the formatted text. |
| `lv_arc_bind_value` | 76 | |
| `lv_timer_create` | 36 | |
| `lv_anim_start` | 104 | LVGL copies the 96-byte descriptor. |
| `lv::mount` of the slice (column, label, arc, bound label) | 1008 to 1052 | The README says 1052. The first mount of a state also creates its subject. |
| `lv::spawn` of a task with one sleep | 120 | Frame plus timer. |
| `lv::mount` of a column with ten one-character labels | 2364 | About 220 per label. |
| `lv::mount` of a list: ten rows of label, bound label, button with label (51 widgets) | 11064 to 11100 | About 217 per widget. |

Sizes of our objects on the same target: `mounted<ui>` with three ids 20, the `std::expected` around it 24, `lv::state<lv::counter>` 12, `lv::display` 20, `lv::owned<obj>` 8.

### Probes with GCC 16.2

| Checked | Result | How |
|---|---|---|
| `std::define_static_array` of equal contents | The same object: `static_assert(a() == b())` holds. | `g++-16 -std=c++26 -freflection`, `probe/p1.cpp` |
| `std::define_static_object` | Equal values give one object, different values two. | same |
| Section of a `define_static_array` result on Arm | `.rodata._ZTAX...`, a weak symbol, one section per value. | `arm-none-eabi-readelf -S`, `probe/dsa.cpp` |
| A non-movable type returned by value | Its address is final in all five cases: plain, `std::expected(std::in_place, ...)`, `optional::emplace` of a factory, `std::array`, an aggregate. | `probe/p4.cpp`, each object stores `this` |
| Returning a named non-movable object into `std::expected` | Compile error. | `probe/p4b.cpp` |
| `std::vector<non-movable>::emplace_back` | Compile error. | `probe/p4c.cpp` |
| `std::inplace_vector` | Present (`__cpp_lib_inplace_vector` 202603). `constinit` works. A non-movable element can be emplaced. `sizeof(inplace_vector<int, 8>)` is 36. | `probe/p5.cpp` |
| `std::start_lifetime_as_array` | Present (`__cpp_lib_start_lifetime_as` 202207). | same |
| Trivial relocation | Neither `__cpp_lib_trivially_relocatable` nor `__cpp_trivial_relocatability` is defined. | same |
| A storage struct made by `define_aggregate`, one member per instance, `constinit`, in a named section | Compiles and runs. `sizeof` 176, offsets and sizes readable by reflection. | `probe/p6.cpp` |
| A coroutine frame in caller memory | The promise's `operator new(size, arena&, ...)` gets the coroutine's first argument. The frame (56 bytes) lies inside the arena. | `probe/p10.cpp` |
| `[[gnu::section(".ccm")]] constinit` on a zeroed 1 KB object | Section type PROGBITS: the zeros are stored in the image. | `arm-none-eabi-readelf -S`, `probe/sec.cpp` |
| The same with the name `.bss.ccm` or `.noinit.fb` | NOBITS: nothing is stored. `alignas(32)` is kept. | same |
| Section attribute on a variable template, an inline variable, a static member of a class template | Honoured. Each gets a COMDAT group in the named section. | same |
| A `const` and a writable variable in one named section | Error: "causes a section type conflict". | `probe/sec2.cpp` |
| A `const` struct with a pointer in a named section | Stays read-only (flags `A`), with a relocation. | `probe/sec3.cpp` |
| `[[gnu::section(name)]]` with a `constexpr const char*` | Error: "section attribute argument not a string constant". | `probe/lit.cpp` |

### Spike: a per-screen arena for LVGL's own allocations

The linker wraps `lv_malloc_core`, `lv_realloc_core` and `lv_free_core` (`-Wl,--wrap`). During `lv::mount` every allocation comes from a bump arena. LVGL is unchanged.

| Screen | TLSF heap | Arena, plain bump | Arena, last block grows in place | Calls during mount |
|---|---|---|---|---|
| Slice, 4 widgets | 1008 | 1532 (460 dead) | 1300 (228 dead) | 35 malloc, 21 realloc, 8 free |
| List, 51 widgets | 11064 | 16376 (4872 dead) | 12692 (1188 dead) | 317 malloc, 159 realloc, 36 free |

- The mount, twenty frames of drawing, a change of a bound text and the unmount all work.
- A third of all calls are `realloc`: LVGL grows child, style and event lists by one entry each time.
- A bump arena therefore needs 15 to 30 percent more bytes than TLSF, not fewer.
- Up to 112 bytes still went to the common heap: the parent's child list is not ours.
- After the mount, 40 frees of arena blocks arrived while drawing and 270 at unmount. The arena ignores them.
- What the arena gives: a named region, a known peak, release in one step, no fragmentation. It does not save RAM.

## 1. Where lvgl26 stores a value twice, or copies

| Finding | Link | Mark |
|---|---|---|
| A state keeps `value_` and, once bound, a subject with the value again. After that `get()` reads the subject and `value_` is stale. | `include/lv/state.hpp` | `source` |
| The subject is a heap block of 44 bytes. It holds the value four times over: `value`, `prev_value`, `min_value`, `max_value`. | [lv_observer.h](https://github.com/lvgl/lvgl/blob/v9.6.0/include/lvgl/core/lv_observer.h) | `measured` |
| A bound arc holds the value a third time, in its own field. LVGL draws from that field. | [lv_arc.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/widgets/arc/lv_arc.c) | `source` |
| A bound label holds it a fourth time, as text on the heap. Each change frees the text and allocates a new one. | [lv_label.c, `lv_label_set_text_vfmt`](https://github.com/lvgl/lvgl/blob/v9.6.0/src/widgets/label/lv_label.c) | `source` |
| `lv::computed` keeps no value of its own. It computes on `get()`, and its subject holds the result. | `include/lv/computed.hpp` | `source` |
| Constant text is stored twice: in read-only memory and in a heap copy. `.text` maps to `lv_label_set_text`, which copies. | `include/lv/props.hpp`, [lv_label.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/widgets/label/lv_label.c) | `source` |
| The copy costs at least 16 bytes per label, and `strlen + 1` rounded up plus 4 above that. | probe | `measured` |
| `lv_label_set_text_static` keeps the pointer and copies nothing. It exists for label, checkbox, arclabel, span and dropdown. | [lv_label.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/widgets/label/lv_label.c) | `source` |
| Static text does not support the long mode "dots". LVGL logs a warning and shows the text uncut. | [lv_label.c, line 1459](https://github.com/lvgl/lvgl/blob/v9.6.0/src/widgets/label/lv_label.c) | `source` |
| The reference lists `text_static` as a property of its own. No example or tree uses it. | `docs/reference/lvgl-v9.6.0.json` | `source` |
| A tree's constant style is one `static constexpr lv_style_t` per node and member, not per value. | `include/lv/style.hpp`, `add_const_style` | `source` |
| `tests/large_tree.cpp` has four distinct styles and 81 style objects in its object file. | `nm` on `build/host-v96/tests/.../large_tree.cpp.o` | `measured` |
| The theme path does not have this: it stores styles with `std::define_static_array`, which gives one object per value. | `include/lv/theme.hpp`, probe `p1.cpp` | `measured` |
| In a tree, a shorthand (`pad_all`, `size`) becomes a local style on the heap: 32 bytes for the first, 20 more for `pad_all`. | `include/lv/style.hpp`, probe | `measured` |
| The theme path expands the same shorthands at compile time into constant properties (`add_long_forms`). | `include/lv/theme.hpp` | `source` |
| A widget's style list is a heap array of 8 bytes per style, grown by `realloc` on each add. It is a list of references, not a copy. | [lv_obj_style.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/core/lv_obj_style.c) | `source` |
| LVGL builds its default theme for the first display: 2532 bytes of styles on the heap. `lv::display` puts our constant theme on top of it. | `include/lv/display.hpp`, probe | `measured` |
| An image made by the compiler is one constant. `lv_image_set_src` keeps the pointer. | `include/lv/image.hpp`, [lv_image.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/widgets/image/lv_image.c) | `source` |
| LVGL still copies an image to the heap when stride or premultiplied alpha do not match, and for A1, A2, A4 and compressed data. | `docs/research/images.md`, section 1 | `source` |
| A font made by the compiler is constants only. A style holds the pointer. | `include/lv/font.hpp` | `source` |
| `lv::scaled_font` and `lv::encoded_image` use the heap by design: TinyTTF's glyph cache, the decoder's buffer. | `include/lv/scaled_font.hpp`, `include/lv/encoded_image.hpp` | `source` |
| Ids cost one pointer each in `mounted`. Names exist only at compile time. The same pointer is also in the parent's child list. | `include/lv/tree.hpp` | `source` |
| `mounted` owns its root through `lv::weak`, which registers a delete callback: 24 bytes of heap. | `include/lv/owned.hpp`, probe | `measured` |
| One `lv::mount` registers that callback four times and removes it three times, because `weak` re-registers on every move. | `include/lv/tree.hpp`, `include/lv/owned.hpp` (read, not counted at run time) | `source` |
| A frame is the only copy of the picture. `lv_display_set_buffers` wraps it in a descriptor inside `lv_display_t` and allocates nothing. | [lv_display.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/display/lv_display.c) | `source` |
| A band is handed to the panel as a span, without a copy. | `include/lv/display.hpp` | `source` |
| `lv::display` calls `lv_display_flush_ready` when the panel function returns. A DMA transfer must block or copy the band first. | `include/lv/display.hpp`, `drawn` | `source` |
| `lv::frame` and `lv::band` are aligned to `LV_DRAW_BUF_ALIGN` (4 by default). No parameter raises it to a cache line. | `include/lv/display.hpp` | `source` |
| A coroutine frame is one `lv_malloc`. A watched widget adds a 16-byte record and a delete event. | `include/lv/task.hpp` | `source` |
| A generic binding adds an observer (32 bytes) and a delete event on the widget. | `include/lv/props.hpp`, [lv_observer.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/core/lv_observer.c) | `measured` |
| An event handler costs nothing of ours: the function pointer travels in `user_data`. LVGL allocates a 12-byte record and a pointer in a list. | `include/lv/props.hpp`, [lv_event.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/misc/lv_event.c) | `source` |
| A property value is in the program once, as an argument in code, and once in the widget. The tree itself is not stored. | `include/lv/tree.hpp`, `apply_node` | `source` |
| A document loaded at run time has its text copied on purpose: its buffer is temporary. | `include/lv/runtime.hpp`, `copies_text` | `source` |

## 2. What LVGL allocates per widget, and what can be avoided

| Finding | Link | Mark |
|---|---|---|
| A widget is `lv_malloc_zeroed(instance_size)`. The size is a field of the class, found by walking the base classes. | [lv_obj_class.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/core/lv_obj_class.c) | `source` |
| There is no creation into caller memory, no hook, and no allocator per object. `lv_obj_class.c` is identical in v9.6.0 and the v10 pin. | local `diff` | `source` |
| `lv_obj_class_t` and every widget struct are private. The public API gives no size of a widget. | [lv_obj_class_private.h](https://github.com/lvgl/lvgl/blob/v9.6.0/src/core/lv_obj_class_private.h) | `source` |
| `spec_attr` (56 bytes) is allocated when a widget gets a child, an event or scroll data. | [lv_obj.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/core/lv_obj.c) | `source` |
| The child list grows by `lv_realloc` of one pointer per child. No call reserves room. | [lv_obj.c, `lv_obj_add_child`](https://github.com/lvgl/lvgl/blob/v9.6.0/src/core/lv_obj.c) | `source` |
| The style list grows the same way, 8 bytes per style. A local style adds an `lv_style_t` and a property array. | [lv_obj_style.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/core/lv_obj_style.c) | `source` |
| Each event handler is its own `lv_malloc` of 12 bytes, plus a pointer in a growing array. | [lv_event.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/misc/lv_event.c) | `source` |
| An observer is a node of the subject's linked list. For a widget, LVGL also adds a delete event to the widget. | [lv_observer.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/core/lv_observer.c) | `source` |
| v9.6: `lv_subject_init_*` can use a subject embedded in our object, but is deprecated. `lv_subject_create` allocates. | [lv_observer.h](https://github.com/lvgl/lvgl/blob/v9.6.0/include/lvgl/core/lv_observer.h) | `source` |
| v10 pin: the `init` functions are gone. A subject is always a node of LVGL's own list on the heap. | local `diff` of `lv_observer.c` | `source` |
| lvgl26 already uses `lv_subject_create` on both versions, so a state costs 44 bytes of heap on both. | `include/lv/state.hpp` | `source` |
| A timer is a list node (36 bytes). An animation is a copied descriptor (104 bytes). | probe | `measured` |
| The built-in allocator is one TLSF instance over a static array of `LV_MEM_SIZE` bytes. | [lv_mem_core_builtin.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/stdlib/builtin/lv_mem_core_builtin.c) | `source` |
| `LV_MEM_ADR` puts that one pool at a fixed address. `LV_MEM_POOL_ALLOC` takes it from a function. | same file | `source` |
| `LV_ATTRIBUTE_LARGE_RAM_ARRAY` is applied to the pool array, so a section attribute can place it. | same file | `source` |
| `lv_mem_add_pool` adds memory to the same TLSF instance. It cannot direct one screen's widgets to one pool. | same file | `source` |
| `LV_MEM_POOL_EXPAND_SIZE` is deprecated in v9.6: the source warns and says to use `LV_MEM_SIZE`. | [lv_tlsf.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/stdlib/builtin/lv_tlsf.c) | `source` |
| `LV_USE_STDLIB_MALLOC = LV_STDLIB_CUSTOM` lets the program supply `lv_malloc_core`, `lv_realloc_core`, `lv_free_core` and the monitor functions. | [lv_conf_template.h](https://github.com/lvgl/lvgl/blob/v9.6.0/lv_conf_template.h), [lv_mem.h](https://github.com/lvgl/lvgl/blob/v9.6.0/include/lvgl/stdlib/lv_mem.h) | `source` |
| The allocator files are identical in v9.6.0 and the v10 pin. | local `diff` | `source` |
| A per-screen arena through that hook works. See "Spike" above for its cost. | `armprobe/spike.cpp` | `measured` |
| The compiler can count the widgets, styles, handlers and bindings of a tree. It cannot turn the counts into bytes without LVGL's private sizes. | `include/lv/tree.hpp`, the rows above | `source` |
| LVGL allocates the default theme's styles once, for the first display. `LV_USE_THEME_DEFAULT 0` removes them. | [lv_display.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/display/lv_display.c) | `source` |

Avoidable without changing LVGL: the text copy, local styles, subjects and observers (with our own binding), the delete callback churn, the default theme.
What cannot: the widget struct, `spec_attr`, the three growing lists, event records.

## 3. Memory regions on real platforms

### STM32

| Finding | Link | Mark |
|---|---|---|
| On H7, DMA1 and DMA2 reach almost all memories, except ITCM and DTCM RAM. | [ST FAQ](https://community.st.com/t5/stm32-mcus/dma-is-not-working-on-stm32h7-devices/ta-p/49498) | `source` |
| DTCM is the default RAM in some H7 projects. The FAQ's fix is to move the buffer to RAM_D1 or D2. | same | `source` |
| With the data cache on, DMA reads the RAM, not the cache. A cache line is 32 bytes; buffers are aligned to it before cache maintenance. | same | `source` |
| The other way is an MPU region without cache. Such a region is a power of two in size and aligned to its size. | same | `source` |
| ST's G4 linker script declares `CCMSRAM` as its own region. `.data` is `>RAM AT> ROM`. | [STM32G474RETX_FLASH.ld](https://github.com/STMicroelectronics/STM32CubeG4/blob/master/Projects/NUCLEO-G474RE/Templates/STM32CubeIDE/STM32G474RETX_FLASH.ld) | `source` |
| ST's stock F4 start-up copies only `.data` and clears only `.bss`. A section in another region is not initialized. | [startup_stm32f407xx.s](https://github.com/STMicroelectronics/cmsis_device_f4/blob/master/Source/Templates/gcc/startup_stm32f407xx.s) | `source` |
| ST's LTDC example puts the framebuffer in external SDRAM and graphics in external Quad-SPI flash. | [AN4861](https://www.st.com/resource/en/application_note/dm00287603-lcdtft-display-controller-ltdc-on-stm32-mcus-stmicroelectronics.pdf) | `summary` |
| F4 CCM RAM cannot be used by DMA. | [AN4296](https://www.st.com/resource/en/application_note/an4296-use-stm32f3stm32g4-ccm-sram-with-iar-embedded-workbench-keil-mdkarm-stmicroelectronics-stm32cubeide-and-other-gnubased-toolchains-stmicroelectronics.pdf) | `summary` |

### ESP32

| Finding | Link | Mark |
|---|---|---|
| `.data` and `.bss` go to internal DRAM. On ESP32 at most 160 KB of DRAM can be static. | [Memory types](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/memory-types.html) | `source` |
| Constants go to DROM, flash mapped through the MMU cache. `DRAM_ATTR` forces one into RAM. | same | `source` |
| Most DMA controllers need buffers in DRAM, word-aligned. `DMA_ATTR` does that for a static. | same | `source` |
| `MALLOC_CAP_DMA` excludes external PSRAM. | [Heap allocation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/mem_alloc.html) | `source` |
| `EXT_RAM_BSS_ATTR` moves a zeroed static to PSRAM. | [External RAM](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/external-ram.html) | `source` |
| PSRAM cannot be reached while the flash cache is off, for example during a flash write. | same | `source` |
| A linker fragment file places an archive, an object or a symbol. | [Linker script generation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/linker-script-generation.html) | `source` |
| RGB panel: the framebuffer is internal by default; `fb_in_psram` moves it. Two full framebuffers must be in PSRAM. | [RGB LCD](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/lcd/rgb_lcd.html) | `source` |
| Bounce buffers: two small internal buffers feed DMA and an interrupt copies into them from the PSRAM frame. | same | `source` |
| Espressif's LVGL port has the flags `buff_dma` and `buff_spiram` for the draw buffer. | [esp_lvgl_port](https://github.com/espressif/esp-bsp/blob/master/components/esp_lvgl_port/README.md) | `source` |

### RP2040 and RP2350

| Finding | Link | Mark |
|---|---|---|
| Default script: code and `.rodata` in flash, `.data` in RAM loaded from flash, 256 KB RAM, two 4 KB scratch banks. | [memmap_default.ld](https://github.com/raspberrypi/pico-sdk/blob/2.1.1/src/rp2_common/pico_crt0/rp2040/memmap_default.ld) | `source` |
| `__not_in_flash(group)` puts data, also `static const` data, into RAM. `__in_flash(group)` keeps it in flash. | [sections.h](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_platform_sections/include/pico/platform/sections.h) | `source` |
| `__uninitialized_ram(x)` is a `NOLOAD` section in RAM. The value survives a reset. | same | `source` |
| RP2350 supports flash or PSRAM on a second chip select. Its window is read-only until `WRITABLE_M1` is set. | [RP2350 datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf) | `source` |

### NXP i.MX RT

| Finding | Link | Mark |
|---|---|---|
| FlexRAM banks are set up as ITCM, DTCM or OCRAM. The TCMs run at core speed; OCRAM is slower. | [AN12077](https://www.nxp.com/docs/en/application-note/AN12077.pdf) | `source` |
| Data only the core uses belongs in DTCM. Data shared with DMA belongs in OCRAM. | same | `source` |
| OCRAM is cached. NXP recommends a region without cache for memory shared with DMA. | same | `source` |
| With GCC, `AT_NONCACHEABLE_SECTION_ALIGN(var, n)` is a section `NonCacheable` plus `aligned(n)`. | [fsl_common_arm.h](https://github.com/nxp-mcuxpresso/mcux-sdk/blob/main/drivers/common/fsl_common_arm.h) | `source` |
| The RT1052 flash script has a non-cacheable region of size 0 by default. The SDRAM script gives it 2 MB. | [flexspi_nor.ld](https://github.com/nxp-mcuxpresso/mcux-sdk/blob/main/devices/MIMXRT1052/gcc/MIMXRT1052xxxxx_flexspi_nor.ld), [flexspi_nor_sdram.ld](https://github.com/nxp-mcuxpresso/mcux-sdk/blob/main/devices/MIMXRT1052/gcc/MIMXRT1052xxxxx_flexspi_nor_sdram.ld) | `source` |

### The tools

| Finding | Link | Mark |
|---|---|---|
| `MEMORY` names blocks. The linker does not move sections to make them fit. | [ld MEMORY](https://sourceware.org/binutils/docs/ld/MEMORY.html) | `source` |
| A section has a run address and a load address. `AT>` sets the load address. Start-up code must copy `.data` and clear `.bss`. | [ld Output Section LMA](https://sourceware.org/binutils/docs/ld/Output-Section-LMA.html) | `source` |
| `NOLOAD` marks a section as not loaded. `KEEP` protects input sections from `--gc-sections`. | [ld Output Section Type](https://sourceware.org/binutils/docs/ld/Output-Section-Type.html), [ld KEEP](https://sourceware.org/binutils/docs/ld/Input-Section-Keep.html) | `source` |
| The `section` attribute applies to global variables, initialized or not. `[[gnu::section("x")]]` is accepted. | [GCC Common Attributes](https://gcc.gnu.org/onlinedocs/gcc/Common-Attributes.html), [GCC Attributes](https://gcc.gnu.org/onlinedocs/gcc/Attributes.html) | `source` |
| `-fdata-sections` gives each data item its own section, named after it. | [GCC Optimize Options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html) | `source` |
| Our linker script has two regions, CODE and RAM, and collects `.rodata` into CODE. A variable in an unknown section is not placed by it on purpose. | `cmake/arm/mps2-an386.ld` | `source` |
| A zeroed object in a section named `.ccm` is stored as zeros in the image. Named `.bss.ccm`, it is not. | `probe/sec.cpp` | `measured` |
| A `define_static_array` result is in a section `.rodata._ZTAX...`. A linker script can select these by pattern. | `probe/dsa.cpp` | `measured` |

### What LVGL recommends for buffers

| Finding | Link | Mark |
|---|---|---|
| PARTIAL mode: a buffer of at least a tenth of the display. DIRECT and FULL: buffers of the display's size. | [Display setup](https://lvgl.io/docs/open/main-modules/display/setup) | `source` |
| With one buffer LVGL waits for `lv_display_flush_ready`. With two, hardware sends one while LVGL draws into the other. | same | `source` |
| `lv_display_flush_is_last` tells the last piece of a frame. | same | `source` |
| `LV_DRAW_BUF_ALIGN` is 4 by default, `LV_DRAW_BUF_STRIDE_ALIGN` 1. `lv_display_set_buffers` rejects a buffer that is not aligned. | [lv_conf_template.h](https://github.com/lvgl/lvgl/blob/v9.6.0/lv_conf_template.h), [lv_display.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/display/lv_display.c) | `source` |
| v9.6 takes the `LV_ATTRIBUTE_*` macros from one custom header. Examples in the template name `.itcm`, `.ext_flash`, `.ext_ram`. | [lv_conf_template.h](https://github.com/lvgl/lvgl/blob/v9.6.0/lv_conf_template.h) | `source` |
| `LV_ATTRIBUTE_FLUSH_READY` exists because `lv_display_flush_ready` is often called from a DMA or display interrupt. | same | `source` |
| `LV_ATTRIBUTE_DMA` existed in v8.3 and is gone from v9. | [v8.3 template](https://github.com/lvgl/lvgl/blob/release/v8.3/lv_conf_template.h) | `source` |

## 4. C++ means to plan a layout at compile time

| Finding | Link | Mark |
|---|---|---|
| `constinit` makes dynamic initialization ill-formed. It does not make the variable const. | [dcl.constinit](https://eel.is/c++draft/dcl.constinit) | `source` |
| `define_static_array`, `define_static_string` and `define_static_object` return pointers to template parameter objects: static, const, one per value. | [meta.define.static](https://eel.is/c++draft/meta.define.static), [temp.param](https://eel.is/c++draft/temp.param) | `source` |
| GCC 16.2 gives equal contents one object, also across calls. | `probe/p1.cpp` | `measured` |
| Reflection can build a storage struct from a description, and read every member's size, alignment and offset as constants. | `probe/p6.cpp` | `measured` |
| A prvalue initializes its destination directly. Returning a named local is only allowed elision. | [dcl.init.general](https://eel.is/c++draft/dcl.init.general), [class.copy.elision](https://eel.is/c++draft/class.copy.elision) | `source` |
| So a non-movable result works when every `return` is a prvalue. `std::expected<T, E>(std::in_place, ...)` is one. | `probe/p4.cpp` | `measured` |
| Today's `mount` builds a named local, fills it, and returns it. With a non-movable type that does not compile. | `include/lv/tree.hpp`, `probe/p4b.cpp` | `measured` |
| `optional` and `expected` hold a non-movable type through `in_place` or `emplace`. A helper with a conversion operator emplaces a function's result. | [expected.object.cons](https://eel.is/c++draft/expected.object.cons), [P3288R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3288r0.html) | `source` |
| `std::vector` of a non-movable type cannot grow. | `probe/p4c.cpp` | `measured` |
| `std::inplace_vector` needs only an emplace-constructible element. Its storage is inline and its capacity is in the type. | [inplace.vector](https://eel.is/c++draft/inplace.vector), [sequence.reqmts](https://eel.is/c++draft/sequence.reqmts) | `source` |
| In a freestanding build `emplace_back` is deleted. `try_emplace_back` and `unchecked_emplace_back` remain. | [inplace.vector](https://eel.is/c++draft/inplace.vector) | `source` |
| GCC 16 ships `std::inplace_vector`. | [GCC 16 changes](https://gcc.gnu.org/gcc-16/changes.html), `probe/p5.cpp` | `measured` |
| `std::start_lifetime_as` needs an implicit-lifetime type. libstdc++ has it since 16.1. | [obj.lifetime](https://eel.is/c++draft/obj.lifetime), [libstdc++ status](https://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html) | `source` |
| Trivial relocation (P2786) was removed from C++26 at Kona. A later paper lists open questions for C++29. | [P3920R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3920r0.html), [P4197R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p4197r0.html) | `source` |
| GCC 16.2 defines neither feature macro of trivial relocation. | `probe/p5.cpp` | `measured` |
| `monotonic_buffer_resource` uses its buffer first and asks upstream only when it is full. Nothing in it is `constexpr`. | [mem.res.monotonic.buffer](https://eel.is/c++draft/mem.res.monotonic.buffer) | `source` |
| `<memory_resource>` is not a freestanding header. `<inplace_vector>`, `<optional>` and `<expected>` are. | [compliance](https://eel.is/c++draft/compliance) | `source` |
| Constexpr placement new (P2747) is in GCC 15. Structured-binding packs (P1061) are in GCC 16. | [cxx-status](https://gcc.gnu.org/projects/cxx-status.html) | `source` |
| A coroutine's allocation function is tried first with the size and the coroutine's parameters. | [dcl.fct.def.coroutine](https://eel.is/c++draft/dcl.fct.def.coroutine) | `source` |
| So a frame can be put into caller memory. Its size is only known when `operator new` is called. | `probe/p10.cpp` | `measured` |
| The section attribute needs a string literal. A setting in `lv::config` cannot name a section. | `probe/lit.cpp` | `measured` |
| The section attribute works on variable templates, inline variables and static members of class templates in GCC 16.2. | `probe/sec.cpp` | `measured` |
| Older GCC ignored it on variable templates (bug 88061). | [gcc-bugs mail](https://gcc.gnu.org/pipermail/gcc-bugs/2022-March/779938.html) | `source` |
| A section cannot hold const and writable variables together. | `probe/sec2.cpp` | `measured` |

What this allows.

- Our own run-time data can be one struct per tree, made by reflection, with a size known to `sizeof`.
- The user declares it like a frame: `constinit`, with a section attribute of their own.
- The library cannot place anything in a named section by itself: no literal, no macro.
- What breaks with a non-movable `mounted`: `std::vector<mounted>`, returning a named local, and `std::optional<mounted> = mount(...)` without the factory helper.

## 5. How others do it

### Qt for MCUs

| Finding | Link | Mark |
|---|---|---|
| QML is compiled to C++. There is no script engine and no garbage collection. | [Differences](https://doc.qt.io/QtForMCUs/qtul-qtquick-differences.html) | `source` |
| Stated needs: 20 KB of static RAM at least, 20 KB of heap, plus the framebuffer. | [Overview](https://doc.qt.io/QtForMCUs/qtul-overview.html) | `source` |
| `Loader` takes its items from an allocator of its own and frees before each new allocation. | [Loader](https://doc.qt.io/QtForMCUs/qml-qtquick-loader.html) | `source` |
| `resourceStorageSection` names the linker section of a group of images. The section must exist in the linker script. | [resourceStorageSection](https://doc.qt.io/QtForMCUs/qtul-qmlproject-imagefilesnode-mcu-resourcestoragesection.html) | `source` |
| `resourceCachePolicy`: copy at start, copy when needed, or `NoCaching`, which draws from storage. | [resourceCachePolicy](https://doc.qt.io/QtForMCUs/qtul-qmlproject-imagefilesnode-mcu-resourcecachepolicy.html) | `source` |
| `resourceRuntimeAllocationType` is a number that selects a platform allocator for cached resources. | [resourceRuntimeAllocationType](https://doc.qt.io/QtForMCUs/qtul-qmlproject-imagefilesnode-mcu-resourceruntimeallocationtype.html) | `source` |
| The example linker script has a preload region in SDRAM and a section for font data. | [Linker script setup](https://doc.qt.io/QtForMCUs/platform-porting-guide-linker-script-setup.html) | `source` |
| The static font engine draws glyphs at build time and needs no cache. | [Fonts](https://doc.qt.io/QtForMCUs/qtul-fonts.html) | `source` |

### Slint

| Finding | Link | Mark |
|---|---|---|
| Slint needs a global allocator on bare metal. | [MCU docs](https://docs.slint.dev/latest/docs/rust/slint/docs/mcu/index.html) | `source` |
| `render()` draws into a pixel slice the caller owns. `render_by_line()` needs one line of pixels. | same | `source` |
| `ReusedBuffer` redraws only what changed and expects the buffer to hold the previous frame. | [RepaintBufferType](https://docs.slint.dev/latest/docs/rust/slint/platform/software_renderer/enum.RepaintBufferType.html) | `source` |
| Images and fonts are pre-processed at compile time and drawn without decoding. | [EmbedResourcesKind](https://docs.slint.dev/latest/docs/rust/slint_build/enum.EmbedResourcesKind.html) | `source` |
| A model returns a row by value. `VecModel` clones the element. | [Model](https://docs.slint.dev/latest/docs/rust/slint/trait.Model.html) | `source` |

### TouchGFX

| Finding | Link | Mark |
|---|---|---|
| TouchGFX allocates nothing at run time. | [Memory usage](https://support.touchgfx.com/docs/basic-concepts/memory-usage) | `source` |
| RAM holds the objects of one screen at a time, in a block the compiler sizes for the largest screen. | same | `source` |
| `Partition` is a set of slots. The slot size is a constant from the largest type of a type list. | [Partition](https://support.touchgfx.com/docs/api/classes/classtouchgfx_1_1_partition) | `source` |
| External RAM holds framebuffers and a bitmap cache. External flash holds images, fonts and texts. | [Memory usage](https://support.touchgfx.com/docs/basic-concepts/memory-usage) | `source` |
| The bitmap cache is a buffer the user supplies with its size. | [Caching bitmaps](https://support.touchgfx.com/docs/development/ui-development/touchgfx-engine-features/caching-bitmaps) | `source` |
| A display with its own memory can use partial framebuffers instead of a full one. | [Partial framebuffer](https://support.touchgfx.com/docs/development/scenarios/lowering-memory-usage-with-partial-framebuffer) | `source` |

### Embedded Wizard

| Finding | Link | Mark |
|---|---|---|
| All GUI memory except static variables is dynamic, from its own heap manager with a garbage collector. | [Memory footprint](https://doc.embedded-wizard.de/memory-footprint) | `source` |
| The bitmap mode `DirectAccess` stores pixels uncompressed in flash and draws from there. | same | `source` |
| The same choice exists for string constants. Compressed ones are loaded into RAM. | same | `source` |

### Rust and Zephyr

| Finding | Link | Mark |
|---|---|---|
| `heapless` containers hold their memory inline, with the capacity in the type. | [heapless](https://docs.rs/heapless/latest/heapless/) | `source` |
| `StaticCell` reserves memory at compile time and is initialized at run time. | [static_cell](https://docs.rs/static_cell/latest/static_cell/) | `source` |
| Each embassy task gets a `static` sized at compile time, times `pool_size`. Overflow is a linker error. | [embassy-executor README](https://raw.githubusercontent.com/embassy-rs/embassy/main/embassy-executor/README.md) | `source` |
| Earlier, on stable Rust, tasks came from a bump arena of a configured size. A full arena panicked at run time. That option was removed. | [README v0.7.0](https://raw.githubusercontent.com/embassy-rs/embassy/embassy-executor-v0.7.0/embassy-executor/README.md), [CHANGELOG](https://raw.githubusercontent.com/embassy-rs/embassy/main/embassy-executor/CHANGELOG.md) | `source` |
| `link_section` places a static in a section. | [Rust Reference](https://doc.rust-lang.org/reference/abi.html) | `source` |
| Zephyr: `K_MEM_SLAB_DEFINE` defines a pool of equal blocks at compile time. | [Slabs](https://docs.zephyrproject.org/latest/kernel/memory_management/slabs.html) | `source` |
| Zephyr: a `zephyr,memory-region` node of the devicetree becomes a linker region. | [devicetree_regions.h](https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/main/include/zephyr/linker/devicetree_regions.h) | `source` |
| Zephyr: `__noinit`, `__nocache`, `__dtcm_bss_section` and others are section macros over `Z_GENERIC_SECTION`. | [section_tags.h](https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/main/include/zephyr/linker/section_tags.h) | `source` |
| Zephyr's LVGL module: static render buffers by default, in `.lvgl_buf` or a named memory region. | [Kconfig.memory](https://raw.githubusercontent.com/zephyrproject-rtos/zephyr/main/modules/lvgl/Kconfig.memory) | `source` |

What they share.

- The two that use no heap (TouchGFX, embassy) size storage from types the compiler knows.
- embassy moved from a run-time arena to compile-time statics. Rust knows a future's size; C++ does not know a frame's.
- Asset placement is by named linker section, chosen per group of assets, with the section defined by the port.
- "Draw from flash without a RAM copy" is an explicit, named option in Qt for MCUs and Embedded Wizard.

## 6. Zero-copy patterns for a UI

| Finding | Link | Mark |
|---|---|---|
| Text from flash: `lv_label_set_text_static` keeps the pointer. | [lv_label.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/widgets/label/lv_label.c) | `source` |
| Changing text without a heap: `lv_label_set_text_static(label, NULL)` redraws from the same buffer. | [lv_label.c, line 175](https://github.com/lvgl/lvgl/blob/v9.6.0/src/widgets/label/lv_label.c) | `source` |
| So a bound text can live in a buffer of ours that the label points to. The format's longest result is known at compile time. | the two rows above, `include/lv/values.hpp` (`text_format`) | `source` |
| Images from flash: an uncompressed image in the display's format is drawn in place. | `docs/research/images.md`, section 1 | `source` |
| A chart can read its points from the program's array: `lv_chart_set_series_ext_y_array`. | [lv_chart.h](https://github.com/lvgl/lvgl/blob/v9.6.0/include/lvgl/widgets/lv_chart.h) | `source` |
| A canvas draws into a buffer of the program: `lv_canvas_set_buffer`. | [lv_canvas.h](https://github.com/lvgl/lvgl/blob/v9.6.0/include/lvgl/widgets/lv_canvas.h) | `source` |
| A value widget (arc, bar, slider) keeps its own value. LVGL has no getter callback or pointer binding for it. | [lv_bar.c](https://github.com/lvgl/lvgl/blob/v9.6.0/src/widgets/bar/lv_bar.c) | `source` |
| A binding does not need a subject. It needs a list of (widget, setter) to call when the state changes. | `include/lv/props.hpp`, `follow_subject` | `source` |
| That list can be nodes inside the mounted object: the tree fixes their number. Then the state holds the only copy outside the widget. | `include/lv/tree.hpp`, `probe/p6.cpp` | `measured` |
| LVGL's own bind functions also write back (arc, slider). Without a subject that needs a `value_changed` handler of ours. | `include/lv/props.hpp`, `binder_of` | `source` |
| Event data is passed by pointer (`lv_event_get_param`). Our handlers take a function pointer in `user_data`. Nothing is copied. | `include/lv/props.hpp`, `add_handler` | `source` |
| DIRECT mode draws at the final place in a full frame. `lv::frame` uses it. | [Display setup](https://lvgl.io/docs/open/main-modules/display/setup), `include/lv/display.hpp` | `source` |
| DMA from the buffer: LVGL waits for `lv_display_flush_ready`, which may come later from an interrupt. | [Display setup](https://lvgl.io/docs/open/main-modules/display/setup) | `source` |
| lvgl26 cannot do that yet. Its display reports ready when the panel function returns. | `include/lv/display.hpp` | `source` |
| Lists: TouchGFX keeps a fixed set of row widgets and refills one by callback as it scrolls in. | [Scroll List](https://support.touchgfx.com/docs/development/ui-development/ui-components/containers/scroll-list) | `source` |
| Slint and Qt for MCUs create rows only while visible. | [ListView](https://docs.slint.dev/latest/docs/slint/reference/std-widgets/views/listview/), [ListView](https://doc.qt.io/QtForMCUs/qml-qtquick-listview.html) | `source` |
| In LVGL a row is real widgets: about 217 bytes of heap per widget here. A list must bound its rows. | `armprobe/spike.cpp` | `measured` |

## Not confirmed

- The byte counts of the `weak` churn in `lv::mount` were read from the code, not counted at run time.
- Why a new label already has a `spec_attr`. The 200 bytes fit that reading; the cause was not traced.
- Whether GCC documents which const objects go to `.rodata`. The probes show it; no manual page was found.
- ST application notes AN4296, AN4839 and AN4861 were not read. The CCM, cache and LTDC rows rest on ST's FAQ and on summaries.
- F7 and H7 memory sizes, memory-mapped QSPI details and an ST H7 linker script were not read.
- Where NXP's LCDIF framebuffers go. The note read only says that DMA-shared buffers belong in OCRAM.
- `LV_MEM_POOL_INCLUDE` and `LV_MEM_POOL_ALLOC` have no documentation beyond the source that uses them.
- Qt for MCUs: that ordinary QML objects are statically allocated. Only the `Loader` allocator is documented.
- Slint: the structure of its generated code and where properties are stored.
- TouchGFX: that widgets are members of the view class, and where it advises to put a framebuffer.
- No framework was found whose list rows read the model in place. Slint returns a row by value.
- A compile-time coroutine frame size in GCC: nothing was found. Clang's builtin works only inside the coroutine.
- How `std::pmr::null_memory_resource` behaves with `-fno-exceptions`.
- The cost of the arena on LVGL v10. The spike ran on v9.6.0 only; the allocator files are identical.
- Whether a per-screen arena survives a mount inside a mount (an overlay opened while the screen is built).

## What it means for us

### Every duplicate or copy found today

Sizes are for Cortex-M4.

| # | What | Bytes | Can it go? | How |
|---|---|---|---|---|
| 1 | Constant text: flash plus a heap copy | 16 or more per text | Yes | Use the `_static` setter for text of a constant tree. Find it by reflection. Refuse it with long mode "dots". |
| 2 | Constant styles: one object per node, not per value | 12 plus 8 per property, per use. 81 objects for 4 styles in `large_tree`. | Yes | Store them with `std::define_static_array`, as the theme does. |
| 3 | Shorthand style properties as local styles | 32 to 52 of heap per node | Yes | Expand them at compile time with `add_long_forms`, as the theme does. |
| 4 | The default theme under our theme | 2532 of heap, once | Yes, as a choice | A CMake option that sets `LV_USE_THEME_DEFAULT 0`. A theme of ours must then be complete. |
| 5 | State value: `value_` and the subject | 4 in the object, 44 of heap | Yes | Our own binding list. A subject only when `raw()` is asked for. |
| 6 | Observer and delete event per binding | 32 plus 16 to 76 of heap | Yes | A binding node inside the mounted object. One delete hook for the root. |
| 7 | Bound text: formatted on the heap at every change | 16 or more, freed and allocated again | Yes | A buffer inside the mounted object, sized from the format, shown with the static setter. |
| 8 | Delete callback of `mounted`: registered four times per mount | 24 of heap, three alloc and free pairs | Yes | A non-movable `mounted` registers once. |
| 9 | Ids: a pointer in `mounted` and in the parent's list | 4 per id | Could, not worth it | Walking the child path costs time at each use. |
| 10 | Widget value next to the state (arc, bar) | 4 to 8 per widget | No | LVGL draws from its own field. |
| 11 | Property values as code and in the widget | A call per property | No | A table in flash would only move them. |
| 12 | Style, child and event lists of a widget | 8, 4 and 4 per entry, plus `realloc` waste | No | An upstream change: a call that reserves room. |
| 13 | Coroutine frame per task | 84 or more of heap | Partly | Slots inside the mounted object. The slot size is a guess checked at run time. |
| 14 | A band sent by DMA | A copy of the band, or a blocked UI | Yes | A panel that reports "sent" later. |
| 15 | Animation descriptor | 104 of heap while it runs | No | LVGL copies it. |
| 16 | Images, fonts, frames | None | Nothing to do | Keep the rules of `docs/research/images.md`. |

For the list of 51 widgets, rows 1, 5, 6 and 7 are about 1.9 KB of the 11.1 KB.
The parts: 30 texts at 16, 10 bindings at about 100, 10 handlers stay.

### Three candidate designs

(a) Everything of ours inside the object `lv::mount` returns. The compiler makes one storage struct per tree: ids, binding nodes, text buffers, component instances, task slots. It is not movable. LVGL's widgets stay on LVGL's heap.

```cpp
auto ui_live = lv::mount<ui>(display.screen());        // built in this variable
[[gnu::section(".bss.dtcm")]] constinit lv::screen<ui> ui_live;   // or: declared first,
ui_live.mount(display.screen());                                  // then mounted
```

(b) A per-screen arena, also for LVGL's allocations. The program declares the memory and names its section. `lv::mount` switches LVGL's allocator to it while it builds.

```cpp
[[gnu::section(".bss.ccm")]] constinit lv::storage<ui, 2048> ui_memory;
auto ui_live = lv::mount<ui>(display.screen(), ui_memory);
```

(c) Named regions. The port's linker script defines them. The program places its own objects with attributes; a sample script and documentation say which data fits which region.

```cpp
[[gnu::section(".bss.frame")]] constinit lv::frame<320, 240, lv::cache_line> frame;   // DMA-capable, aligned
// .rodata._ZTAX* (fonts, images, styles, themes) -> external flash, in the linker script
```

| | (a) Storage in the mounted object | (b) Per-screen arena | (c) Named regions |
|---|---|---|---|
| Heap use | Only LVGL's own structs. Ours: none. | None for a static screen. Changes after the mount fall back to the heap. | Unchanged. It only says where the heap is. |
| Bytes stored twice | Removes rows 5 to 8. Rows 1 to 4 are separate fixes. | Removes nothing. It needs 15 to 30 percent more than TLSF. | Removes nothing. |
| What the user writes | The same `lv::mount` line. The result is not movable. | A size per screen, which a test must find. A second argument. | A section attribute on a frame. A linker script. |
| What a port provides | Nothing. | LVGL built with a custom allocator. A general allocator of ours. | Sections in the linker script. Start-up code that clears them. |
| Several screens | One object per screen. A static one, or one in a `std::optional` with the factory helper. | One arena per screen, or one for the largest, as TouchGFX does. | No effect. |
| Dynamic lists | A fixed capacity inside the object (`inplace_vector`), rows of real widgets on LVGL's heap. | Poor. A bump arena does not give a removed row back. | No effect. |
| Overlays | A second mounted object. | A second arena, or the heap. Nested mounts are untested. | No effect. |
| Compile-time cost | One more generated struct per tree. To be measured on `large_tree`. | None beyond (a). | None. |
| Works on both LVGL versions | Yes: public API only. | Yes: the allocator files are identical. | Yes. |
| Main risk | Two-way binding without a subject. C code that expects a subject. | Size cannot be computed. A third of the calls are `realloc`. | The library cannot name a section itself. |

They do not exclude each other. (a) is where "no value twice" is won. (c) is where "memory areas for specific data" is answered, and costs little. (b) buys placement and a known peak for LVGL's own structs, at the price of RAM and a custom allocator.

### Recommended plan

1. Fix the four duplicates that need no new design (rows 1 to 4). Measure: heap of the 51-widget list falls by about 480 bytes for texts.
   Also measure that `large_tree` has 4 style objects, not 81, and that a node with `pad_all` allocates no local style.
   The first display costs about 1.5 KB with the option set.
2. Add the per-call heap probe of this note as a test on Cortex-M4 in QEMU. Measure: it prints the table above, and the nightly run fails when a number grows.
3. Spike: `mounted` as non-movable storage made by reflection, with binding nodes and text buffers, and a state without a subject. This settles the biggest unknown, see below.
4. Build components on that storage: an instance is a member of the storage struct, a repeated part is an `inplace_vector` with a capacity. Measure: heap per row equals LVGL's structs only.
5. Regions for what the program declares.
   The parts: an alignment parameter on `lv::frame` and `lv::band`, a second RAM region and a pattern for `.rodata._ZTAX*` in `cmake/arm/mps2-an386.ld`, a documented rule to name a zeroed section `.bss.<name>`. Measure: a test in QEMU checks the frame's address against the region's bounds, and the image holds no zeros for it.
6. A panel that finishes later: the display calls `lv_display_flush_ready` when the panel says "sent". Measure: a test panel acknowledges from a timer; no copy of the band is made.
7. Decide on the arena (b) only now, with the remaining heap of a real screen known. If wanted: LVGL built with `LV_STDLIB_CUSTOM`, sizes found by the test of step 2, a contract when a screen does not fit.
8. Ask upstream for two small things.
   The first is a call that reserves room in the child, style and event lists.
   The second is creation of a widget in caller memory.
   Either removes most of the cost of (b).

The spike of step 3. Take the slice (`examples/mcu/slice.cpp`) and the 51-widget list.

- Build `lv::mounted<Tree>` as a struct from `define_aggregate`: one binding node per bound property, one text buffer per bound text, the ids.
- `lv::state` keeps an intrusive list of nodes instead of a subject. `raw()` still creates a subject on demand.
- An arc bound to a state writes back through one `value_changed` handler.
- `lv::mount` returns `std::expected<mounted, mount_error>(std::in_place, ...)`.
- Pass: both programs behave as before on v9.6.0 and the v10 pin. Heap per bound label falls from about 112 bytes to 0. `mounted` grows by 12 bytes per binding and the buffer.
- Also record: code size of the slice, and compile instructions of `tests/large_tree.cpp` under its fixed limit.
- Fail, and fall back to subjects, if a two-way widget cannot be kept in step, or `large_tree` no longer compiles under its limit.

### Choices this leaves to the owner

1. May `mounted` become non-movable? It breaks `std::vector<mounted>` and returning a named one.
2. Should a state lose its LVGL subject by default? C code would have to ask for `raw()`.
3. Should constant text use the static setter by default, with long mode "dots" as the exception?
4. Is turning off LVGL's default theme an option of ours, or left to the user's LVGL configuration?
5. Is a per-screen arena wanted at all, given that it costs RAM and needs LVGL built with a custom allocator?
6. May sizing use LVGL's private headers in a test only, to compute an arena from the tree?
