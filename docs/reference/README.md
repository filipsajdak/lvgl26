# Reference data

The reference of lvgl26 as JSON, one file for each LVGL it builds with: `lvgl-v9.6.0.json` and `lvgl-v10-pin.json`. Editor completion, the agents' tools and the reference pages read it.

`tools/reference/reference.cpp` prints a file by reflection, through the functions that generate the library's own structs. The test `reference_data` fails when a file here differs from what the program prints.

| Key | Holds |
|---|---|
| `lvgl` | The LVGL version the file was made with |
| `base` | The members every widget has: `id`, the `style` members, the properties of `obj`, the events |
| `widgets` | For each other widget, the properties it adds |
| `style` | The members of a style |
| `enums` | For each enum, the names of its values as code writes them in quotes |

A member has a `name` and a `kind`: `property`, `event`, `style` or `id`. A property also has `sets`, the LVGL function behind it, and `takes`: one entry for each value. An entry says how to `write` the value, and names its `enum` when it is one.
