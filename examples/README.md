# Examples

Existing UIs rewritten with lvgl26. Every port has a test, and every port is built
with all the quality presets.

| Group | Ported | Of | How a port is checked |
|---|---|---|---|
| LVGL `get_started` | 4 | 4 | Same pixels as the C original (`tests/examples_get_started.cpp`) |
| LVGL `widgets`: label, button, arc, bar, slider, switch, checkbox, led, line, obj | 45 | 52 | Same pixels as the C original, also after clicks, drags and time (`tests/examples_widgets_*.cpp`) |
| LVGL `widgets`: buttonmatrix, dropdown, spinbox, textarea, image, scale, keyboard | 22 | 44 | Same pixels as the C original, also after a choice, a key or a click (`tests/examples_widgets_*.cpp`) |
| LVGL other `widgets`, `layouts`, `styles`, `event`, `anim`, `scroll`, others | 0 | 199 | Same pixels as the C original |
| Qt for MCUs demos and examples | 0 | 35 | Screenshots; written from the documented behaviour only |
| Qt 6 and Qt Design Studio examples (BSD-3-Clause) | 0 | about 60 | Screenshots |

## Not ported yet

| Example | What it needs |
|---|---|
| `label_translation` | LVGL's translation tables (`lv_translation_*`), which the framework does not wrap |
| `arc_knob_widget` | `lv_arc_rotate_obj_to_angle`: a view reaches `set` and `get` functions only |
| `slider_styling`, `switch_styling`, `checkbox_styling` | A style for one part in one state (the knob while pressed) |
| `obj_draggable` | The input device (`lv_indev_get_vect`), which the framework does not wrap |
| `obj_transform` | `LV_DRAW_TRANSFORM_USE_MATRIX`, which the test build does not enable |
| `roller_*` (6 of 6) | A quoted enumerator beside a string: `.options = {"Mon\nTue", "normal"}` is refused (see Findings) |
| `roller_bind_value` | A state that follows a roller: LVGL names its bind function `lv_roller_bind_value`, the setter is `selected` |
| `spinner_basic` | `.arc_sweep = 270 * deg` does not compile (see Findings) |
| `table_*` (8 of 8) | More than one cell: `.cell_value` takes a row, a column and a text, and a props struct names it once |
| `buttonmatrix_button_ctrl`, `_button_width`, `_one_checked`, `_popover`, `_recolor`, `keyboard_custom_map` | An array of control flags, written with LVGL's C enumerators |
| `buttonmatrix_styling`, `textarea_cursor` | A style for one part in one state (`items` and checked, `cursor` and focused) |
| `dropdown_styling` | The style of the list part: the list is a widget of its own, reached by `lv_dropdown_get_list` |
| `textarea_buttonmatrix`, `textarea_format` | `lv_textarea_add_text`, `lv_textarea_delete_char` and `lv_textarea_add_char`, which are not set or get functions |
| `textarea_password`, `keyboard_custom_draw` | A keyboard joined to a text area (`.textarea` takes a widget pointer), and draw task events |
| `spinbox_with_buttons` | `lv_obj_align_to` (placing a widget next to another) and symbol pictures on a button |
| `image_recolor`, `image_svg` | Not tried: a style property bound to a state (`lv_obj_bind_style_prop`), and an SVG picture |

Not tried yet, and expected to need functions that are neither set nor get functions:
`list_*` (`lv_list_add_button`), `msgbox_*` (`lv_msgbox_add_title`), `tabview_*` (`lv_tabview_add_tab`),
`tileview_*` (`lv_tileview_add_tile`), `menu_*`, `win_*` (`lv_win_add_title`), `calendar_*` (`lv_calendar_add_header_arrow`),
`chart_*` (`lv_chart_add_series`), `canvas_*` (draw functions), `span_*` (`lv_spangroup_add_span`),
`scale_*` with sections or needles, `imagebutton_basic`, `animimg_basic`, `arclabel_curved_text`, `lottie_*`.

## Findings

Each finding is a gap in lvgl26 that stopped a port. The port is left out and no workaround is used.

A quoted name works only when every value of a property is an enumerator. The roller mixes a string and one:

```cpp
lv::roller({.options = {"Mon\nTue\nWed", "normal"}})  // refused: it takes const char*, then lv_roller_mode_t
```

An angle for a spinner does not compile with `-Werror=sign-conversion` (the setter takes `uint32_t`):

```cpp
lv::spinner({.arc_sweep = 270 * lv::deg})  // props.hpp: conversion to uint32_t from int
```

A table has one `cell_value` in its props, so the C original's several `lv_table_set_cell_value` calls have no form:

```cpp
lv::widget<"table">({.cell_value = {0, 0, "City"}})  // and the cell (0, 1), (1, 0) ... ?
```

A control map is an array of enumerators, and there is no way to write them by name:

```cpp
constexpr lv_buttonmatrix_ctrl_t ctrl[] = {"none", "checkable"};  // wanted
```

The names of `lv_image_align_t` are not shortened, because `_lv_image_align_auto_transform` has no `LV_` prefix like the others.
The port writes `.inner_align = "lv_image_align_top_left"`, where the other enumerators read `"top_left"`.

A state follows a dropdown in one direction only, because LVGL names its bind function `lv_dropdown_bind_value` and the setter is `selected`.
`dropdown_bind_value` writes the choice back in an `on_value_changed` handler.

The theme gives a screen a gap between its children.
An example that lays out the screen itself (a row or a column with `space_evenly`) needs that gap.
Its port sets the layout on the screen with `screen.set({...})`, as the `switch_orientation` port does.
A group in the screen's place has no gap, and the pictures differ.

## Rules

- The examples keep the framework's defaults: styles use design tokens, states are
  strongly typed. Each example group has its own `tokens.json`.
- A port that differs from the original says so in a comment at the top of the file.
- Qt for MCUs sources are under a commercial licence: they are not read into this
  repository and no asset is copied from them.
