# Mode-aware looks (coffee v2, gap 9): a look whose colours follow the mode

Status: design note, nothing built. Written 2026-11-27 after `display.follow(mode, {day, dark})` landed (`docs/research/theme-follow.md`). Questions for the owner are at the end.

## The problem

`display.follow` restyles what a **theme** gives. A look (`lv::look<"tile">`, `include/lv/look.hpp`) is one constant `lv_style_t` per path of one design, so a tile that takes its colours from the project's tokens keeps its day colours when the mode turns dark. The mock-up wants the Service page's "Display: Day / Dark" row to turn the whole machine, and the coffee screens use looks for their own tokens (roast, bean, track). Today the only way is to take every colour from the Signal theme.

## What exists that a solution can use

- `detail::restyle` (`include/lv/detail/theme_styles.hpp`) walks the active screen and the three layers and puts a style where another stood with `lv_obj_replace_style`. It keeps the position in the widget's list and allocates nothing.
- `switch_to(index)` of the display is the one function that runs on a change, with the index of the value among the enumerators (`include/lv/display.hpp`).
- A look reaches a widget as a constant style in `.style` (a local style slot, an added style). A merge `style | look` makes a new constant style.
- LVGL's `lv_style_t` has no user field: nothing in a style says which look it is, or that it has a night twin.

## Where the pair lives (A, B, C) and how a switch finds it (1, 2)

Where the dark value is written:

- **A. Two token files.** `lv::look<"tile", day_design>` and `lv::look<"tile", dark_design>`, two files of the same shape (as the two Signal themes). Nothing new in the token format; the same group names must exist in both (a contract at the first use).
- **B. One file, the dark value beside the day value** in `$extensions` of the token (`io.github.filipsajdak.lvgl26`, `vendor_key`): `"track": {"$value": {...day...}, "$extensions": {"io.github.filipsajdak.lvgl26": {"dark": {...}}}}`. Design tools open the file as before and see the day value. One look reads both.
- **C. The Design Tokens Format has no modes** (2025.10); tools such as Figma export one file for each mode, so A is what a designer's export gives.

How a switch finds the twin of the style that a widget holds:

1. **A registry of pairs that fills as looks are used.** A mode look is one mutable static object (`constinit`) that holds `lv_style_t styles[2]` and an intrusive `next` pointer; building a widget that takes it joins it to the list once (no heap, one pointer). `switch_to` walks the widgets once and, for each style of a widget, asks the registry whether the pointer is in a pair (a range compare per pair), then `lv_obj_replace_style`. This is the hook `theme-follow.md` named.
2. **No registry: the widget remembers.** A node that takes a mode look stores its pair in its `lv::mounted` storage (the node's `storage`, `docs/research/node-storage.md`) and the display walks the mounted trees. That needs every mounted tree to be known to the display, which they are not today.

I propose A with 1: no change of the token format, no new walk, the cost is one pointer per used mode look.

## The call: what a user writes

- **Form X: a look takes the design list.** `lv::look<"tile", {day, dark}>` is a `lv::mode_style`, and `.style = lv::look<"tile", ...>` takes it. The style slots then accept a second type: a cost in every file (the slot's argument type is on the hot path of `large_tree`), unless the type converts to `lv::style` (its day style) and the registration happens on conversion, which cannot know the mode state.
- **Form Y: a separate property.** `.look = lv::mode_look<"tile">` (a node property that takes a mode look, joins the registry and adds the style). The ordinary `.style` stays as it is, no cost for a file that uses no mode look. The price: the look is no longer one of the styles of `.style`, and the order against the node's own style is the order of the properties (look first, as today).
- **Form Z: the look is named once and the mode comes from the display.** `lv::design<lv::override>` is already the one place that names the token file. The display's `follow` gives the list of designs (`display.follow(mode, {day, dark}, {day_tokens, dark_tokens})`) and `lv::look<"tile">` is unchanged in the tree: at build time it asks the display's mode registry. Nothing in the tree changes, but a look then depends on the display (a run-time lookup, and a tree that is mounted before the display follows has a look with no pair).

I propose Y: it costs a file that does not use it nothing, and the registry is visible in the call.

## What it costs and what it must not break

- Compile cost: the core's slot types do not change under Y. A new opt-in header `lv/mode_look.hpp` (not part of `<lv/lv.hpp>`), measured with `include_only` and the probes.
- Heap: none. Per mode look: two `lv_style_t` (the day one is the one `lv::look` makes today) and one pointer. A switch walks each widget once.
- A look in a group: `lv::group` holds no theme style, but a look in its `.style` is a local style like any other; `lv_obj_replace_style` finds it.
- A merge of two mode looks (`lv::look<"a b">` stacks) is two entries in the widget's list, each with its own pair: no new constant is made. A merge `style | mode_look` is refused with a message (a merged constant has no twin).
- Shape: both designs must name the same groups and the same properties: a compile error at the look, with a message that lists the difference (`tests/fail/mode_look_*.cpp`).

## Questions for the owner

1. Where does the dark value live: two files (A), or one file with the dark value in `$extensions` (B)? I propose A.
2. Form Y (`.look = lv::mode_look<"tile">`, opt-in header), X (`.style` takes it) or Z (the display names the designs)? I propose Y.
3. A registry that fills as looks are used (one pointer for each mode look, no heap) is acceptable? The alternative (2) needs every mounted tree known to the display.
4. A screen that is not loaded keeps the mode it was built with (as the themes do): acceptable?
5. Does the coffee example need it for the Service row only, or must every screen of steps 5-7 use mode looks for roast, bean and track? That decides whether the look in a `lv::group` and the look in a list row both have to work in the first version.

## Decisions (owner, 2026-11-27) and what was built

1. A: two token files of one shape; a shape mismatch is a compile error.
2. Y: `.look = lv::mode_look<"tile">`, an opt-in header.
3. The registry that fills as looks are used was replaced by something simpler that needs no mutable object: a mode look is a constant table of its styles in each mode, and a widget that takes it carries one LVGL event (a no-op callback, the table as user data) as a marker. A switch walks the widgets, finds the markers and replaces styles with `lv_obj_replace_style`. No list, no `constinit` object, nothing allocated at a switch, and a style that a program shares by value is never taken for a mode look's.
4. A screen that is not loaded keeps the mode it was built with.
5. All looks of the coffee machine follow the mode, also in a group and in a row of a list: both are tested (`tests/mode_look.cpp`).
