# Quick start

A speedometer for a 320 x 240 screen, in one short file. It shows the three ideas of lvgl26: a screen is a tree, a value is typed, and widgets follow a value.

![The finished screen: a title, a dial at half its range with "120 km/h" inside it, and a Reset button](media/quick-start.png)

You need no display and no board. The program draws into memory and writes a PNG file.

## The program

This is all of `main.cpp`, in three parts.

**A value.** `lv::state` holds a value that widgets follow. The speed has a unit, so `speed = 3 * m` does not compile.

<!-- snippet: examples/quick_start/main.cpp state -->
```cpp
constinit lv::state speed{0 * km / h};

void reset() { speed = 0 * km / h; }
```

**A screen.** It is a tree, written as one constant. Properties go in `()`, children in `[]`. `lv::bind(speed)` ties a property to the value.

<!-- snippet: examples/quick_start/main.cpp tree -->
```cpp
constexpr auto ui = lv::column({.flex_align = {"center", "center", "center"}, .size = {320 * px, 240 * px}})[
    lv::label({.text = "Speed"}),
    lv::arc({.range = {0, 240}, .size = {120 * px, 120 * px}, .value = lv::bind(speed)})[
        lv::label({.align = "center", .text = lv::bind(speed)})],
    lv::button({.on_clicked = &reset})[lv::label({.text = "Reset"})]];
```

**A main.** Two objects start LVGL and give it a display. `lv::mount` creates the widgets.

<!-- snippet: examples/quick_start/main.cpp main -->
```cpp
constinit lv::frame<320, 240> frame;  // the display's memory

int main() {
  lv::application app;              // LVGL starts; it stops when app goes out of scope
  lv::display display{app, frame};  // no panel: the picture stays in the frame

  auto root = lv::mount<ui>(display.screen());  // creates the widgets, and owns them
  if (!root) return 1;

  speed = 120 * km / h;  // the dial and the text follow
  return lv::write_png(display.screen(), "quick_start.png") ? 0 : 2;
}
```

Above them go the includes and two `using` lines for the units:

```cpp
#include <lv/lv.hpp>
#include <lv/png.hpp>

#include <mp-units/systems/si.h>

using namespace lv::units;
using namespace mp_units::si::unit_symbols;
```

The whole file is [examples/quick_start/main.cpp](../examples/quick_start/main.cpp).

## Build and run

You need GCC 16, CMake 3.30 and Ninja. On macOS: `brew install gcc cmake ninja`. Anywhere else, the container `gcc:16` with `apt-get install -y cmake ninja-build`.

Put this `CMakeLists.txt` next to `main.cpp`:

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
```

Then:

```sh
cmake -B build -G Ninja -DCMAKE_C_COMPILER=gcc-16 -DCMAKE_CXX_COMPILER=g++-16
cmake --build build
cd build && ./quick_start
```

In the container, leave the two `-D` options out. The first build takes a minute or two, because it compiles LVGL. Then open `quick_start.png`.

## What you just used

| Idea | In the program | It gives you |
|---|---|---|
| The tree | `constexpr auto ui = lv::column(...)[...]` | The compiler reads the screen. A wrong property or a missing unit stops the build. |
| Typed values | `120 * km / h`, `320 * px` | A length is not a speed, and a number is not a length. |
| State and binding | `lv::state`, `lv::bind(speed)` | Change the value; the dial and the text follow. |
| Names, not C | `.align = "center"` | An LVGL value is its name in quotes. A wrong name lists the right ones. |
| Objects own things | `lv::application`, `lv::display`, `root` | Nothing to shut down or delete by hand. |

## Try a mistake

Give the title a width without a unit: `lv::label({.text = "Speed", .width = 120})`. The build stops and says what the property takes:

```text
error: static assertion failed: lv: wrong value for this property. It takes a width or x
(120 * lv::px, 50 * lv::pct or lv::size_content, not a vertical length). You gave int.
```

## If you come from another framework

| You know | In lvgl26 |
|---|---|
| A QML or Slint document | The `constexpr` tree. It is C++: no second language, no code generator |
| A property binding | `lv::bind(state)` |
| A signal handler or callback | `.on_clicked = &reset`, or a coroutine that waits for the event |
| `QApplication` and `exec()` | `lv::application` and `run()` |

## Where next

- The [README](../README.md) tours the rest: styles and design tokens, derived values, coroutines, animation.
- [examples](../examples/README.md) holds LVGL's own examples, rewritten with lvgl26.
