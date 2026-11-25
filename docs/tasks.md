# Tasks

A task is a coroutine on LVGL's loop: UI steps written top to bottom, which wait for events, time and animations.

Header: `<lv/lv.hpp>`, the core.

## A task

The ball of `examples/bounce` falls, bounces lower each time, rests, and is picked up again. One task says so in order:

<!-- snippet: examples/bounce/scene.hpp bounce -->
```cpp
inline lv::task<> bounce(lv::view<"obj"> box_view) {
  for (bool to_right = true;; to_right = !to_right) {
    lv::spawn(slide(box_view, to_right ? left : right, to_right ? right : left), box_view);
    co_await fall(box_view, drop_height);
    for (int height = next_height(drop_height); height > last_height;
         height     = next_height(height)) {
      co_await rise(box_view, height);
      co_await fall(box_view, height);
    }
    co_await lv::sleep(rest);
    co_await rise(box_view, drop_height);  // picked up again
    co_await lv::sleep(held);
  }
}
```

`fall` and `rise` are tasks of their own, each one [animation](animation.md). `tests/example_bounce.cpp` steps the clock and pins the motion.

## Wait for an event

```cpp
lv::task<> count_clicks(lv::view<"button"> button) {
  for (;;) {  // ends by itself when the button is deleted
    co_await button.next().clicked;
    co_await lv::sleep(100 * ms);
    ++clicks;
  }
}
```

`view.next()` names every LVGL event of a widget: `clicked`, `value_changed` and the rest. The names are generated from LVGL's list of events.

## Wait for the first of several

```cpp
lv::task<bool> confirm(lv::view<"button"> ok, lv::view<"button"> cancel) {
  co_return co_await lv::first(
    ok.next().clicked.then([] { return true; }),
    cancel.next().clicked.then([] { return false; }),
    lv::sleep(5 * sec).then([] { return false; })
  );
}
```

`lv::first` waits for whichever comes first and cancels the rest. It returns what the winner's handler returns.

## Start a task, and end it

```cpp
lv::spawn(count_clicks(button));
lv::spawn(blink(lamp), panel);  // belongs to the panel: it ends when the panel is deleted
```

- A task that belongs to a part of the screen is spawned with that part as its owner. It ends when the owner is deleted.
- A task without an owner lives until it returns, or until a widget it has waited on is deleted.
- **A deleted widget cancels the task.** Once a task has waited on a widget, it is tied to it. Deleting the widget destroys the task and runs the destructors of its local objects.
- To react to the deletion instead, wait for `view.next().gone`.
- Everything runs on the UI thread. The frames of the coroutines come from LVGL's allocator.

## Work from another thread

Another thread never calls LVGL. It posts work to the UI thread through `lv::ui_scheduler`, in its own header `<lv/ui_scheduler.hpp>`, on a hosted system only:

```cpp
lv::ui_scheduler ui;  // on the UI thread

ui.post([reading] { temperature = reading; });  // from any thread
co_await ui.schedule();                         // in a task: go on in a later turn of the loop
```

`post()` only locks a mutex and queues the work. The UI thread runs the queue from an LVGL timer, in the order of posting.

It needs no LVGL build option: `LV_USE_OS` may stay off. A program whose [settings](settings.md) say `"allow": {"threads": false}` cannot use it: the build stops and names the key.
