# Input devices without the C API

Research and design for a type of lvgl26 that feeds LVGL's input, written 2026-11-19.

Marks follow `docs/research/README.md`.
Every mark is dated 2026-11-19 unless its row says otherwise.

- `source`: LVGL's code in the build trees of v9.6.0 and of the v10 pin `53819fe`, or our own code.
- `measured`: a sketch compiled with GCC 16.2 and run against both LVGL versions; "Measured here" says how.

The question comes from the WebAssembly spike (branch `spike/wasm-native`).
Its `examples/wasm/platform.cpp` calls `lv_indev_create`, `lv_indev_set_type` and `lv_indev_set_read_cb` for a pointer.
lvgl26 has no type for an input device, so a program that owns its display has to call LVGL's C API.
The window examples have the same gap: `examples/ports/slint/printer/window.cpp` calls `lv_sdl_mouse_create`.
The status page plans the row "A window display without C": an SDL window and a mouse as objects of `lv::`.

## Short answers

| Question | Answer | Evidence |
|---|---|---|
| Can a pointer be an object of `lv::`, like `lv::display`? | Yes: one LVGL input device with a read function of ours, made in the constructor and deleted in the destructor | Sketch, LVGL's API |
| What does an input device cost? | 488 bytes in 2 blocks of LVGL's heap on a 64-bit host: the device and its read timer. The same on both versions | Sketch |
| Why did the spike miss short presses? | LVGL reads the state at each tick of a 33 ms timer. A press and a release between two ticks are never seen | Sketch: 0 clicks |
| Can that be fixed once, in the library? | Yes: the device keeps a small queue of samples and drains it in one read with `continue_reading` | Sketch: 1 click, 2 clicks, a whole drag |
| Can a press be answered without waiting for the timer? | Yes: making the read timer ready when a sample arrives reads it at the next step | Sketch: 1 click with no time passed |
| How do keypad and encoder fit? | The same queue and read function, with another sample type and a focus group | LVGL's API, reasoned |
| How do tests drive it? | Through the same queue: a click is two samples and one step, with no waits | Sketch, reasoned for the API |

## Findings in LVGL

All rows are `source`, read in both versions; they agree unless a row says otherwise.

| Finding | Where | What it means for us |
|---|---|---|
| Four kinds: pointer (mouse, touch), keypad, button (a hardware button at a point of the screen), encoder | `include/lvgl/indev/lv_indev.h`, `lv_indev_type_t` | A pointer first; the others later, without changing the pointer |
| One read callback fills `lv_indev_data_t`: a point, a key, a button id, an encoder step count, a state and `continue_reading` | same, `lv_indev_data_t` | One read function per kind of sample |
| `lv_indev_create` allocates the device in a linked list and creates its read timer with the period `LV_DEF_REFR_PERIOD`, 33 by default | `src/indev/lv_indev.c`, `lv_indev_create` | LVGL allocates, once per device; lvgl26 allocates nothing |
| A new device is tied to the default display | same | The type takes the display, and calls `lv_indev_set_display` |
| The read loop calls the callback again while `continue_reading` is true, except in event mode | same, `lv_indev_read` | A queue of samples can be drained in one read |
| Each sample of a pointer runs the whole pointer processing: press, move, release, click | same, `indev_pointer_proc` | A press and a release in one read make a click; measured |
| In event mode the timer is paused, and `lv_indev_read` is called by the program; the timer resumes during a press, for long presses | same, `lv_indev_set_mode`, `indev_proc_press` | A second way to feed; not needed with the queue |
| `lv_timer_ready` makes a timer run at the next `lv_timer_handler` | `include/lvgl/misc/lv_timer.h` | A sample can be read at the next step, not up to 33 ms later |
| A screen animation blocks all input (`disp->prev_scr`) | `lv_indev_read` | Samples wait in the queue meanwhile; the queue needs a size |
| A keypad or an encoder moves the focus of the device's group; it has no effect without one | `lv_indev_set_group`, `lv_group_t` | A keypad needs a group; lvgl26 has none today |
| The test driver holds a press for 50 ms: `lv_test_mouse_click_at` waits between press and release | `src/debugging/test/lv_test_indev.c` | LVGL's own tests work around the same limit |
| The SDL driver makes its own device and reads SDL's events in its own timer: `lv_sdl_mouse_create` | `src/drivers/sdl/` | An SDL window can either wrap LVGL's driver or feed our queue |

From our code, also `source`:

- `lv::display` (`include/lv/display.hpp`) takes the application, a frame or a band, and a panel function or a named object, and keeps a pointer to the object. A temporary is a compile error.
- `lv::application` (`include/lv/application.hpp`) owns LVGL's lifetime and the step; `advance()` is safe in an interrupt.
- `tests/lists_selection.cpp` calls `lv_group_create` and `lv_group_set_default` itself: no type of ours makes a group.
- The spike's page holds a press for 50 ms before the release (`tools/wasm/page/index.html`, `MIN_HOLD_MS`); `web-target.md` records it.

