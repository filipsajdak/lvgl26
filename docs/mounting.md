# Where a tree lives

`lv::mount` builds the widgets of a tree and returns an `lv::mounted` that owns them. The object also holds what the tree's nodes keep while the widgets exist. The compiler knows its size, so lvgl26 allocates nothing for it: the object lives where the program puts it.

An `lv::mounted` is never copied or moved. LVGL's handler on the root widget points at it, and so may what the nodes keep. It is built in the variable that receives it, and it carries its own failure.

## In a variable

<!-- snippet: examples/mounting/main.cpp local -->
```cpp
  auto shown = lv::mount<toast>(display.screen());  // built in this variable; it never moves
  if (!shown) return shown.error() == lv::mount_error::out_of_memory ? 2 : 3;
  shown->get<"message">().set({.text = "Saved at 12:00"});
```

`if (!shown)` tells whether LVGL had memory for the widgets, and `shown.error()` says why it had not. `shown->get<"id">()` reaches a widget by its id, as with `std::expected`. When the variable goes, the widgets go with it.

A function may return `lv::mount<ui>(parent)`, and a struct may hold one as a member initialised from it: the result is built in place. A named `lv::mounted` cannot be returned, and a `std::vector` cannot hold one. For a screen that is replaced later, keep a `std::optional<lv::mounted<ui>>`: `screen.emplace(parent)` builds the widgets inside it, and `screen.reset()` deletes them.

## In a global

A global made with `constinit` holds no widgets until `mount(parent)` runs. Before that, `!home_page` is true and `home_page.error()` is `lv::mount_error::not_mounted`.

<!-- snippet: examples/mounting/main.cpp global -->
```cpp
constinit lv::mounted<home> home_page;  // no widgets yet: mounted once the display exists
```

<!-- snippet: examples/mounting/main.cpp mount-global -->
```cpp
  if (auto mounted = home_page.mount(display.screen()); !mounted) return 1;
  home_page->get<"title">().set({.text = "Ready"});
```

`mount(parent)` returns a `std::expected<void, lv::mount_error>`. Call it once: a second call breaks its precondition, also after the parent deleted the widgets. When `lv::application` ends, LVGL deletes the widgets, and the global lets go of them.

## In memory set aside

`lv::storage_size<ui>` is the number of bytes an `lv::mounted<ui>` takes, and `alignof(lv::mounted<ui>)` its alignment. Both are constants, so a program can lay out its memory at start.

<!-- snippet: examples/mounting/main.cpp memory -->
```cpp
// Memory set aside at start, sized by the compiler.
alignas(lv::mounted<dialog>) constinit std::array<std::byte, lv::storage_size<dialog>> dialog_memory{};
```

`lv::mount_in` builds the `lv::mounted` in that memory. `std::destroy_at` ends it and deletes the widgets.

<!-- snippet: examples/mounting/main.cpp mount-in -->
```cpp
  auto& asked = lv::mount_in<dialog>(dialog_memory, display.screen());
  if (!asked) return 4;
  app.advance(50 * ms), app.step();
  std::destroy_at(&asked);  // deletes the dialog's widgets
```

## When the parent goes first

The parent may delete the widgets first: its own `lv::mounted`, or C code through `raw()`. The `lv::mounted` notices, through one handler on the root widget. It lets go of what the nodes keep, and `root()` is then empty. Deleting a single widget of a mounted tree through the C API is not supported: a widget of a tree goes with its tree.

## Giving the widgets away

`detach()` and `lv::place` leave the widgets to their parent, as the C API does. They work for a tree whose nodes keep nothing at run time. For a tree that keeps something, both are compile errors that say so: keep its `lv::mounted` as long as the widgets are shown.
