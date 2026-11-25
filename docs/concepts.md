# Concepts for parts

A part is a function that returns a piece of tree: a spin box, a card, a heading. Its parameters are checked at compile time by the concepts of namespace `lv`. A wrong argument is refused at the call, with the name of the concept in the message, and not deep inside the part.

## The concepts

| Concept | What satisfies it | Where you write it |
|---|---|---|
| `lv::tree_handler` | A function, or a lambda that captures nothing: what a constexpr tree can hold | `lv::tree_handler auto clicked`, in a part that builds a constexpr tree |
| `lv::event_handler` | Anything callable with an `lv::event` or with nothing, also a lambda that captures one pointer | `lv::event_handler auto clicked`, in code that runs: `view.on<>()` |
| `lv::dependency` | An `lv::state` or an `lv::computed`: what a widget can follow | `const lv::dependency auto& count`, or the template argument `template <lv::dependency auto& Count>` |
| `lv::state_value` | What a state can hold: a bool, an enum, a quantity, a named value, a colour, a pointer | `template <lv::state_value T>` |
| `lv::tree_node` | A node of a tree: what `lv::button({...})[...]` makes, or a part returns | `lv::tree_node auto content` |
| `lv::view_value` | A view of a widget: `lv::obj`, `lv::view<"label">` | `lv::view_value auto widget` |
| `lv::style_value` | The type of `.style`: what `lv::style({...})` makes | `lv::style_value auto extra` |
| `lv::font_value` | What `.text_font` takes: a font from `lv::typeface`, `lv::raw(font)`, or `lv::bind(scaled)` | `const lv::font_value auto& font` |
| `lv::token_value` | A design token of any kind: `tok.color.surface`, `tok.space.md` | `lv::token_value auto value` |
| `lv::token_of<T>` | A design token that holds a `T` | `lv::token_of<lv_color_t> auto fill` |
| `lv::repeat_part` | A generic lambda that captures nothing: what `lv::for_each` calls in the compiler | `lv::for_each<E>(part)`; see [Repetition](repetition.md) |

No concept has an article. A concept that names a kind of value ends in `_value`, as `state_value` does: `view_value`, `style_value`, `font_value`, `token_value`. `lv::token_of<T>` is a token that holds a `T`. The others name what a value is used for.

A group of tokens, such as a pair of colours, is a struct of tokens. It is not a token, so `lv::token_value` refuses it. Take the group by a template parameter, or take its two tokens as two parameters.

`tests/concepts.cpp` states, with `static_assert`, what satisfies each concept and what does not.

## A part

A part takes its state, its handlers and its fonts as parameters, and builds the tree from them.

<!-- snippet: examples/parts/main.cpp parts -->
```cpp
// A heading in a font that a style accepts: a typeface's font, lv::raw(font) or lv::bind(font).
constexpr auto heading(const char* name, const lv::font_value auto& font) {
  return lv::label({
    .text  = name,
    .style = {.text_font = font},
  });
}

// A number with a button on each side. The state is any state or computed; the handlers are
// functions or lambdas that capture nothing.
constexpr auto spin_box(
  const char* name, const lv::dependency auto& count, lv::tree_handler auto fewer,
  lv::tree_handler auto more
) {
  return lv::row(
    {.flex_align = {"start", "center", "center"}, .size = {300 * px, lv::size_content}},
    lv::label({.flex_grow = 1, .text = name}),
    lv::button({.on_clicked = fewer}, lv::label({.text = "-"})),
    lv::label({.text = lv::bind(count, "{}")}),
    lv::button({.on_clicked = more}, lv::label({.text = "+"}))
  );
}

// The same with the state as a template argument: the lambdas name it, and the part needs
// no handler parameters.
template <auto& Count>
  requires lv::dependency<std::remove_cvref_t<decltype(Count)>>
constexpr auto counting_spin_box(const char* name) {
  return spin_box(
    name,
    Count,
    [] { Count.update([](lv::counter c) { return --c; }); },
    [] { Count.update([](lv::counter c) { return ++c; }); }
  );
}

// A box around any piece of tree.
constexpr auto card(lv::tree_node auto content) {
  return lv::box({.flex_flow = "column", .size = {320 * px, lv::size_content}}, content);
}
```

The state and the two functions that change it are ordinary globals.

<!-- snippet: examples/parts/main.cpp state -->
```cpp
constinit lv::state copies{lv::counter{1}};
constinit lv::state pages{lv::counter{1}};

void fewer_copies() {
  copies.update([](lv::counter c) { return --c; });
}
void more_copies() {
  copies.update([](lv::counter c) { return ++c; });
}
```

The tree calls the parts. The first spin box gets its state and its functions as arguments. The second names its state as a template argument and carries its own lambdas.

<!-- snippet: examples/parts/main.cpp tree -->
```cpp
constexpr auto ui = card(
  lv::column(
    {}, heading("Print", title_font), spin_box("Copies", copies, &fewer_copies, &more_copies),
    counting_spin_box<pages>("Pages")
  )
);
```

## A lambda as a handler

A handler in a constexpr tree is a function pointer, so a function and a lambda that captures nothing both work, in a tree and through `lv::tree_handler auto`. A part that builds a constexpr tree takes `lv::tree_handler`, and a lambda with a capture is refused at the part's own call. `lv::event_handler` is for code that runs, such as `view.on<>()`, where a lambda may capture one pointer. A lambda that takes an `lv::event` gets the event. `tests/handlers.cpp` presses the buttons of such parts and checks the states.

A lambda cannot capture a state, because a capture is a value that the tree would have to keep. Name the state as a template argument instead, as `counting_spin_box` does: the lambda names the global, and captures nothing.

A lambda with a capture that is passed to a part taking `lv::tree_handler` is refused at the call, and the compiler names the failed expression, `without_captures__name_the_state_as_a_template_argument`. Written straight into a tree, or through a part that takes `lv::event_handler`, it is refused where the tree takes it:

```text
error: lv: wrong value for this property. It takes a function or a lambda that captures nothing:
`&function`, `[] { ... }` or `[](lv::event e) { ... }`. A handler in a constexpr tree is a plain
function pointer, so it cannot hold state: name the state as a template argument ...
```

To give a handler a capture, add it to a view that is already mounted: `view.on<LV_EVENT_CLICKED>(handler)` keeps a capture of one pointer.

## A wrong argument

A part that constrains its parameters refuses a wrong argument at the call. The message names the concept and the type that failed it:

```text
error: no matching function for call to 'spin_box(...)'
  required for the satisfaction of 'dependency<auto:N>' [with auto:N = int]
```

## Editors

Clang, which most editors use, does not yet read a constrained placeholder as a non-type template parameter (`template <lv::dependency auto& Count>`, Clang 21). GCC reads it. Write the same constraint as a requires clause, as the example does, and both read the part.
