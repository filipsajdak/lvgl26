# Where a tree lives

`lv::mount` builds the widgets of a tree and returns an `lv::mounted` that owns them. The object also holds what the tree's nodes keep while the widgets exist. The compiler knows its size, so lvgl26 allocates nothing for it: the object lives where the program puts it.

An `lv::mounted` is never copied or moved. LVGL's handler on the root widget points at it, and so may what the nodes keep. It is built in the variable that receives it, and it carries its own failure.

## In a variable

<!-- snippet: examples/mounting/main.cpp local -->
```cpp
  auto shown = lv::mount<toast>(display.screen());  // built in this variable; it never moves
  if (!shown.has_value()) return shown.error() == lv::mount_error::out_of_memory ? 2 : 3;
  shown->get<"message">().set({.text = "Saved at 12:00"});
```

`if (!shown)`, or `shown.has_value()`, tells whether LVGL had memory for the widgets, and `shown.error()` says why it had not. `shown->get<"id">()` reaches a widget by its id, as with `std::expected`. When the variable goes, the widgets go with it.

`set({...})` changes properties of a widget at run time, written in place as in a tree. A style in it is a named style, a look, or a style made with `lv::style({...})`: `set` holds a style by reference so that its argument stays small on the stack, and a style written out inline (`.style = {.radius = ...}`) is a compile error that says so. A prepared set of properties is a function that calls `set`, not a variable: a variable given to `set` is a compile error.

A mount is also refused with `lv::mount_error::out_of_memory` when LVGL's heap has less room in one piece than the longest list of styles a widget can have: 1008 bytes on a 64-bit target, 504 on a 32-bit one. LVGL v9.6.0 and the v10 pin grow a widget's list of styles without checking the allocation (lvgl/lvgl#11014), so lvgl26 makes sure, before each style it adds, that the list could grow to its longest. A version of LVGL with the fix of lvgl/lvgl#11015 reports the failure itself; lvgl26 then uses LVGL's answer and checks nothing beforehand.

A function may return `lv::mount<ui>(parent)`, and a struct may hold one as a member initialised from it: the result is built in place. A named `lv::mounted` cannot be returned, and a `std::vector` cannot hold one.

## In a global

A global made with `constinit` holds no widgets until `mount(parent)` runs. Before that, `home_page.has_value()` is false and `home_page.error()` is `lv::mount_error::not_mounted`. Its memory is the global itself, sized by the compiler: one mention of the tree, and nothing on the heap.

<!-- snippet: examples/mounting/main.cpp global -->
```cpp
constinit lv::mounted<home> home_page;  // no widgets yet: mounted once the display exists
constinit lv::mounted<dialog> asked;    // a window that opens and closes
```

<!-- snippet: examples/mounting/main.cpp mount-global -->
```cpp
  if (!home_page.mount(display.screen())) return 1;
  home_page->get<"title">().set({.text = "Ready"});
```

`mount(parent)` returns a `std::expected<void, lv::mount_error>`. When `lv::application` ends, LVGL deletes the widgets, and the global lets go of them.

## A window that opens and closes

`unmount()` deletes the widgets and ends what the nodes keep. The global is then not mounted, as before its first `mount(parent)`, and can be mounted again in the same memory. lvgl26 allocates nothing for that: after a second mount, LVGL's heap is what it was after the first.

<!-- snippet: examples/mounting/main.cpp window -->
```cpp
  if (!asked.mount(display.screen())) return 4;  // the window opens
  app.advance(50 * ms), app.step();
  asked.unmount();                               // it closes: its widgets go
  if (!asked.mount(display.screen())) return 5;  // and opens again, in the same memory
```

`mount(parent)` works whenever the global is not mounted: never mounted, after `unmount()`, after a failed mount, and after the parent deleted the widgets. Mounting a global that is mounted breaks a precondition, and so does `unmount()` of one that is not.

`unmount()` is for the life of a root: a window that closes, a screen that is torn down. Pages that take turns inside a tree are switched with `lv::shown`, which is planned and not built yet. It keeps one page alive at a time in memory of the tree's own.

## In memory that is not a variable

`lv::mount_in` builds an `lv::mounted` in memory that is not a variable of its own, such as a slice of one region reserved at start. `lv::storage_size<ui>` and `alignof(lv::mounted<ui>)` say how much it needs, and `std::destroy_at` ends it.

```cpp
std::span<std::byte> slice = region.subspan(offset, lv::storage_size<dialog>);  // reserved at start
auto& in_region = lv::mount_in<dialog>(slice, display.screen());
```

## When the parent goes first

The parent may delete the widgets first: its own `lv::mounted`, or C code through `raw()`. The `lv::mounted` notices, through one handler on the root widget. It lets go of what the nodes keep and is not mounted any more: `!root`, and `root()` is empty. Deleting a single widget of a mounted tree through the C API is not supported: a widget of a tree goes with its tree.

## Giving the widgets away

`detach()` and `lv::place` leave the widgets to their parent, as the C API does. They work for a tree whose nodes keep nothing at run time. For a tree that keeps something, both are compile errors that say so: keep its `lv::mounted` as long as the widgets are shown.
