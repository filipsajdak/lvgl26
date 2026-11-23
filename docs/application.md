# Application and loop

A program starts LVGL with two objects, `lv::application` and `lv::display`, and calls no C function of LVGL.

Header: `<lv/lv.hpp>`, the core; `<lv/host.hpp>` for the system's clock; `<lv/pointer.hpp>` for a mouse.

## The smallest example

This is the `main` of the [quick start](quick-start.md). It draws one picture into memory and writes it to a file:

<!-- snippet: examples/quick_start/main.cpp main -->
```cpp
constinit lv::frame<320, 240> frame;  // the display's memory

int main() {
  lv::application app;  // LVGL starts; it stops when app goes out of scope
  lv::display display{app, frame, lv::signal::dark};  // no panel; Signal is the look

  auto root = lv::mount<ui>(display.screen());  // creates the widgets, and owns them
  if (!root) return 1;

  speed = 120 * km / h;  // the dial and the text follow
  return lv::write_png(display.screen(), "quick_start.png") ? 0 : 2;
}
```

`lv::application` starts LVGL, and stops it when it goes out of scope. `lv::display` draws into the frame; its last argument is the [theme](themes.md).

## How it works

The application owns LVGL's start and end, its clock and its loop. A display is tied to an application, and a pointer to a display, as each later object is declared after the one it uses.

The frame is the display's memory, a global of the program. Its type gives the size and the alignment that LVGL asks for, so LVGL never refuses it. LVGL ignores a buffer that is not aligned, and the screen stays empty.

`app.run()` is the program's loop. A platform object, such as a window, joins it with a member `lv::in_loop` and is polled before each step. A closed window ends `run()`, unless the program takes the request.

## What you can do

### Run in a window

A program for a desktop gives the display a window, and ends `main` with `return app.run();`:

<!-- snippet: examples/window/main.cpp main -->
```cpp
int main() {
  lv::application app{lv::host};
  platform::sdl_window window{app, "lvgl26", frame};
  if (!window) return 1;
  lv::display display{app, frame, window};  // LVGL draws into the frame; the window shows it
  lv::pointer mouse{display, window};       // the window's mouse feeds it

  auto root = lv::mount<app::ui>(display.screen());
  if (!root) return 2;
  app::start(root);                         // the application's task, on the mounted tree
  return app.run();                         // until the window is closed
}
```

`lv::application app{lv::host}` takes the operating system's clock. The window is the one file that knows SDL: [A pointer](input.md) says how a platform feeds the display and the pointer.

### Tell the time on a device

A device passes its own clock, `lv::application app{{.clock = &since_start}}`. Or it tells the time from a timer interrupt, `app.advance(5 * ms)`. A host that owns the loop, such as a browser page, calls `app.step()` once a frame.

### Send the picture to a panel

`lv::display display{app, frame, &changed}` calls `void changed(lv::area)` with the part of the frame that is new. An `lv::band<320, 240, 24>` holds 24 lines instead of the whole screen, and its panel function gets each piece: `void send(lv::area, std::span<const std::byte>)`.

### Stop drawing what nobody sees

`display.hide()` stops the drawing until `display.show()`. The trees, their states and their tasks stay. A window that is hidden or minimised tells its display so itself.

### Watch the heap

`app.heap()` says how much of LVGL's heap is in use. [Targets](targets.md) gives what a screen and a task take on a microcontroller.

## What the compiler checks

A frame needs a size:

<!-- snippet: tests/fail/frame_without_size.cpp -->
```cpp
#include <lv/lv.hpp>
lv::frame<0, 240> frame;
```

<!-- message: frame_without_size -->
```text
error: static assertion failed: lv::frame<Width, Height>: both must be greater than 0
```

A source of the loop joins an application that outlives it:

<!-- snippet: tests/fail/loop_temporary_application.cpp -->
```cpp
#include <lv/lv.hpp>
struct source {
  void poll() {}
};
void run(source& s) { lv::in_loop<source> loop{lv::application{}, s}; }
```

<!-- message: loop_temporary_application -->
```text
tests/fail/loop_temporary_application.cpp:5:68: error: use of deleted function 'lv::in_loop<Source>::in_loop(lv::application&&, Source&) [with Source = source]': a source joins an application that outlives it: declare the application first, then the source
```

These stop the build too, each with a test of its own in `tests/fail/`:

- a band with no screen, or taller than its screen (`band_without_screen`, `band_taller_than_screen`);
- a panel function of the wrong shape for a frame or a band (`display_frame_temporary_panel`, `display_band_panel_without_pixels`);
- a source of the loop without `poll()` (`loop_not_a_source`).

## What it costs

| Cost | What we know | Where it comes from |
|---|---|---|
| Processor time | 24 animated arcs drawn into a frame nobody saw took 438 M instructions a second. With the display hidden, nothing is marked for drawing, and they took 15 M. | [visibility.md](https://github.com/filipsajdak/lvgl26/blob/main/docs/research/visibility.md), measured 2026-11-19 |
| RAM | The frame or the band is the program's own memory, of the size its type gives. | [display.hpp](https://github.com/filipsajdak/lvgl26/blob/main/include/lv/display.hpp), from the code |
| ROM | The application and the display objects were 1.6 KB of the code of the Cortex-M4 slice. | [Targets](targets.md), measured 2026-11-15 |
| Stack | not measured yet | not measured yet |
| Compile time | not measured yet | not measured yet |

## Rules

<!-- rules: no-boilerplate-8 fast-run-8 -->
- [`no-boilerplate-8`](rules.md#no-boilerplate-8): Start the program with `lv::application`, take the screen from an `lv::display`, and end `main` with `return app.run();`.
- [`fast-run-8`](rules.md#fast-run-8): When nobody sees the screen, say so with `display.hide()`: a display that is not visible draws nothing until it is shown again.
<!-- rules: end -->

## Limits

- The first source of the loop that can wait owns the wait, so a second window's events are read at the next step.

What is planned is on the [status page](https://filipsajdak.github.io/lvgl26/status.html).

## Names

`<lv/lv.hpp>` declares `lv::application`, `lv::display`, `lv::frame`, `lv::band`, `lv::area` and `lv::in_loop`, as [headers.json](https://github.com/filipsajdak/lvgl26/blob/main/docs/reference/headers.json) lists them; `<lv/host.hpp>` declares `lv::host`. The widgets and properties of a screen are in the reference data: [lvgl-v9.6.0.json](https://github.com/filipsajdak/lvgl26/blob/main/docs/reference/lvgl-v9.6.0.json).
