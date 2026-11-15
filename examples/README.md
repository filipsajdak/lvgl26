# Examples

Existing UIs rewritten with lvgl26. Every port has a test, and every port is built
with all the quality presets.

| Group | Ported | Of | How a port is checked |
|---|---|---|---|
| LVGL `get_started` | 4 | 4 | Same pixels as the C original (`tests/examples_get_started.cpp`) |
| LVGL `widgets`: label, button, arc, bar, slider, switch, checkbox, led, line, obj | 45 | 52 | Same pixels as the C original, also after clicks, drags and time (`tests/examples_widgets_*.cpp`) |
| LVGL other `widgets`, `layouts`, `styles`, `event`, `anim`, `scroll`, others | 0 | 243 | Same pixels as the C original |
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

## Rules

- The examples keep the framework's defaults: styles use design tokens, states are
  strongly typed. Each example group has its own `tokens.json`.
- A port that differs from the original says so in a comment at the top of the file.
- Qt for MCUs sources are under a commercial licence: they are not read into this
  repository and no asset is copied from them.