## Measured here

GCC 16.2 on macOS arm64, against the LVGL libraries of the `host-v96` and `host-v10` builds.
The sketch is outside the repository.
It makes a button and a slider, a pointer device with a read function, and steps LVGL with its own clock.
"Queue" is a ring of 8 samples, drained with `continue_reading`.
Every number was the same on both versions.

| Case | Result |
|---|---|
| Heap of one pointer device | 488 bytes in 2 blocks; given back by `lv_indev_delete` |
| Level, as in the spike: press and release 5 ms apart, then one read | 0 clicks |
| Queue: the same press and release, then one read | 1 click |
| Queue, and the read timer made ready on the push; one step with no time passed | 1 click |
| Queue: two clicks between two reads | 2 clicks |
| Queue: press, two moves and a release on a slider of 0 to 100, in one read | The slider at 95 |

## The design rule

The input device is the counterpart of `lv::display`.

- The program owns it, declared after the display, and it ends before the display.
- It is tied to one display, given in the constructor.
- The platform feeds it typed values; the device turns them into LVGL's samples.
- Nothing is lost between two reads, and nothing waits for the timer when it need not.
- lvgl26 allocates nothing: the queue is part of the object, its size a template argument.
- Mistakes are compile errors: a feed of the wrong shape, a temporary, a keypad without a group.

## A. How the platform feeds a device

