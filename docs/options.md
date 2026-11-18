# Option lists

A dropdown and a roller show a list of options. LVGL takes the list as one text with a line break between the options. lvgl26 takes the options as a list, and the compiler joins them.

## A list of texts

Write each option as an item.

<!-- snippet: examples/lvgl/widgets/dropdown_event.cpp list -->
```cpp
    lv::dropdown({.on_value_changed = &show_choice, .options = {"Berlin", "Paris", "Tokyo", "Sydney"}}),
```

The compiler makes the one text LVGL takes and keeps it in read-only memory. A dropdown keeps the pointer and copies nothing, so the list costs no heap. A roller copies the list into its label, because LVGL has no setter that keeps it.

An item must not be empty and must not hold a line break. Both are compile errors that name the item. A single text, `.options = "Cut\nCopy\nPaste"`, works as before. LVGL copies it, except that a dropdown of a constexpr tree keeps the pointer, as it does for a list.

Which properties take a list is found by the name of the setter: one that ends in `_set_options` and takes a text called `options`. LVGL says in its doc comments that these take options separated by line breaks, and a test reads the comments and fails when the names and the comments disagree.

## A list and a mode

A roller takes the list and its mode. Put the list in braces of its own.

<!-- snippet: examples/lvgl/widgets/roller_options.cpp roller -->
```cpp
constexpr auto normal = lv::roller({.options = {{"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"}, "normal"},
                                    .selected = {0, false}, .visible_row_count = 3, .width = 120 * px});

constexpr auto infinite = lv::roller({.options = {{"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"}, "infinite"},
                                      .selected = {2, false}, .visible_row_count = 3, .width = 120 * px});
```

`{"Mon\nTue", "normal"}` still works: a text, then the mode. `{"Mon", "Tue"}` is a text and a mode too, so it is refused with the message that "Tue" is no mode.

## The options of an enum

`lv::options_of<E>` makes one option for each enumerator of `E`, in the order they are declared. The text of an option is the name of its enumerator.

<!-- snippet: examples/lvgl/widgets/dropdown_bind_value.cpp enum -->
```cpp
enum class fruit { Apple, Banana, Cherry, Date, Elderberry };
constinit lv::state<fruit> chosen{fruit::Apple};
```

A state of the enum follows the choice with `.selected = lv::bind(state)`, in both directions: the user's choice sets the enumerator, and a change of the state shows in the widget.

<!-- snippet: examples/lvgl/widgets/dropdown_bind_value.cpp bound -->
```cpp
    lv::dropdown({.options = lv::options_of<fruit>, .selected = lv::bind(chosen), .width = 160 * px}),
```

The compiler maps the place of an option to its enumerator. The values of the enumerators need not be 0, 1, 2. A state of another enum, a plain list and a widget with no list are compile errors, each with a message that says what to write.

A state that holds an index (a struct with one integer member) works with every list, as before.

## Leaving enumerators out

`lv::but` names the enumerators that are left out. The places count the options that are there, and the mapping respects it. If the state holds an enumerator that is left out, the widget keeps what it shows.

<!-- snippet: examples/options/main.cpp but -->
```cpp
// The sheets of this printer's tray: A5 is left out, so the list has three options.
constexpr auto tray = lv::options_of<sheet, lv::but<sheet::a5>>;
```

`lv::but` is the one that `lv::for_each` takes: both leave out the same enumerators.

## Other texts

The name of an enumerator is rarely the text to show. Specialize `lv::option_text` for the enum to give the texts. The function runs in the compiler.

<!-- snippet: examples/options/main.cpp texts -->
```cpp
// The texts of an enum's options, given instead of the names of its enumerators.
template <>
struct lv::option_text<sheet> {
  static consteval const char* of(sheet s) {
    switch (s) {
      case sheet::a4: return "A4";
      case sheet::letter: return "Letter";
      case sheet::legal: return "Legal";
      case sheet::a5: return "A5";
    }
    return "";
  }
};
```

A table of translated texts is meant to plug in here.

## Limits

- A state of an enum follows the choice in a constexpr tree. Through `view.set({...})` at run time it is refused by a contract; give the state an index there.
- The choice of a roller moves without animation when the state changes.
