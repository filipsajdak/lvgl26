# Examples

Existing UIs rewritten with lvgl26. Every port has a test, and every port is built
with all the quality presets.

| Group | Ported | Of | How a port is checked |
|---|---|---|---|
| LVGL `get_started` | 4 | 4 | Same pixels as the C original (`tests/examples_get_started.cpp`) |
| LVGL `widgets`, `layouts`, `styles`, `event`, `anim`, `scroll`, others | 0 | 295 | Same pixels as the C original |
| Qt for MCUs demos and examples | 0 | 35 | Screenshots; written from the documented behaviour only |
| Qt 6 and Qt Design Studio examples (BSD-3-Clause) | 0 | about 60 | Screenshots |

## Rules

- The examples keep the framework's defaults: styles use design tokens, states are
  strongly typed. Each example group has its own `tokens.json`.
- A port that differs from the original says so in a comment at the top of the file.
- Qt for MCUs sources are under a commercial licence: they are not read into this
  repository and no asset is copied from them.