| | F1 A value set each step (the spike) | F2 A function the device calls (LVGL's way) | F3 Samples pushed into a queue |
|---|---|---|---|
| Platform code | `pointer = {x, y, down}` before `step()` | `lv::pointer mouse{display, &read_touch}`; `read_touch()` returns the state | `mouse.press(at)`, `mouse.move(to)`, `mouse.release()` |
| A short press | Lost unless held for one read period: every platform repeats the spike's 50 ms hold | Lost the same way | Kept: measured |
| Latency of a press | Up to 33 ms | Up to 33 ms | The next step: the push makes the read timer ready |
| From an interrupt | No: the value is read by LVGL's timer while it changes | The function runs in LVGL's step, not in the interrupt | Yes, with a queue of one producer and one consumer |
| A touch controller polled over I2C | Awkward | Natural: the read function polls it | Needs a poll loop of the program's own |
| Allocation | None | None | None: the ring is in the object |

Recommended: F3 for events (browser, SDL, an interrupt of a touch controller), with F2 kept as a second constructor for a device that is polled.

- F3 is what removes the workaround from every platform: the queue answers the missed press once.
- F2 is the honest model of a polled touch panel, and costs one constructor.
- F1 adds nothing that F3 does not do better.

## B. The shape of the types

| | T1 One class per kind | T2 One template over the kind | T3 One class, the kind as a run-time value |
|---|---|---|---|
| Use site | `lv::pointer mouse{display}` | `lv::input<lv::pointer> mouse{display}` | `lv::input mouse{display, "pointer"}` |
| A keypad call on a pointer | Not declared: a compile error | Not declared, through a constraint | A contract violation at run time |
| Open-closed | A new kind is a new class over a shared detail queue | A new kind is a new sample type | A new kind changes the class |
| Editor completion | Only what the kind has | Only what the kind has, through stubs per kind | Everything |

Recommended: T1, with one detail template for the queue and the read function that every kind uses (the open-closed part).

- `lv::pointer`, then `lv::keypad` and `lv::encoder`, read like `lv::display` and `lv::frame`.
- The shared part is `detail::input_queue<Sample, Capacity>`, as `detail::one_of_region` is the shared part of `lv::one_of`.

## C. What a sample is

- A position is `lv::point{x, y}` in pixels of the display, beside `lv::area`.
- A pointer sample is a position and a state, `lv::pressed` or `lv::released`, never a bare bool.
- `mouse.feed(at, state)` is the one call a platform needs; `press(at)`, `move(to)` and `release()` are short forms of it for code that has events.
- A key is LVGL's key by name in quotes, as enumerators are written elsewhere: `keys.press("enter")`; a character is `keys.press('a')`.
- An encoder step count is `lv::counter`, signed by direction: `knob.turn(lv::counter{-2})`.
- A full queue keeps presses and releases and merges moves; when it still has no room, the push returns false. That is input from outside, so it is no contract.

## D. Focus for keypad and encoder

A keypad or an encoder does nothing without a group, and lvgl26 has no group today.

| | G1 The display makes a default group | G2 A type `lv::focus` the program declares | G3 The keypad makes its own group |
|---|---|---|---|
| Use site | `lv::keypad keys{display}` | `lv::focus order{display}; lv::keypad keys{display, order}` | `lv::keypad keys{display}` |
| Widgets join it | Every focusable widget, as LVGL's default group does | The same, through the default it sets | The same |
| Two keypads on one display | Share it | Share it, or one each | One each: the focus differs per device |
| Cost | One group on LVGL's heap per display, made when the first keypad is | One per declared object | One per keypad |

Recommended: G1 for this slice, G2 when a program needs two orders.

- `lists.md` measured that a button row joins the default group, and `shown-theme-format.md` decided where the focus goes after a switch: both assume one group per display.

## E. A window and its mouse

LVGL's SDL driver makes its own display and its own mouse.

| | W1 Wrap LVGL's driver | W2 Our own SDL adapter feeding `lv::pointer` |
|---|---|---|
| Use site | `lv::sdl_window window{app, 320, 240}`, with its mouse inside | `lv::sdl_window window{app, frame}` and `lv::pointer mouse{window.display()}` fed from SDL's events |
| Code of ours | Small: a constructor and a destructor, behind an option `LVGL26_SDL` found by reflection | An event loop of ours, and a texture upload |
| The display | LVGL's own, not an `lv::display` | An `lv::display` like every other |
| Short presses | As LVGL's driver handles them | Through the queue |

Recommended: W1 first, because the window examples only need it to drop their two C calls; W2 when an application must treat the window like any other display.

## F. Tests

- A test feeds the same queue: `mouse.press({60, 35}); mouse.release(); app.step();` and checks the click, with no wait.
- The read timer is made ready on the push, so one step reads the queue; a test of the timer itself advances 33 ms.
- LVGL's test driver (`lv_test_mouse_click_at`) stays for the library's own tests of widgets, where it is used today.
- A test checks the dropped and merged samples of a full queue, and that a short press makes a click on both versions.

## Use-site sketches

These are sketches of the proposal, not code that compiles today.

A browser page, the spike's platform file without C:

```cpp
constinit lv::frame<480, 360> picture;
lv::application app{{.clock = &page_clock}};
lv::display display{app, picture, &changed};
lv::pointer mouse{display};                       // a queue of 16 samples by default

extern "C" void web_pointer(std::int32_t x, std::int32_t y, std::int32_t down) {
  mouse.feed({x, y}, down ? lv::pressed : lv::released);   // a press of 5 ms is still a click
}
extern "C" std::uint32_t web_step(std::uint32_t now) { page_time = now; return app.step().ms; }
```

An SDL window, wrapping LVGL's driver (W1):

```cpp
lv::application app{lv::host};
lv::sdl_window window{app, 800, 480};             // LVGL's window and its mouse; needs LVGL26_SDL
auto root = lv::mount<printer::ui>(window.screen());
return app.run();
```

A touch panel on a microcontroller, polled (F2) or fed from its interrupt (F3):

```cpp
lv::touch read_panel() { return gt911.touched() ? lv::touch{gt911.at(), lv::pressed} : lv::touch{}; }
lv::pointer panel{display, &read_panel};          // polled at each read of LVGL

lv::pointer<4> panel_queue{display};              // or a queue of 4, fed by the interrupt
void touch_interrupt() { panel_queue.feed(gt911.at(), gt911.state()); }
```

## Decisions for the owner

1. How does a platform feed a device?
   Options: F1 a value per step; F2 a function the device calls; F3 samples pushed into a queue.
   Recommendation: F3, with F2 as a second constructor for a polled device.
   Cost: a queue in each device object; 16 samples of a pointer are 192 bytes on a 64-bit host.
2. One class per kind, one template, or one class with a run-time kind?
   Recommendation: one class per kind over a shared detail queue: `lv::pointer`, later `lv::keypad` and `lv::encoder`.
   Cost: a stub per kind for the editor.
3. Is a press answered at the next step?
   Options: wait for LVGL's read timer; make it ready when a sample arrives.
   Recommendation: make it ready; measured to read with no time passed.
   Cost: input is read in the same step as it arrives, also in a busy step.
4. Where does the focus group of a keypad live?
   Options: G1 the display's default group; G2 a declared `lv::focus`; G3 one per keypad.
   Recommendation: G1 now, G2 when two orders are needed.
   Cost: one group on LVGL's heap per display that has a keypad.
5. How does a window get its mouse?
   Options: W1 wrap LVGL's SDL driver; W2 our own adapter feeding `lv::pointer`.
   Recommendation: W1 first, behind `LVGL26_SDL`.
   Cost: the window's display is LVGL's, not an `lv::display`, until W2.
6. What happens when the queue is full?
   Options: drop the oldest; merge moves and refuse the rest; a contract violation.
   Recommendation: merge moves, keep presses and releases, and return false when there is still no room.
   Cost: a bool that a platform may ignore.
7. The names: `lv::pointer` or `lv::mouse` and `lv::touch`; `lv::point`; `lv::pressed` and `lv::released`.
   Recommendation: `lv::pointer` for both mouse and touch, as LVGL's kind is one; the rest as written above.

## What was compiled, and what was reasoned

| Claim | Compiled and run | Reasoned |
|---|---|---|
| A level read misses a short press; a queue keeps it, also two clicks and a drag | Yes, both versions | |
| The read timer made ready reads at the next step | Yes, both versions | |
| The heap of a device | Yes, both versions | |
| Keypad and encoder through the same queue | | Yes: `continue_reading` is checked for every kind |
| A queue fed from an interrupt | | Yes: one producer and one consumer, not built |
| The focus group of the display | | Yes |
| Wrapping LVGL's SDL driver | | Yes: two calls of the window examples today |

## Not confirmed

- The byte counts on a 32-bit target.
- Whether a merged move loses a gesture that LVGL would have seen; the sketch did not test gestures.
- The browser's high-DPI scaling and multi-touch, which the spike did not try either.

## What the owner decided

Decided 2026-11-19, over the list above.
The number is the decision it answers.

| # | Decision |
|---|---|
| 1 | A queue the platform pushes into, plus a polled form: `lv::pointer mouse{display}` with `press`, `move`, `release` and `feed`, and `lv::pointer panel{display, &read_panel}`. |
| 2 | One class per kind over a shared detail queue (`detail::input_queue`). |
| 3 | A sample makes the read timer ready, so LVGL reads it at its next step. |
| 4 | A later keypad uses the display's default group. |
| 5 | Our own SDL adapter feeds `lv::pointer`; LVGL's SDL driver is not wrapped. The window, a device and a browser page give one application the same `lv::display` and `lv::pointer`. |
| 6 | A full queue merges a move into its newest sample, keeps every press and release, and `feed` returns false for a sample it cannot keep. |
| 7 | The names `lv::pointer`, `lv::point`, `lv::pressed` and `lv::released`. |

What the first pull request built, and what it measured on the host, `measured` 2026-11-19:

- `lv::pointer` takes 496 bytes of LVGL's heap in `tests/pointer.cpp`, and gives them back when it ends; the sketch above measured 488 with another display.
- A press and a release fed before a step are one click, read in that step with no time passed; two are two; a press, two moves and a release drag a slider to its end.
- A queue of 2 merges a third move, refuses a release, and the merged move still reaches the slider.
- The polled form, with a function and with a lambda without captures, makes a click from two reads.
- `examples/platform/sdl_window.hpp` shows the frame in an SDL2 window and feeds the pointer; the window example and both window ports link and run under SDL's dummy video driver on macOS.
- The queue is fed from the code that steps the application. Feeding it from an interrupt is not supported in this slice.

### The window joins the application's loop

Decided 2026-11-19, after the owner read the window example of pull request 132.

| # | Decision |
|---|---|
| 8 | `main` ends with `return app.run();` on every platform. A platform object joins the application's loop and is polled before each step; `step()` stays for a host that owns the loop, such as the browser page. |
| 9 | A close request of the window ends `run()` by default; the program can take it and decide to quit, to ignore it, or to hide the window. |
| 10 | `window.hide()` and `window.show()` keep the display, its trees and their state. `lv::display` carries a `visible` state that the platform writes, with `display.show()` and `display.hide()` forwarded to a panel that has them, and `lv::display_of(view)`. |
| 11 | A task that belongs to a screen is spawned with the tree's root as its owner, so unmounting ends it and mounting again starts exactly one. |

How it was built, and what was chosen where the decision left a choice, `measured` 2026-11-19 on the host with both LVGL versions where it says so:

- A source joins with a member `lv::in_loop<Source>`: a node of an intrusive list in the application, filled in the source's constructor and unlinked in its destructor. Nothing is allocated; `tests/app_loop.cpp` checks the heap.
- `step()` polls every source in the order they joined, then runs LVGL. So a host that steps needs no second call, and the browser page did not change.
- The wait between two steps is the first source's that can wait, else the application's `idle` function, else none. One source owns the wait because only one can block on its events; a second window would have its events read at the next step, at most 10 ms later with the SDL adapter. Several windows that each block are not supported by this rule; that would need a wait on several event sources at once.
- A source may leave while the loop polls another one: the walk moves past it. A source that outlives its application leaves the loop when the application ends, and later ends without touching it. A temporary application is a compile error.
- A hidden display draws what was marked before (`lv_refr_now`), then turns marking off (`lv_display_enable_invalidation`). Marking off alone was not enough: a layout change wakes the refresh, which then drew areas marked before the hide. While hidden the frame does not change; on show the whole screen is marked once. Of the visibility note's options this is "invalidation off", with one draw at the moment of hiding.
- The visible state is observable without an allocation: a `detail::watch_list` in the display, told on each change, for the later pause of `lv::run` tasks.

