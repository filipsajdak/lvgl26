# Application and loop

A program starts LVGL with two objects, `lv::application` and `lv::display`, and calls no C function of LVGL.

Header: `<lv/lv.hpp>`, the core.

## Two objects

This is the `main` of the [quick start](quick-start.md):

<!-- snippet: examples/quick_start/main.cpp main -->
```cpp
constinit lv::frame<320, 240> frame;  // the display's memory

int main() {
  lv::application app;                                // LVGL starts; it stops when app goes out of scope
  lv::display display{app, frame, lv::signal::dark};  // no panel; Signal is the look

  auto root = lv::mount<ui>(display.screen());  // creates the widgets, and owns them
  if (!root) return 1;

  speed = 120 * km / h;  // the dial and the text follow
  return lv::write_png(display.screen(), "quick_start.png") ? 0 : 2;
}
```

- `lv::application` starts LVGL, and stops it when it goes out of scope.
- `lv::display` draws into the frame. Without a panel the picture stays in the frame, and here it becomes a PNG file.
- The last argument of a display is its [theme](themes.md).
- A program that runs ends with `return app.run();`. It runs timers, animations and input until `app.quit()`.

## Time

- `lv::application app{lv::host}` takes the operating system's clock, from `<lv/host.hpp>`.
- A device passes its own clock, `lv::application app{{.clock = &since_start}}`.
- Or the device tells the time from a timer interrupt: `app.advance(5 * ms)`.

## A panel

- `lv::display display{app, frame, &changed}` calls `void changed(lv::area)` with the part of the frame that is new.
- An `lv::band<320, 240, 24>` holds 24 lines instead of the whole screen. Its panel function gets each piece: `void send(lv::area, std::span<const std::byte>)`.
- A frame or a band has the size and the alignment that LVGL asks for, by its type. LVGL ignores a buffer that is not aligned, and the screen stays empty.

## The loop

`app.run()` is the program's loop. A platform object, such as a window, joins it with a member `lv::in_loop` and is polled before each step.

A closed window ends `run()`, unless the program takes the request. A hidden window keeps its trees and draws nothing.

A host that owns the loop, such as a browser page, calls `app.step()` instead. [A pointer](input.md#the-applications-loop) says more about the loop and the window.

## The heap

`app.heap()` says how much of LVGL's heap is in use. [Targets](targets.md) gives what a screen and a task take on a microcontroller.
