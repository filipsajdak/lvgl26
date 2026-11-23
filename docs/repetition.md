# Repetition

`lv::for_each` writes the repeated widgets of a tree. The compiler calls a part once for each element and puts what it returns among the children of the widget that holds the `lv::for_each`. The tree is the one that would have been written by hand: no list at run time, no memory from lvgl26.

`lv::for_each` and `lv::for_each_field` are in their own header: `#include <lv/repeat.hpp>` (see [Which header](which-header.md)).

Use it for a set that is fixed when the program is written: the tiles of a home screen, the lines of a detail view, the tubes of a gauge. For rows that change while the program runs, use [lists](lists.md).

## One tile for each enumerator

The part is a generic lambda that takes the enumerator as a template argument. `lv::but` leaves enumerators out.

<!-- snippet: examples/repeat/main.cpp tiles -->
```cpp
enum class page { none, print, scan, copy, usb };
constinit lv::state<page> shown{page::none};

// The label of a tile is a member of `labels` with the name of the enumerator.
struct labels {
  static constexpr char print[] = "Print";
  static constexpr char scan[]  = "Scan";
  static constexpr char copy[]  = "Copy";
  static constexpr char usb[]   = "USB";
};

constexpr auto tile = []<page Page> {
  return lv::button(
    {.size = {80 * px, 60 * px},
     .on_clicked =
       [] {
         shown = Page;
       }},
    lv::label({.text = lv::like<Page, labels>})
  );
};

constexpr auto home =
  lv::row({.size = {100 * pct, lv::size_content}}, lv::for_each<page, lv::but<page::none>>(tile));
```

Each tile has its own handler: `shown = Page` is a different function for each enumerator, so nothing looks the enumerator up at run time. `lv::for_each` can stand among other children: `lv::column[header, lv::for_each<page>(tile), footer]`.

Two helpers derive something from the name of the enumerator:

- `lv::name_of<Page>` is the name as a constant text: `"print"`.
- `lv::like<Page, labels>` is the member of the struct or enum `labels` that has the same name. A name that `labels` lacks is a compile error that names it.

## One line for each captioned member

A member of a row type is repeated when it carries a caption. A caption is an annotation whose text is a key of the strings.

<!-- snippet: examples/repeat/main.cpp lines -->
```cpp
struct job_id {
  int value;
};

struct job {
  job_id id;
  const char* title;
  [[= lv::caption{"Owner"}]] const char* owner;
  [[= lv::caption{"Pages"}]] lv::counter pages;
  [[= lv::caption{"Size"}]] const char* size;
};

constexpr auto line = []<lv::field F> {
  return lv::row(lv::label({.text = F.caption}), lv::label({.text = F.text()}));
};

constinit lv::list<job, 4> queue;
constexpr auto details = lv::column(
  {.size = {100 * pct, lv::size_content}},
  lv::each(queue, &job::id, lv::column(lv::for_each_field<job>(line)))
);
```

An `lv::field` holds `F.caption` (the key), `F.name` (the member's name) and `F.member` (the pointer to the member, for `lv::bind(F.member)`). `F.text()` is the binding that shows the member as a text: `lv::bind(F.member)` for a text, a number through `"{}"` for anything else.

A member without a caption is not repeated, here `id` and `title`. A type with no captioned member is a compile error, so a forgotten annotation does not give an empty list. A caption is a string literal, because an annotation holds a structural type, which `std::string_view` is not.

`lv::for_each_field` works inside the row template of `lv::each`, as above, and outside it.

## One element for each record

A constant array of records works the same way. A record that is a structural type is a template argument. Any other record, such as one with a text literal, is taken by reference.

<!-- snippet: examples/repeat/main.cpp tubes -->
```cpp
struct tube {
  int percent;
};
constexpr tube levels[] = {{90}, {50}, {80}, {10}};

constexpr auto gauges =
  lv::row({.size = {100 * pct, lv::size_content}}, lv::for_each<levels>([]<tube T> {
            return lv::bar({
              .range = {0, 100},
              .size  = {20 * px, 80 * px},
              .value = {T.percent, false},
            });
          }));
```

For a record that holds a text, write `[]<const auto& T> { ... }`. A C array and a `std::array` both work.

## Rules

- A part returns a widget. A part that sets `.id` is a compile error: the repeated widgets are not reached by id, and one id repeated would be used twice.
- A part that does not accept the element, such as `[]<other_enum E>` for a `page`, is a compile error that says how to write it.
- `lv::but` names enumerators of the enum. Naming another is a compile error, and so is leaving no enumerator.
- A part is a captureless generic lambda, the concept `lv::repeat_part` of [Concepts for parts](concepts.md). A capture is refused at the call. What a part needs it names as a template argument or reads from a constant.
- The widget a part returns satisfies `lv::tree_node`, and a part that returns anything else is refused.
- Compile time grows with the number of repeated widgets as it does for widgets written by hand. `tests/large_tree.cpp` shows that a tree that does not repeat is no dearer.
