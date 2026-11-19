# LVGL Safe as a second back end

Feasibility study of 2026-11-18, against `origin/main` at `fa5ccc6`.
Marks follow `docs/research/README.md`.
Every mark is dated 2026-11-18 unless its row says otherwise.
No product code was written and no spike was built.

The question: can lvgl26 get a second back end over LVGL Safe, the library LVGL writes for functional safety?
The owner's goals are the test.
Write less code with more meaning, give the compiler facts, size buffers at compile time, allocate nothing at run time.

How the work was done.

- The repository [lvgl/lvgl_safe][repo] was cloned shallow at commit `d314ad8` (2026-08-25), the preview v0.1.0.
- Its 18 public headers, its docs folder, `basic_example/main.c` and the font converter's conversion function were read.
- No code of LVGL Safe was run.
  Its static libraries were read as data, with `llvm-nm` and `llvm-objdump` of LLVM 23.1.5.
- The size of each struct was measured by compiling its headers with clang 23.1.5.
  The flags were `-std=c99 -ffreestanding`, for the targets `x86_64-linux-gnu` and `thumbv7em-none-eabi`.
  The headers include `stdio.h`; an empty stub stood in for it.
  The probe file stayed outside the repository.
- The page [lvgl.io/safe][site] and the LVGL Pro terms were fetched with `curl` and read.
- ISO 26262-8 itself was not read.
  Its clause 11 is read through one vendor paper, [Synopsys 2017][synopsys], about the 2011 edition.

## Short answers

| Question | Answer | Evidence |
|---|---|---|
| Can lvgl26 drive LVGL Safe at all? | Yes, for a subset: six widgets, absolute positions, no layout. lvgl26 would supply layout, looks, fonts and images at compile time. | Mapping below |
| May lvgl26 ship an adapter today? | Not established. No text grants any right beyond evaluation. One header says "All rights reserved". | The preview below |
| Can the preview be tried on a device? | No. It ships static libraries for x86-64 Linux and Windows only. Its configuration header may not be edited. | The preview below |
| What is the API? | 20 public functions and plain structs. The caller declares each struct, calls `create`, then writes its fields. There are no setters. | API below |
| Can the compiler lay out a screen? | Yes, for fixed content. Text that changes at run time needs a reserved width, or centre alignment on an anchor. | Compile-time section below |
| Does our reflection carry over? | Probably, in a simpler form: a widget's properties would be the members of its struct. Not tried. | Reflection section below |
| Which shape is best for safety? | (c) separate layers now. (b) a generator of C99, studied after the full delivery. (a) not recommended. | Shapes below |
| What waits for December 2026? | The source, the safety manual, the licence text, the final API, the target platforms. | Not knowable below |

## The preview

