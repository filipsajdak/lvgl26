# Views and ids

A view is a typed handle to one widget: it reads and changes the widget, and never deletes it.

Header: `<lv/lv.hpp>`, the core.

## A widget of a tree, by its id

A node with `.id = "message"` is reached through the mounted tree. The compiler finds the id, and a wrong one lists the ids of the tree.

<!-- snippet: examples/mounting/main.cpp local -->
```cpp
  auto shown = lv::mount<toast>(display.screen());  // built in this variable; it never moves
  if (!shown.has_value()) return shown.error() == lv::mount_error::out_of_memory ? 2 : 3;
  shown->get<"message">().set({.text = "Saved at 12:00"});
```

`get<"message">()` returns an `lv::view<"label">`, the view of the widget's kind. `set({...})` changes its properties, written as in a tree.

## Views never delete

`lv::view<"arc">` is one pointer. Copying it copies the pointer, and dropping it leaves the widget alone.

Only `lv::mount` and `lv::owned<T>` own widgets. `lv::weak<T>` empties itself when LVGL deletes the widget.

## The untyped way

`view.set<"name">(...)` and `view.get<"name">()` reach any `lv_*_set_*` and `lv_*_get_*` function of the widget by its name. A name the widget does not have stops the build and lists the names it has.

One thing is typed there. A getter of an angle returns an `lv::angle` or an `lv::decidegrees`, so `image.get<"rotation">() == 90 * deg` holds whatever LVGL's number is ([Values and units](values.md)).

`raw()` gives the widget's C pointer: the one door to LVGL's C API.
