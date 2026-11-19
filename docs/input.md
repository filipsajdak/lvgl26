# A pointer

A mouse or a touch panel is an `lv::pointer`, an object of the program tied to a display, as the display is tied to the application. It is in its own header: `#include <lv/pointer.hpp>`.

## Fed by the platform

The platform tells the pointer what happened, in order: `press(at)`, `move(to)` and `release()`, or `feed(at, lv::pressed)` and `feed(at, lv::released)`. A position is an `lv::point` in pixels of the display.

<!-- snippet: examples/window/main.cpp main -->
```cpp
int main() {
  lv::application app{lv::host};
  platform::sdl_window window{"lvgl26", frame};
  if (!window) return 1;
  lv::display display{app, frame, window};  // LVGL draws into the frame; the window shows it
  lv::pointer mouse{display};               // the window's mouse feeds it

  app::running application{display.screen()};
  if (!application.ok()) return 2;
  while (window.feed(mouse)) window.wait(app.step());
  return 0;
}
```

The samples wait in a queue inside the object, 16 by default, so lvgl26 allocates nothing for them. LVGL reads its input every 33 ms and sees the state at that moment; without the queue a press and a release between two reads would be lost. LVGL takes the whole queue in one read, and a sample makes it read at the next step, without waiting for the period.

`lv::pointer<4> mouse{display}` makes the queue smaller. When it is full, a move is merged into the newest sample, every press and release is kept, and a sample that does not fit makes `feed` return false. Feed the pointer from the code that steps the application, not from an interrupt.

## Asked by LVGL

A touch panel that is polled gives the pointer a function, which LVGL calls at each read:

```cpp
lv::pointer_sample read_panel() { return panel_touched() ? lv::pointer_sample{panel_at(), lv::pressed} : lv::pointer_sample{}; }
lv::pointer panel{display, &read_panel};
```

A lambda without captures works as well. A lambda that captures is a compile error, as is feeding a polled pointer: it has no queue.

## One application, three platforms

The applications of `examples/wasm` (`counter/app.hpp`, `bounce/app.hpp`) know no platform: a tree, states, handlers and a task, and the three things a platform needs, `app::width`, `app::height` and `app::running`. Each platform gives them an `lv::display` and an `lv::pointer`:

- The browser page: `examples/wasm/platform.cpp` feeds the pointer with what the page reports in each step, a press, a move or a release.
- The desktop window: `examples/window/main.cpp`, shown above, through `examples/platform/sdl_window.hpp`, a window that shows the frame and a mouse that feeds the pointer. It is the one file that names SDL.
- A test: `tests/window_app.cpp` gives the counter a frame and a pointer fed from a script, and checks the clicks and the slider.

LVGL's own SDL driver is not used, and no platform calls LVGL's input functions.

## The rules

- Declare the pointer after its display: it is deleted before the display, and keeps a pointer to it.
- A pointer costs LVGL about 500 bytes of its heap on a 64-bit host: the device and its read timer.
- A test feeds the pointer and steps the application: a click is two samples and one step, with no wait.
