# Quick start

Build a speedometer for a 320 x 240 screen in about ten minutes. You write the screen, tie it to a typed value, and save a picture of it.

![The finished screen in the dark Signal theme: a lime dial at half its range, the text "120 km/h" and a RESET button](media/quick-start.png)

You need no display and no board. The program draws into memory and writes a PNG file.

## What you need

- GCC 16 or newer. lvgl26 uses C++26 reflection, which no other compiler has yet.
- CMake 3.30 or newer, and Ninja.

On macOS, Homebrew has all three:

```sh
brew install gcc cmake ninja
```

Anywhere else, a container with GCC 16 is the short way:

```sh
docker run --rm -it -v "$PWD":/work -w /work gcc:16 bash
apt-get update && apt-get install -y cmake ninja-build
```

## The project

Make a folder with three files: `CMakeLists.txt`, `tokens.json` and `main.cpp`. This is the whole `CMakeLists.txt`:

<!-- snippet: examples/quick_start/CMakeLists.txt -->
```cmake
# The quick start of the documentation, as a project of its own: copy this folder
# and it builds. docs/quick-start.md walks through it.
cmake_minimum_required(VERSION 3.30)
project(quick_start LANGUAGES C CXX)

include(FetchContent)

# LVGL's own heap: a picture of the whole screen needs more than LVGL's 64 KB.
set(LVGL26_LVGL_HEAP_BYTES 1048576)

# lvgl26 brings the LVGL it is pinned to. Pin a commit of lvgl26 for a product.
FetchContent_Declare(lvgl26
  GIT_REPOSITORY https://github.com/filipsajdak/lvgl26
  GIT_TAG main)
FetchContent_MakeAvailable(lvgl26)

# mp-units gives the speed its unit. lvgl26 does not include it: you choose it.
set(MP_UNITS_API_CONTRACTS NONE CACHE STRING "" FORCE)
set(MP_UNITS_API_STD_FORMAT TRUE CACHE STRING "" FORCE)
set(MP_UNITS_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(mp-units
  URL https://github.com/mpusz/mp-units/archive/refs/tags/v2.5.0.tar.gz
  SOURCE_SUBDIR src
  EXCLUDE_FROM_ALL
  SYSTEM)
FetchContent_MakeAvailable(mp-units)

add_executable(quick_start main.cpp)
target_link_libraries(quick_start PRIVATE lvgl26::lvgl26 mp-units::mp-units)
# Where #embed finds tokens.json.
target_compile_options(quick_start PRIVATE --embed-dir=${CMAKE_CURRENT_SOURCE_DIR})
```

- **lvgl26** is headers only. It fetches LVGL, the C library that draws the widgets, and builds it for you.
- **mp-units** is a units library. The speed in this program is a real quantity, `120 * km / h`, not a bare number.

## The design tokens

The colors and the spacing of the screen are design tokens: named values in a JSON file. The format is the one of the W3C design tokens group. These are a part of Signal, the design system of this project, in its dark theme. Save them as `tokens.json`:

<!-- snippet: examples/quick_start/tokens.json -->
```json
{
  "$description": "The quick start's design tokens: a part of Signal, the project's design system, in its dark theme.",
  "color": {
    "$type": "color",
    "ground": { "$value": "#0A0A0C" },
    "reading": { "$value": "#FFFFFF" },
    "muted": { "$value": "#B4B7C0" },
    "line": { "$value": "#34353D" },
    "active": { "$value": "#D4FF3A" },
    "on-active": { "$value": "#0A0A0C" }
  },
  "space": {
    "$type": "dimension",
    "edge": { "$value": "16px" },
    "gap": { "$value": "12px" },
    "button-x": { "$value": "24px" },
    "button-y": { "$value": "17px" },
    "none": { "$value": "0px" }
  },
  "stroke": {
    "$type": "dimension",
    "track": { "$value": "8px" }
  },
  "opacity": {
    "$type": "number",
    "opaque": { "$value": 1 },
    "none": { "$value": 0 }
  }
}
```

The compiler reads the file. In `main.cpp`, `#embed` brings it in and `lv::tokens` turns it into typed values:

<!-- snippet: examples/quick_start/main.cpp tokens -->
```cpp
constexpr char tokens_json[] = {
#embed "tokens.json"
    , 0};

constexpr auto tok = lv::tokens<tokens_json>;
```

- `tok.color.active` is a color and `tok.space.edge` a length. A style takes only the right kind.
- A style takes tokens, not loose numbers. A color that is not in the file does not reach the screen.
- Change a value in the file and build again: the picture changes with it.

## The screen

A screen is a tree, written as one constant:

<!-- snippet: examples/quick_start/main.cpp tree -->
```cpp
constexpr auto ui = lv::column({.flex_align = {"center", "center", "center"},
                                .size = {320 * px, 240 * px},
                                .style = {.bg_color = tok.color.ground,
                                          .bg_opa = tok.opacity.opaque,
                                          .border_width = tok.space.none,
                                          .pad_all = tok.space.edge,
                                          .pad_row = tok.space.gap,
                                          .radius = tok.space.none,
                                          .text_color = tok.color.muted}})[
    lv::label({.id = "title", .text = "SPEED"}),
    lv::arc({.id = "speedo",
             .range = {0, 240},
             .size = {108 * px, 108 * px},
             .style = {.arc_color = tok.color.line, .arc_rounded = false, .arc_width = tok.stroke.track},
             .style_indicator = {.arc_color = tok.color.active, .arc_rounded = false, .arc_width = tok.stroke.track},
             .style_knob = {.bg_opa = tok.opacity.none, .pad_all = tok.space.none},
             .value = lv::bind(speed)})[
        lv::label({.align = "center", .id = "reading", .style = {.text_color = tok.color.reading}, .text = lv::bind(speed)})],
    lv::button({.id = "reset",
                .on_clicked = &reset,
                .style = {.bg_color = tok.color.active,
                          .pad_hor = tok.space.button_x,
                          .pad_ver = tok.space.button_y,
                          .radius = tok.space.none,
                          .shadow_width = tok.space.none,
                          .text_color = tok.color.on_active}})[
        lv::label({.text = "RESET"})]];
```

- Properties go in `()`, children in `[]`.
- `.flex_align` centres the children of the column. An LVGL value with a name is written as that name in quotes.
- The reading is a child of the dial, and `.align = "center"` puts it in the middle.
- `lv::column` puts its children below each other. `lv::label`, `lv::arc` and `lv::button` are LVGL widgets.
- The properties of a widget come from LVGL itself. lvgl26 reads LVGL's headers at compile time and builds one typed member for each setter.
- Name the properties in alphabetical order: `.id`, `.range`, `.size`, `.style`, `.value`. C++ asks for the order of declaration, and the members are sorted.
- An `id` gives a widget a name that you can ask for later.
- `.style` is the look of the widget. `.style_indicator` and `.style_knob` are the looks of two parts of the dial.

The compiler reads this tree. A wrong property, a wrong value or a missing unit stops the build.

## A typed value

Put these lines above the tree:

<!-- snippet: examples/quick_start/main.cpp state -->
```cpp
constinit lv::state speed{0 * km / h};

void reset() { speed = 0 * km / h; }
```

- `lv::state` holds a value that widgets follow. `lv::bind(speed)` in the tree ties a property to it.
- The dial shows the number. The label shows the number with its unit: `120 km/h`.
- The unit is part of the type. `speed = 3 * m` does not compile, because a length is not a speed.
- `reset` is an ordinary function. The button calls it when it is clicked.

## The program

The display needs memory to draw into. A frame holds the whole screen:

<!-- snippet: examples/quick_start/main.cpp display -->
```cpp
constinit lv::frame<320, 240> frame;  // the display's memory
```

Then `main`:

<!-- snippet: examples/quick_start/main.cpp main -->
```cpp
int main(int argc, char** argv) {
  lv::application app;              // LVGL starts; it stops when app goes out of scope
  lv::display display{app, frame};  // no panel: the picture stays in the frame

  auto root = lv::mount<ui>(display.screen());  // std::expected; owns the widgets
  if (!root) return 1;

  speed = 120 * km / h;  // the arc and the label follow

  const char* path = argc > 1 ? argv[1] : "quick_start.png";
  if (!lv::write_png(display.screen(), path)) return 2;
  std::printf("wrote %s\n", path);
  return 0;
}
```

- `lv::application` is LVGL's lifetime. Nothing has to be shut down by hand.
- `lv::display` is a screen to draw on. On a device you also give it a function that sends the pixels to the panel.
- `lv::mount<ui>` creates the widgets of the tree. It returns a `std::expected`: lvgl26 throws no exceptions.
- `root` owns the widgets. They are deleted when it goes out of scope, before the display and the application.

The top of the file is the includes and two `using` lines for the units:

```cpp
#include <lv/lv.hpp>
#include <lv/png.hpp>

#include <mp-units/systems/si.h>

#include <cstdio>

using namespace lv::units;
using namespace mp_units::si::unit_symbols;
```

The whole file is [examples/quick_start/main.cpp](../examples/quick_start/main.cpp).

## Build and run

```sh
cmake -B build -G Ninja -DCMAKE_C_COMPILER=gcc-16 -DCMAKE_CXX_COMPILER=g++-16
cmake --build build
./build/quick_start
```

In the container, leave the two `-D` options out: GCC 16 is the default compiler there. The first build takes a minute or two, because it compiles LVGL. The program prints:

```text
wrote quick_start.png
```

Open the picture. It is the screen at the top of this page.

## Make a mistake

Give the title a width without a unit:

```cpp
lv::label({.id = "title", .text = "Speed", .width = 120}),
```

The build stops, and the message says what the property takes:

```text
error: static assertion failed: lv: wrong value for this property. It takes a width or x
(120 * lv::px, 50 * lv::pct or lv::size_content, not a vertical length). You gave int.
```

Write `120 * px` and it builds again.

## If you come from another framework

| You know | In lvgl26 |
|---|---|
| A QML or Slint document | The `constexpr` tree. It is C++, so there is no second language and no code generator |
| A property binding | `lv::bind(state)`. The widget follows the state |
| A signal handler or callback | `.on_clicked = &reset`, or a coroutine that waits for the event |
| `QApplication` and `exec()` | `lv::application` and `run()` |

## Where next

- The [README](../README.md) tours the rest: styles and design tokens, derived values, coroutines, animation.
- [examples](../examples/README.md) holds LVGL's own examples, rewritten with lvgl26.
