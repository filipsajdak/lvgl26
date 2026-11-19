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

## A window that shows one of several trees

`lv::one_of<saving, failed>` is one slot for one of several trees, as `std::variant` holds one of several types. Its size and alignment are those of the largest `lv::mounted` of its trees, and one tree is alive at a time.

<!-- snippet: examples/mounting/main.cpp slot -->
```cpp
constinit lv::one_of<saving, failed> status;  // a window that shows one of two dialogs
```

`status.mount<failed>(parent)` ends the tree that is there first, then builds `failed` in the same memory. lvgl26 allocates nothing for it, so switching many times ends at the heap it started from. A failed mount leaves the slot empty, with its error. A tree that is not one of the slot's is a compile error that lists them.

<!-- snippet: examples/mounting/main.cpp slot-use -->
```cpp
  if (!status.mount<saving>(display.screen())) return 6;
  app.advance(50 * ms), app.step();
  if (!status.mount<failed>(display.screen())) return 7;  // saving ends first, in the same memory
  status.visit([](lv::mounted<failed>& f) { f->get<"reason">().set({.text = "The card is full"}); },
               [](const auto&) {});                       // saving has nothing to update
```

The slot carries its failure as `lv::mounted` does: `if (!status)`, `status.has_value()`, and `status.error()`, which is `lv::mount_error::not_mounted` while it holds no tree. `status.holds<failed>()` tells which tree is mounted, and `status.get<failed>()` gives it; calling `get` for another tree breaks a precondition. `unmount()` deletes the widgets, and so does the parent or C code: the slot is empty afterwards.

`visit` takes any number of functions, and the parameter type of each is its pattern. `lv::mounted<failed>&` takes that tree, `auto&` every tree that no other arm names, and `lv::not_mounted` the empty slot. Without an arm for the empty slot, arms that return nothing are skipped there, and arms that return `R` give `std::optional<R>`; with it, the result is `R`. The result is the common type of what the arms return, as `std::visit` allows: an arm that returns `int` and one that returns `double` give `double`. A tree that no arm takes, or an arm that can never be called, is a compile error that lists the slot's trees.

A slot is for a root, as `unmount()` is: a window that shows one of several dialogs. Pages that take turns inside a tree are switched with `lv::shown`, which is planned and not built yet.

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