| Finding | Link | Mark |
|---|---|---|
| The repository has no licence file. GitHub's API reports `"license": null`. | [repo][repo], `gh api repos/lvgl/lvgl_safe` | `source` |
| `ls_image_dsc.h` carries "Copyright (c) 2026 LVGL Safe. All rights reserved." No other header carries a notice. | [ls_image_dsc.h][dsc] | `source` |
| The README says: "Source available under a commercial agreement, per product". The open-source LVGL stays MIT. | [README][readme] | `source` |
| The docs say the preview is "published for evaluation and feedback only" and "not ready for production use". | [docs/index.mdx][index] | `source` |
| They say: "The public API is **unstable** and **will change** without notice." | same | `source` |
| They say: "Do not deploy LVGL Safe in any real product or safety-critical system." | same | `source` |
| The preview "ships a static library of LVGL Safe, the examples, and guides for evaluation only". | [docs/delivery.mdx][delivery] | `source` |
| The full delivery lists "License & notices" among its documents. | same | `source` |
| The LVGL Pro terms (version 2025-08) define "Licensed Software" as "the LVGL UI Editor, in object code form". They do not name LVGL Safe. | [Pro terms](https://lvgl.io/pro/terms.pdf) | `source` |
| No page read says what a third party may do with the headers, the library or an adapter. | the pages above | `source` |
| The library comes in two packages, `linux-x86_64` and `windows-x86_64`, with identical headers. There is no Arm build. | [repo][repo] | `source` |
| The Linux library is one object, `lvgl_safe_internal.o`, built by "GCC: (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0". | `strings` of `liblvgl_safe.a` | `measured` |
| Its sizes on x86-64: `.text` 13,008 bytes, `.rodata` 28,128, `.bss` 20. | `llvm-size -A` | `measured` |
| It calls `memset`, `printf`, `snprintf` and `strcmp` from the C library, and no `malloc` or `free`. | `llvm-nm`, undefined symbols | `measured` |
| The Windows library calls the same, through MinGW's `__mingw_printf` and `__mingw_snprintf`. | `llvm-nm` | `measured` |
| Both hold a symbol `ls_watermark_image` and a local `ls_watermark_render`. | `llvm-objdump -t` | `measured` |
| `ls_conf.h` must not be edited: "Its values are already compiled into the static library and baked into the struct layouts". | [docs/build-and-run.mdx][build] | `source` |
| A public header, `ls_error_codes.h`, includes `stdio.h` and defines a macro that calls `printf`. | [ls_error_codes.h][err] | `source` |
| The landing page, dated August 1, 2026: "A preview release is available today for early testing and integration". | [lvgl.io/safe][site] | `source` |
| It says: "Traceability, process documentation, test evidence and MISRA compliance arrive with the full certification package by December 2026." | same | `source` |

What it means.
A back end cannot be built, tested or published from the preview alone.
The licence decides whether lvgl26 may even include the headers in its build.

## The API of the preview

Every widget struct starts with `ls_widget_common_t common`.
The caller declares a widget, passes it to `ls_<widget>_create(&screen, &widget)`, then writes its fields.
`create` "resets the struct to its defaults, installs the render callback and appends the widget to the screen's list" ([how-it-works][how], `source`).
"Configure the widget *after* this call, never before - `create` overwrites whatever was there" (same, `source`).
Fields may change "at any time - the new value is picked up by the next `ls_render()`" (same, `source`).
Widgets "cannot be removed"; `common.hidden = true` takes one off the screen (same, `source`).
The renderer "renders the full active screen every time" (same, `source`).

Sizes are x86-64 / Cortex-M, in bytes, `measured` as described above.

| Type or function | What the caller provides | Sizes | Mark |
|---|---|---|---|
| `ls_display_t`, `ls_display_init` | `hor_res`, `ver_res`, `stride`, a frame buffer of `stride × ver_res` bytes, `screen_active` | 32 / 20 | `source` |
| `ls_frame_buffer_color_t` | RGB565 only: a `uint16_t`, `LS_BYTE_PER_PIXEL` 2 | 2 a pixel | `source` |
| `ls_screen_t`, `ls_screen_init` | `bg_color`, `active_focus_index`, optionally its own `render_cb` | 40 / 24 | `source` |
| `ls_screen_add_widget` | Called by `create`; documented as used internally | | `source` |
| `ls_widget_common_t` | `indev_event_cb`, `indev_click_test_cb` and `render_cb` may be replaced; `hidden` | 48 / 24 | `source` |
| `ls_rectangle_t`, `ls_rectangle_create` | `x`, `y`, `width`, `height`, `bg_opa`, `bg_color`, `border_color`, `border_width`. No radius. | 80 / 52 | `source` |
| `ls_label_t`, `ls_label_create` | Position and box, `opa`, `color`, `font`, `letter_space`, `align`, `underline_thickness`. One text source: `text`, `translation`, or `bind_int` with `bind_fmt`. | 128 / 76 | `source` |
| `ls_label_get_text_size`, `ls_label_render` | Measure text at run time; draw a label from a custom callback | | `source` |
| `ls_button_t`, `ls_button_create` | Position and size, ten background colours and ten opacities by state, `focus_index`, `checked`, `disabled`. No text of its own. | 112 / 88 | `source` |
| `ls_image_t`, `ls_image_create` | `x`, `y`, `src`, `opa`, `a8_color`, `pivot_x`, `pivot_y`, `rotation` in degrees. The size comes from `src`. | 80 / 52 | `source` |
| `ls_image_dsc_t` | `color_format`, `width`, `height`, `stride`, `data`. The preview draws only ARGB8888 and A8 ([image.mdx][image]). | not measured | `source` |
| `ls_image_button_t`, `ls_image_button_create` | Position, ten image sources by state, `focus_index`, `checked`, `disabled` | 144 / 80 | `source` |
| `ls_arc_t`, `ls_arc_create` | `center_x`, `center_y`, `radius`, `color`, `opa`, `thickness`, `start_deg`, `end_deg`. No value, range or knob. | 80 / 52 | `source` |
| `ls_font_t` | A generated descriptor: first and last code point, `h_px`, `base_line`, `glyph_cnt`, bitmap, glyph table. Declared non-const. | 40 / 28 | `source` |
| `ls_translation_t` and its three functions | A table of tags with one string per language; `init`, `set_language`, `get` | | `source` |
| `ls_indev_process` | Once per loop: the pointer's `x`, `y`, its state and its previous state | | `source` |
| `ls_indev_event_cb_t` | `void (*)(void* widget, ls_indev_event_type_t)`: pressed, pressing, clicked. No user data, no coordinates. | | `source` |
| `ls_render` and two helpers | Render the active screen; clear the buffer; reach a pixel from a custom render callback | | `source` |
| `ls_sin`, `ls_cos` | Integer trigonometry | | `source` |
| `ls_error_code_t` | Every function returns OK, internal error, invalid argument, invalid configuration, memory corruption or render failed. | | `source` |
| `ls_conf.h` | Feature switches: indev, test, fill, image, text, rotated and perspective images, rectangle, label, image, 9-patch | | `source` |

Further findings on the API.

| Finding | Link | Mark |
|---|---|---|
| `nm` lists 20 global functions, matching the headers. The renderers of the fill, arc and images are local functions; `ls_label_render` is global. | `llvm-objdump -t` | `measured` |
| So a rectangle, arc or image cannot get its default render callback without `create`. Building widgets in constant evaluation and skipping `create` looks closed. | inference from the row above | inference |
| A label without a font, or an image without a source, makes `ls_render()` fail "for the whole frame". | [api/index.mdx][api] | `source` |
| A widget is focused when its `focus_index` equals the screen's `active_focus_index`. Moving focus is the application's code. | [how-it-works][how] | `source` |
| A centred label with `width = 0` centres on `x`. | [api/label.mdx][label] | `source` |
| `bind_int` is an `int32_t*` read "on every render" and formatted with `bind_fmt`, a printf format. | same, and `snprintf` in the library | `source` |
| The font converter writes 8 bits of coverage a pixel. Each glyph is a cell as wide as its bitmap and as high as the font. | [font_conv.py][fontconv] | `source` |
| It keeps no kerning and no side bearing. A font covers "the inclusive Unicode codepoint range", so the range is contiguous. | same, [assets.mdx][assets] | `source` |
| `ls_translation_set_language(uint32_t)` takes no context object. | [ls_translation.h][tr] | `source` |
| All 22 relocations to `.bss` in the Linux library lie in `ls_translation_init`, `_set_language` and `_get`. | `llvm-objdump -dr` | `measured` |
| The README states the goal "No internal global or static state". The two rows above show what the preview does. | [README][readme] | `source` |
| The label's align enum is spelled `lv_label_align_t` in `ls_label.h`. LVGL v10's sources at our pin have no type of that name. v9.6 was not searched. | [ls_label.h][lsl] | `source` |
| The README lists "image rotation and perspective transforms". `ls_image_t` has no perspective field; only `LS_USE_RENDER_IMAGE_PERSPECTIVE` exists. | [README][readme], [ls_image.h][lsi] | `source` |
| The README lists "LVGL Pro Editor" as the tooling of both libraries. A comment in `ls_button.h` says an array "maps badly to XML". | [README][readme], [ls_button.h][lsb] | `source` |
| No served page mentions C++, bindings or integration with another framework. LVGL Pro Editor appears only as "Tooling" in the comparison table. | [lvgl.io/safe][site], [docs](https://lvgl.io/docs/safe), its standards, comparison and delivery pages | `summary` 2026-11-19, read by an agent |
| No served page names an Arm target, a compiler or a toolchain, and none gives a roadmap of widgets or features. | same | `summary` 2026-11-19, read by an agent |

## Safety claims and tool confidence

| Finding | Link | Mark |
|---|---|---|
| The README's table, "Supported up to": ISO 26262 ASIL B, IEC 62304 Class B, IEC 61508 SIL 2, DO-178C DAL C. | [README][readme] | `source` |
| Its words: "Certification ready: compatible with ISO 26262, IEC 62304, IEC 61508, and DO-178C". | same | `source` |
| "The current v0.1.0 preview is **not yet verified, audited, or qualified**, and ships none of these artifacts." | [standards.mdx][standards] | `source` |
| "The certified element is your product, not the library." | [delivery.mdx][delivery] | `source` |
| It targets "conformance with the **mandatory rules** of MISRA C:2012". | [standards.mdx][standards] | `source` |
| The served docs say: "The safety-oriented properties described in these docs are design goals. They are not yet verified, audited, or qualified." | [lvgl.io/docs/safe](https://lvgl.io/docs/safe) | `summary` 2026-11-19, quoted by an agent |
| The served standards page adds: "Do not present it as evidence in an assessment." | [standards](https://lvgl.io/docs/safe/standards) | `summary` 2026-11-19, quoted by an agent; the repository's copy says the same |
| ISO 26262-8 table 3: impact TI1 always gives TCL1. TI2 with detection TD1, TD2 or TD3 gives TCL1, TCL2 or TCL3. | [Synopsys 2017][synopsys], figure 1 | `source`, of the paper |
| "TCL1 means that no further qualification is required." | same | `source`, of the paper |
| TCL2 and TCL3 are qualified by use, by the tool's process, by validation, or by development to a safety standard. | same, figure 4 | `source`, of the paper |
| "The user of the software tool is responsible for the TCL value assessment". Each use case is assessed on its own. | same, page 3 | `source`, of the paper |
| A tool chain whose output further tools check "could be raised to TCL1". The example is synthesis checked by equivalence checking. | same, page 3 | `source`, of the paper |
| A qualification records the tool's version and "the configuration and environment for which the software tool is qualified". | same, page 6 | `source`, of the paper |
| The paper says nothing about compilers or code generators. | same | `source`, of the paper |
| That a generator whose C output is verified can reach TD1 is our reading of the tool-chain example. | | inference |
| MISRA C++:2023 targets C++17. No edition for C++20 or later was found. | [MathWorks](https://www.mathworks.com/help/bugfinder/misra-cpp-2023-rules-and-directives.html) | `summary` |
| A compiler is qualified for one version and one target. No kit for GCC 16 was found. | [overview](https://wrocpp.github.io/toolset/qualified-compilers/) | `summary`, weak source |
| Qt Safe Renderer is certified for its own module only. It draws safety elements in a layer over an uncertified main UI. | [Qt blog](https://www.qt.io/blog/2018/05/31/function-safety-certified-qt-safe-renderer), [delivery](https://doc.qt.io/QtSafeRenderer/qtsr-delivery.html) | `summary` |

## Mapping lvgl26 onto LVGL Safe

"Has" means LVGL Safe has it.
"Substitute" means lvgl26 would build it from what LVGL Safe has, with run-time code of its own where noted.
"Lacks" means neither has it.

This table keeps lvgl26's own vocabulary: the user writes `lv::arc({.range, .value})` as today.
The back end translates each node into `ls_` structs by hand-written rules.
The reflection section below is a different, smaller thing: properties named by LVGL Safe's own fields.
Shape (b) would need both: our widgets for the user, LVGL Safe's fields for the output.

| lvgl26 | LVGL Safe | How |
|---|---|---|
| `lv::label` | Has | `ls_label_t`: text, translation, or an `int32_t` with a format. |
| `lv::button` | Has | `ls_button_t`, then its children as later widgets of the same screen. Creation order is drawing order. |
| `lv::box` | Substitute | `ls_rectangle_t`. No radius, shadow, gradient or padding. |
| `lv::group` | Substitute | Creates no widget. Its children get absolute positions. |
| `lv::column`, `lv::row` | Substitute | No flex. Positions computed at compile time, then written as `x` and `y`. |
| `lv::arc` | Substitute | Two `ls_arc_t`, track and indicator. Value and range become `end_deg`, computed when the value changes. No knob, no dragging. |
| `lv::bar` | Substitute | Two `ls_rectangle_t`: track and indicator, whose `width` follows the value. Square corners. |
| `lv::slider` | Substitute, costly | A bar, a click test, and the pointer's `x`. lvgl26 feeds `ls_indev_process`, so it knows `x`; the callback does not carry it. |
| `lv::toggle`, `lv::checkbox` | Substitute | An `ls_image_button_t` with checked images, or a button with checked colours. Our code flips `checked`. |
| `lv::image` | Has | `ls_image_t`, ARGB8888 or A8 in the preview. |
| `lv::spinner` | Substitute, run time | An `ls_arc_t` whose angles our scheduler changes each frame. |
| `lv::textarea`, `lv::dropdown`, `lv::roller` | Lacks | No text input, no list, no scrolling. |
| Lists, `lv::each` | Substitute | A fixed number of rows, all created at start and hidden when unused. No scrolling. |
| Styles and design tokens | Substitute | Resolved at compile time into fields: colour, opacity, border width, font, letter space. Radius and shadow are lost; padding becomes layout. |
| Looks and themes | Substitute | Applied at compile time. LVGL Safe has no cascade; lvgl26 would be the cascade. |
| Parts and states | Partly | Only buttons and image buttons have state fields. `lv::on("pressed")` maps to `bg_color_pressed`. |
| Units | Plain numbers | Pixels and whole degrees. mp-units quantities convert at compile time. `x` and `y` are unsigned. |
| `lv::state`, `lv::bind` | Substitute, run time | A watcher writes the field; the next render shows it. A label can read an `int32_t` mirror through `bind_int`. |
| `lv::computed` | Substitute, run time | As `lv::state`: a watcher writes a field, or a text buffer that `text` points to. |
| Events and handlers | Partly | Pressed, pressing, clicked only. No user data: a trampoline per node, generated at compile time, finds the handler. |
| Focus and keys | Partly | A focus index per screen, moved by our code. No key events. |
| `lv::mount`, `lv::mounted` | Has the shape | One caller-owned struct holds every widget. Widgets are never deleted, so `unmount` cannot exist. |
| A node that switches trees (`lv::shown`) | Substitute | Every arm created at start, all but one hidden. The storage is the sum of the arms. Or one screen per arm. |
| Tasks, `lv::spawn` | Substitute, run time | LVGL Safe has no tick and no timer. lvgl26 would need its own scheduler, driven from the render loop. |
| Animation, `lv::animate` | Substitute, run time | Fields written each frame by that scheduler. |
| Fonts made by the compiler | Has the format | Our rasteriser could write `ls_font_t`: 8-bit coverage, full-height cells, no kerning. Simpler than LVGL's format. |
| Images decoded by the compiler | Has the format | Our PNG decoder could write `ls_image_dsc_t` in ARGB8888 or A8. |
| Translations | Has | `ls_translation_t`, with library-wide state in the preview. |

Two losses for fonts.
`ls_font_t` is declared non-const and `label.font` is a non-const pointer, so the descriptor sits in RAM; its bitmap stays const.
A font covers a contiguous range, so our subsetting to the glyphs a program uses does not carry over.

## What the compiler would do for the user

LVGL Safe has no layout, so every position is a number the caller writes.
lvgl26 already knows every node, size and look of a tree at compile time.
It could compute a flex column or row there, and write each child's absolute `x` and `y`.

What the compiler knows.

- Fixed sizes, padding and gaps from looks and tokens.
- The size of an image, since it is decoded at compile time.
- The width of fixed text, from a font made at compile time.
  It must follow LVGL Safe's rule, which looks like cell widths plus `letter_space`, without kerning.
  That rule is inferred from the converter, because the renderer's source is not in the preview.

What it does not know, and the answer for each.

- Text that changes at run time, from `bind_int` or a computed string.
  Reserve the widest value of its type, or centre it on an anchor with `width = 0`.
- Translations: lay out with the widest language, all known at compile time.
- A list whose rows come and go: a fixed number of slots, each of a fixed height.
- A size that depends on run-time content: not supported, and LVGL Safe could not lay it out either.

The storage of a screen like our quick start: a title, an arc with its value as text, and a button with a label.

```cpp
// What lvgl26 would generate from the tree, as one object of fixed size.
struct speed_screen {
  ls_screen_t screen;
  ls_label_t title;       // "Speed", x and y computed from the column
  ls_arc_t dial_track;    // lv::arc becomes two arcs
  ls_arc_t dial_value;    // end_deg written when `speed` changes
  ls_label_t dial_text;   // centred on the arc's centre, width 0
  ls_button_t reset;      // on_clicked through a generated trampoline
  ls_label_t reset_text;  // created after the button, so drawn on top
  std::int32_t speed;     // the mirror that dial_text.bind_int reads
};
```

From the measured structs, its size on Cortex-M is 24 + 76 + 52 + 52 + 76 + 88 + 76 + 4 = 448 bytes.
On x86-64: 40 + 128 + 80 + 80 + 128 + 112 + 128 + 4 = 700 bytes, before padding.
The frame buffer of 320 × 240 is 153,600 bytes, as `lv::frame<320, 240>` holds it today.
The enums follow clang's default for each target.
`-fshort-enums` would change the Cortex-M numbers.

At run time the start is one `create` per widget, then writes of constants.
After start nothing is allocated, by lvgl26 or by LVGL Safe.

## Reflection over the `ls_` headers

Not tried: nothing here was compiled with GCC 16.
Today lvgl26 finds a property as a setter: `detail::find` and `resolve` in `include/lv/detail/api.hpp` look up `lv_<widget>_set_<prop>`.
LVGL Safe has no setters, so that scan would find nothing.
Its properties are the data members of each struct.
`std::meta::nonstatic_data_members_of` should list them with names and types.
Every struct is an unnamed `typedef struct`; whether GCC 16 reflects its members through the typedef is not checked.

What reflection would give, and what it would not.

- Names and types of every field, without a hand-written list, skipping `common`.
- Enumerators by name: `LS_LABEL_ALIGN_CENTER` as `"center"`, as today.
- States from field names: `bg_color_pressed_focused` reads as `bg_color` in "pressed" and "focused".
  This rests on a naming habit, not a rule of the API.
- Units: not in the types.
  `x` is pixels and `start_deg` degrees by name; `rotation` is degrees only in the docs.
  A table or a doc reading is needed, as `angle_docs.hpp` does for LVGL.
- The struct layout depends on `ls_conf.h`: `LS_USE_INDEV` adds two members.
  Reflection would see the configuration the product uses.

## The three shapes compared

Nothing below gives lvgl26 a safety level.
LVGL Safe's own wording is "The certified element is your product, not the library."

| | (a) lvgl26 headers in the product | (b) lvgl26 as a generator of C99 | (c) no integration, two layers |
|---|---|---|---|
| What is in the binary | lvgl26's C++26 code: the tree's build, watchers, trampolines, tasks, contracts. GCC 16 compiles it. | Generated C: structs, `create` calls, field writes, trampolines, font and image data. No lvgl26 code. | The safety layer: LVGL Safe and hand-written C. The rest: lvgl26 over LVGL, in another layer or partition. |
| What stays in the compiler | The constant evaluation only | All of lvgl26: tree, layout, looks, fonts, images, checks | Nothing of the safety layer |
| Who qualifies what | The product team argues a C++26 coding standard (none found) and GCC 16 (no kit found). | The team classifies the generator. Verified output may reach TCL1, by our reading. The qualified C compiler builds the output. | The team shows freedom from interference between the layers: memory, timing, composition of the images. |
| What the user gains from lvgl26 | Everything, including C++ logic, states and tasks | Layout, looks and tokens, fonts and images, compile-time checks, less code. Logic stays in C. | Nothing in the safety layer. Everything in the rest. |
| What a changed API costs | Our headers change with it | The generator's rules change with it. Only the generated C is shipped. | Nothing |
| Cost to lvgl26 | Highest, and blocked on a qualified compiler | Medium: a printing program and tests of its output | None |
| Blocked by | The language and the compiler | The licence, the source, possibly LVGL's own editor | Nothing on our side |

Notes on each shape.

- (a) GCC 16 builds code of the product.
  A qualification of GCC 16 for C++26 would be a project of its own, by the product team or a vendor.
- (b) The generator runs on the host, and its output is plain C that a reviewer can read.
  lvgl26 already prints data by reflection in `tools/reference/reference.cpp`.
  Handlers and state logic would be written in C by the user.
  So lvgl26 adds structure and data, not behaviour.
  A changed API breaks the hand-written rules, such as an arc made of two arcs; reflection helps only with plain field names.
  LVGL's own editor is listed as tooling for LVGL Safe and may already generate such C.
- (c) This is how a certified renderer is commonly used beside a full UI.
  LVGL Safe renders whole frames in RGB565, without alpha.
  So the safety layer needs its own buffer and a hardware layer or window, or a colour key.
  That is an inference from the API, not tried.

Recommendation.
(c) is the answer for a product today.
(b) is the only lvgl26 route worth a study, after the full delivery.
(a) is not recommended: it puts code in the binary that no current coding standard or qualified compiler covers.

## Not confirmed

- Whether lvgl26 may include the headers, link the preview, or publish an adapter.
  No licence text answers it.
- Whether LVGL's editor generates C for LVGL Safe today, or will in December.
- LVGL Safe's rule for the width of a line of text.
  Comparing our width with `ls_label_get_text_size` needs the preview to run.
  That is the one spike question, and it was not done.
- Whether GCC 16 reflects the members of an unnamed struct behind a C `typedef`.
- Whether the static state of the translation functions stays in the full delivery.
- Whether perspective transforms are reachable through the public API.
- Whether a callback may write fields of other widgets during `ls_indev_process`.
  The examples do it; the safety manual may restrict it.
- Every claim about ISO 26262-8 rests on one vendor paper about the 2011 edition.
- Every row marked `summary`.

## Not knowable before December 2026

- The source code: the preview is a binary, and the renderer is known only from its docs.
- The safety manual and its assumptions of use.
  They decide what an integrator, and so a generator, must and must not do.
- The licence text, and whether it covers tools that generate code for the library.
- The final API, since the preview's "will change without notice".
- The target platforms, and the compilers the delivery is verified with.
- Whether new widgets or a layout arrive.

## Not read

- Most of `examples/lvgl_safe_api_tour/main.c` (2,172 lines) and its `translations.c`.
  Only the parts on hooks and translations were searched.
- `docs/first-program.mdx`, `docs/vs-lvgl-open.mdx`, and the API pages for the rectangle, button and image button.
  Their headers were read instead.
- The body of `image_conv.py`, and of `font_conv.py` beyond its conversion function.
- The Windows library beyond its symbol table.
- The text of ISO 26262, and the itemis page on tool qualification.
- The served pages of `lvgl.io/docs/safe` were read by an agent only, on 2026-11-19.
  Its quotes match the repository's docs folder, which was read here; rows from it are marked `summary`.
- A second agent, on the tool-confidence pages, sent no report; the Synopsys paper was read here instead.

## Decisions for the owner

1. **Ask LVGL about the licence now, or wait?**
   Option A: write to LVGL, asking whether an MIT project may include the headers and publish a generator.
   Option B: wait for the December delivery and its licence text.
   Recommendation: A, because every other step depends on the answer.
   Cost of A: one message, sent in the owner's name.
   Cost of B: two months without a decision.
2. **Which shape, if any, does lvgl26 pursue?**
   Options: (c) only, written up as advice for safety products; (b) as a study; (a).
   Recommendation: document (c) on the site now, and study (b) after the full delivery.
   Cost: (c) is a page of text.
   (b) is a generator, tests of its output, and a CI job that needs licensed headers.
   (a) is blocked.
3. **Is a spike worth it before December?**
   The spike: print C99 from the quick start's tree by reflection, build it against the preview on x86-64, compare text widths.
   It answers whether compile-time layout matches LVGL Safe's text, and whether reflection reads the C structs.
   Recommendation: not before the licence answer, since the spike uses the preview beyond reading.
   Cost: a few days, outside the repository.
4. **Does lvgl26 build parts of (b) early?**
   Option A: build a compile-time layout and a full-height-cell font format now.
   Option B: wait until (b) is chosen.
   Recommendation: B, since neither part helps the LVGL back end.
   Cost of A: weeks of work that may be thrown away.
   Cost of B: none.
5. **How does the site talk about LVGL Safe?**
   Option A: name it as LVGL's library for safety products, and quote its own claims.
   Option B: say nothing until a shape is chosen.
   Recommendation: A, and never attach a safety level to lvgl26.
   Cost of A: a paragraph, reviewed again in December.
   Cost of B: none, but users ask.

[repo]: https://github.com/lvgl/lvgl_safe/tree/d314ad869db915a03b286b62a489e49c05f9bf56
[readme]: https://github.com/lvgl/lvgl_safe/blob/d314ad869db915a03b286b62a489e49c05f9bf56/README.md
[index]: https://github.com/lvgl/lvgl_safe/blob/d314ad869db915a03b286b62a489e49c05f9bf56/docs/index.mdx
[delivery]: https://github.com/lvgl/lvgl_safe/blob/d314ad869db915a03b286b62a489e49c05f9bf56/docs/delivery.mdx
[standards]: https://github.com/lvgl/lvgl_safe/blob/d314ad869db915a03b286b62a489e49c05f9bf56/docs/standards.mdx
[build]: https://github.com/lvgl/lvgl_safe/blob/d314ad869db915a03b286b62a489e49c05f9bf56/docs/build-and-run.mdx
[how]: https://github.com/lvgl/lvgl_safe/blob/d314ad869db915a03b286b62a489e49c05f9bf56/docs/how-it-works.mdx
[assets]: https://github.com/lvgl/lvgl_safe/blob/d314ad869db915a03b286b62a489e49c05f9bf56/docs/assets.mdx
[api]: https://github.com/lvgl/lvgl_safe/blob/d314ad869db915a03b286b62a489e49c05f9bf56/docs/api/index.mdx
[label]: https://github.com/lvgl/lvgl_safe/blob/d314ad869db915a03b286b62a489e49c05f9bf56/docs/api/label.mdx
[image]: https://github.com/lvgl/lvgl_safe/blob/d314ad869db915a03b286b62a489e49c05f9bf56/docs/api/image.mdx
[dsc]: https://github.com/lvgl/lvgl_safe/blob/d314ad869db915a03b286b62a489e49c05f9bf56/lvgl_safe-0.1.0-linux-x86_64/include/ls_image_dsc.h
[err]: https://github.com/lvgl/lvgl_safe/blob/d314ad869db915a03b286b62a489e49c05f9bf56/lvgl_safe-0.1.0-linux-x86_64/include/ls_error_codes.h
[tr]: https://github.com/lvgl/lvgl_safe/blob/d314ad869db915a03b286b62a489e49c05f9bf56/lvgl_safe-0.1.0-linux-x86_64/include/ls_translation.h
[lsl]: https://github.com/lvgl/lvgl_safe/blob/d314ad869db915a03b286b62a489e49c05f9bf56/lvgl_safe-0.1.0-linux-x86_64/include/ls_label.h
[lsi]: https://github.com/lvgl/lvgl_safe/blob/d314ad869db915a03b286b62a489e49c05f9bf56/lvgl_safe-0.1.0-linux-x86_64/include/ls_image.h
[lsb]: https://github.com/lvgl/lvgl_safe/blob/d314ad869db915a03b286b62a489e49c05f9bf56/lvgl_safe-0.1.0-linux-x86_64/include/ls_button.h
[fontconv]: https://github.com/lvgl/lvgl_safe/blob/d314ad869db915a03b286b62a489e49c05f9bf56/scripts/font_conv.py
[site]: https://lvgl.io/safe
[synopsys]: https://www.synopsys.com/content/dam/synopsys/solutions/documents/automotive-tool-confidence-iso-26262-wp.pdf
