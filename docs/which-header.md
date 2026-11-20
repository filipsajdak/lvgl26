# Which header

`<lv/lv.hpp>` is the core of lvgl26: the application, displays, trees and widgets, views, states, styles, design tokens, tasks, animations, options and units. A feature that would cost every file compile time has a header of its own. `<lv/all.hpp>` includes the core and all of them.

```cpp
#include <lv/lv.hpp>    // the core
#include <lv/list.hpp>  // and lv::list, because this file shows a list
```

A file that includes `<lv/all.hpp>` compiles about 4 G instructions slower than one that includes `<lv/lv.hpp>`, before it makes a widget. The examples include exactly what they use; the quick start and the code on these pages include `<lv/all.hpp>`.

## The headers and their names

The table is written from what each header declares, read by reflection (`docs/reference/headers.json`).

<!-- headers: begin -->
| Header | Names |
|---|---|
| `<lv/encoded_image.hpp>` | `lv::encoded_image` |
| `<lv/font.hpp>` | `lv::face_file`, `lv::font`, `lv::typeface` |
| `<lv/image.hpp>` | `lv::image_file` |
| `<lv/list.hpp>` | `lv::each`, `lv::list`, `lv::list_error`, `lv::row_view` |
| `<lv/look.hpp>` | `lv::design`, `lv::design_of`, `lv::look`, `lv::look_of` |
| `<lv/one_of.hpp>` | `lv::held`, `lv::not_mounted`, `lv::one_of` |
| `<lv/pointer.hpp>` | `lv::contact`, `lv::most_pointer_samples`, `lv::pointer`, `lv::pointer_feed`, `lv::pointer_feeder`, `lv::pointer_sample`, `lv::pressed`, `lv::released` |
| `<lv/repeat.hpp>` | `lv::caption`, `lv::field`, `lv::for_each`, `lv::for_each_field`, `lv::like`, `lv::name_of`, `lv::repeat_part` |
| `<lv/runtime.hpp>` | `lv::load_error`, `lv::load_failure`, `lv::runtime` |
| `<lv/scaled_font.hpp>` | `lv::scaled_font` |
| `<lv/theme.hpp>` | `lv::theme` |
| `<lv/list.hpp>` | `lv::bind(&row::field)`: a field of a row |
<!-- headers: end -->

These are not in `<lv/all.hpp>`, because they are a platform, a design system or a tool, and each is included by itself:

| Header | What it is |
|---|---|
| `<lv/signal.hpp>`, `<lv/signal_type.hpp>`, `<lv/signal_lamps.hpp>` | Signal, the design system of the examples: its themes, its fonts, its lamp colours |
| `<lv/host.hpp>` | The operating system's clock |
| `<lv/ui_scheduler.hpp>` | Work posted from other threads |
| `<lv/mcp.hpp>` | The agent kit's server |
| `<lv/png.hpp>` | A screen written as a PNG file |

## When a header is missing

The compiler says that the name does not exist. It does not say which header declares it. These are the messages, and what to add:

| The message | Add |
|---|---|
| GCC: `'list' is not a member of 'lv'` | the header of `lv::list` in the table above, here `<lv/list.hpp>` |
| GCC: `'look' is not a member of 'lv'` | `<lv/look.hpp>` |
| GCC: `no matching function for call to 'bind(int track::*, ...)'` | `<lv/list.hpp>`: a field of a row |
| Clang, in the editor: `no template named 'list' in namespace 'lv'` | the header of the name, from the table |
| Clang, in the editor: `no member named 'theme' in namespace 'lv'` | the header of the name, from the table |

Any of these messages for a name `lv::X` means: find `lv::X` in the table and include its header.

`tools/check_includes.py` finds every such name in a list of files and says which header it needs:

```sh
python3 tools/check_includes.py src/main.cpp src/parts.hpp
# src/main.cpp:12: lv::list needs #include <lv/list.hpp>
```

It reads the headers of the program that the files include with quotes, and skips comments and strings. It uses only the Python standard library.
