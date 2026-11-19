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

  auto root = lv::mount<demo::ui>(display.screen());
  if (!root) return 2;
  lv::spawn(demo::tick());
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

The application of `examples/window/app.hpp` knows no platform: a tree, states, handlers and a task.

<!-- snippet: examples/window/app.hpp ui -->
```cpp
inline constexpr auto ui = lv::column(
    {.flex_align = {"center", "center", "center"}, .size = {width * px, height * px}})[
    lv::label({.text = "lvgl26 in a window"}),
    lv::arc({.range = {0, most_of_dial}, .size = {130 * px, 130 * px}, .value = lv::bind(dial)})[
        lv::label({.align = "center", .text = lv::bind(dial, "{}")})],
    lv::row({.flex_align = {"center", "center", "center"}, .size = {lv::size_content, lv::size_content}})[
        lv::button({.id = "fewer", .on_clicked = &fewer})[lv::label({.text = "-"})],
        lv::label({.text = lv::bind(clicks, "{} clicks")}),
        lv::button({.id = "more", .on_clicked = &more})[lv::label({.text = "+"})]],
    lv::row({.flex_align = {"center", "center", "center"}, .size = {lv::size_content, lv::size_content}})[
        lv::slider({.id = "volume", .range = {0, most_of_dial}, .size = {200 * px, 12 * px}, .value = lv::bind(volume)}),
        lv::toggle({.checked = lv::bind(lamp)}),
        lv::label({.text = lv::bind(ticks, "{} ticks")})]];
```

The desktop window of `examples/window/main.cpp` gives it an `lv::display` that shows the frame in an SDL window, and an `lv::pointer` fed by the window's mouse. `examples/platform/sdl_window.hpp` is that adapter: it names SDL, and the application does not. The test `window_app` gives the same application a frame and a pointer fed from a script, and a device gives it its panel and its touch controller. LVGL's own SDL driver is not used.

## The rules

- Declare the pointer after its display: it is deleted before the display, and keeps a pointer to it.
- A pointer costs LVGL about 500 bytes of its heap on a 64-bit host: the device and its read timer.
- A test feeds the pointer and steps the application: a click is two samples and one step, with no wait.
