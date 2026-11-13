# lvgl26

A C++26 framework over [LVGL](https://github.com/lvgl/lvgl). Namespace `lv`. MIT.

Status: early. Typed widget views (L1) exist. The declarative tree, tokens, state and agent tools do not yet.

```cpp
#include <lv/lv.hpp>

auto screen = lv::screen_active();
auto title  = screen.add<"label">().set<"text">("Speed");
auto dial   = screen.add<"arc">().set<"range">(0, 240).set<"value">(120);

screen.add<"button">().on<LV_EVENT_CLICKED>([dial] { dial.set<"value">(0); });
```

Widget and property names are checked at compile time against LVGL's own C API, read by reflection.
`set<"text">` calls `lv_label_set_text`; a wrong name is a compile error that lists the valid ones.
A view is one pointer and never deletes its widget. `lv::owned<T>` does; `lv::weak<T>` empties itself
when LVGL deletes the widget.

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
