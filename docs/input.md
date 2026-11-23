# A pointer

A mouse or a touch panel is an `lv::pointer`, an object of the program tied to a display, as the display is tied to the application. It is in its own header: `#include <lv/pointer.hpp>`.

## Fed by the platform

The platform tells the pointer what happened, in order: `press(at)`, `move(to)` and `release()`, or `feed(at, lv::pressed)` and `feed(at, lv::released)`. A position is an `lv::point` in pixels of the display. A platform object can feed it instead: `lv::pointer mouse{display, window}` gives the window a handle, `lv::pointer_feed`, that it calls with each event of its mouse.

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
  app::start(root);  // the application's task, on the mounted tree
  return app.run();  // until the window is closed
}
```

The samples wait in a queue inside the object, 16 by default, so lvgl26 allocates nothing for them. LVGL reads its input every 33 ms and sees the state at that moment; without the queue a press and a release between two reads would be lost. LVGL takes the whole queue in one read, and a sample makes it read at the next step, without waiting for the period.

`lv::pointer<4> mouse{display}` makes the queue smaller. When it is full, a move is merged into the newest sample, every press and release is kept, and a sample that does not fit makes `feed` return false. Feed the pointer from the code that steps the application, not from an interrupt.

## The application's loop

`app.run()` is the program's loop, as `exec()` is in other toolkits: it ends when the window is closed. A platform object joins the loop with a member `lv::in_loop`, which joins in the constructor and leaves in the destructor, and allocates nothing. Each step first polls every source that joined, in the order they joined: the window reads its events and feeds its pointer. Between two steps the first source that can wait (`wait(lv::duration)`) waits until the next step is due or an event comes; without one, the application's `idle` function does. A source is any class with `poll()`, the concept `lv::loop_source`, so a touch controller, a keypad or a socket joins in the same way.

Use `run()` when lvgl26 owns the loop, and `step()` when something else does, such as a browser page that calls in once a frame, a test that steps time by hand, or a task of an RTOS. `step()` polls the sources too, so the application's code is the same either way. The browser example steps.

## Closing, hiding and showing the window

A close request of the window ends `run()`. The program can take it instead: `window.on_close(&decide)`, where `decide()` returns `platform::close::quit`, `platform::close::ignore` or `platform::close::hide`.

`window.hide()` and `window.show()` keep the display, its mounted trees and their state, and `run()` goes on while the window is hidden. A hidden window gets no input, and its display draws nothing: `display.visible()` is false. When it is shown, the display draws its whole screen once. A task tied to a tree goes on while the window is hidden, because the tree still exists; it ends when the tree is unmounted.

The display carries the visible state, whatever hides it. The platform writes it, `display.set_visible(false)`, when its window is hidden or minimised, and `true` when it is shown or restored. `display.hide()` and `display.show()` ask the panel to hide or show itself when it can, as the window can; a panel that cannot, such as a function, only changes the display's state, so a display without a window stops drawing too. `lv::display_of(widget)` is the display a widget is on.

## Asked by LVGL

A touch panel that is polled gives the pointer a function, which LVGL calls at each read:

```cpp
lv::pointer_sample read_panel() { return panel_touched() ? lv::pointer_sample{panel_at(), lv::pressed} : lv::pointer_sample{}; }
lv::pointer panel{display, &read_panel};
```

A lambda without captures works as well. A lambda that captures is a compile error, as is feeding a polled pointer: it has no queue.

## One application, three platforms

The applications of `examples/wasm` (`counter/app.hpp`, `bounce/app.hpp`) know no platform: a tree, states, handlers and a task, and what a platform needs: `app::width`, `app::height`, the tree `app::ui` and `app::start(root)`, which starts its task on the mounted tree. The platform mounts the tree and checks the result. Each platform gives them an `lv::display` and an `lv::pointer`:

- The browser page: `examples/wasm/platform.cpp` feeds the pointer with each event of the page as it comes, a press, a move or a release; a press and a release within one frame both wait in the queue for the next step.
- The desktop window: `examples/window/main.cpp`, shown above, through `examples/platform/sdl_window.hpp`, a window that shows the frame and a mouse that feeds the pointer. It is the one file that names SDL.
- A test: `tests/window_app.cpp` gives the counter a frame and a pointer fed from a script, and checks the clicks and the slider.

LVGL's own SDL driver is not used, and no platform calls LVGL's input functions.

## The rules

- Declare the pointer after its display, and the display after its window: each ends before what it uses. A source of the loop is declared after the application; a temporary application is a compile error, and a source that outlives its application leaves the loop when the application ends.
- A pointer costs LVGL about 500 bytes of its heap on a 64-bit host: the device and its read timer.
- A test feeds the pointer and steps the application: a click is two samples and one step, with no wait.
