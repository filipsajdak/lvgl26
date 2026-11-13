# lvgl26

A C++26 framework over [LVGL](https://github.com/lvgl/lvgl). Namespace `lv`. MIT.

Status: early. Typed widget views, the declarative tree, styles, design tokens and state exist. Async and agent tools do not yet.

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

State is typed and lives in ordinary globals. Widgets follow it:

```cpp
constinit lv::state<int> speed{0};

constexpr auto ui = lv::column[
    lv::arc({.range = {0, 240}, .value = lv::bind(speed)}),     // follows, and writes back
    lv::label({.text = lv::bind(speed, "%d km/h")})];

speed = 120;   // both widgets update
```

A state is an `lv_subject_t` underneath (`speed.raw()`), so C code can share it. Where LVGL has a
bind function (`lv_arc_bind_value`), it is used; any other single-value property follows through a
generic observer.

Design tokens come from a W3C Design Tokens (DTCG) JSON file, read at compile time:

```cpp
constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};
constexpr auto tok = lv::tokens<tokens_json>;

lv::box({.style = {.bg_color = tok.color.surface, .radius = tok.radius.card}})
```

Each token group becomes a struct and each token a typed `lv::token<T>`: colors, dimensions
(spacing, radius, sizes), durations, numbers (opacity), font family and weight, easing curves, and the
composite types shadow, border, typography, transition and gradient. Aliases are resolved. A misspelled
token, a broken alias or an unreadable value is a compile error.

**Styles use tokens.** A design value in a style (color, length, opacity, time, number) must be a token:

```cpp
.style = {.radius = tok.card.radius}                       // ok
.style = lv::style({...}) | tok.card.border | tok.card.shadow   // composite tokens merge with |
.style = {.radius = lv::raw(3_px)}                         // a deliberate, searchable exception
.style = {.radius = 8_px}                                  // error: it takes a design token
```

Layout values (`.size`, `.range`, `.value`) and enums stay free. To accept plain values in styles,
change the project setting. Settings are a C++ specialization, not a macro:

```cpp
template <>
inline constexpr lv::config lv::cfg<lv::override>{.raw_style_values = true};
```

Keep tokens in three tiers: primitive values (`primitive.color.blue-500`), semantic roles that alias
them (`color.primary`), and component tokens that alias those (`card.radius`). UI code uses the last two.

Styles in a mounted tree are stored as `const lv_style_t` in read-only memory.

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
