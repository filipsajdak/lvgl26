# lvgl26

A C++26 framework over [LVGL](https://github.com/lvgl/lvgl). Namespace `lv`. MIT.

Status: early. Typed widget views and the declarative tree exist. Tokens, state, async and agent tools do not yet.

```cpp
#include <lv/lv.hpp>
using namespace lv::literals;

void reset();

constexpr auto ui = lv::column({.size = {320_px, 240_px}})[
    lv::label({.id = "title", .text = "Speed"}),
    lv::arc({.id = "speedo", .range = {0, 240}, .value = 120}),
    lv::button({.on_clicked = &reset})[lv::label({.text = "Reset"})]];

auto root = lv::mount<ui>(lv::screen_active());   // std::expected; owns the widgets
root->get<"speedo">().set({.value = 0});          // a typed view of the arc
```

How it works:

- **Properties are generated.** `lv::props<"arc">` is a struct that C++26 reflection builds from LVGL's
  own C API: one member per `lv_arc_set_*` and `lv_obj_set_*` function and one per event (`on_clicked`).
  Nothing is written by hand and nothing is generated offline.
- **Properties are typed.** Sizes take `120_px`, `50_pct` or `lv::size_content`; angles `90_deg`; times
  `300_ms`; opacity `50_pct`. Integers reject `bool`, floating point and values that do not fit.
- **Mistakes are compile errors.** A wrong value says what the property takes and what you gave.
  A wrong id lists the ids of the tree.
- **The tree costs nothing at run time.** `lv::mount<ui>` reads it at compile time; an optimized build
  contains only the LVGL calls for the properties that are set.
- **Views never delete.** `lv::view<"arc">` is one pointer. `lv::mount` and `lv::owned<T>` own widgets;
  `lv::weak<T>` empties itself when LVGL deletes the widget.

Rules to know:

- Designated initializers must be in alphabetical order (`.id`, `.range`, `.value`). This is a C++ rule.
- A misspelled or misordered member is reported as `could not convert '{...}' to props<"arc">`.
  GCC does not name the member in that case.
- `lv::widget<"spinbox">({...})` reaches any widget that has no short name.
- `view.set<"name">(...)` and `view.get<"name">()` are the untyped escape hatch to any `lv_*_set_*` / `get_*`.

## Requirements

- GCC 16 or newer (`-std=c++26 -freflection`)
- CMake 3.30 or newer, Ninja

## Build and test

```sh
cmake --preset host-v96 && cmake --build --preset host-v96 && ctest --preset host-v96
cmake --preset host-v10 && cmake --build --preset host-v10 && ctest --preset host-v10
```

| Preset | LVGL |
|---|---|
| `host-v96` | v9.6.0 |
| `host-v10` | master commit `53819fe4`, the v10 development line |

To build against a local LVGL checkout, add `-DFETCHCONTENT_SOURCE_DIR_LVGL=<path>`.
