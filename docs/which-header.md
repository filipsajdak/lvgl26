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
| `<lv/look.hpp>` | `lv::design`, `lv::design_of`, `lv::draw`, `lv::drawn_font`, `lv::font_object`, `lv::font_of`, `lv::look`, `lv::look_of` |
| `<lv/one_of.hpp>` | `lv::held`, `lv::not_mounted`, `lv::one_of` |
| `<lv/pointer.hpp>` | `lv::contact`, `lv::most_pointer_samples`, `lv::pointer`, `lv::pointer_feed`, `lv::pointer_feeder`, `lv::pointer_sample`, `lv::pressed`, `lv::released` |
| `<lv/repeat.hpp>` | `lv::caption`, `lv::field`, `lv::for_each`, `lv::for_each_field`, `lv::like`, `lv::name_of`, `lv::repeat_part` |
| `<lv/runtime.hpp>` | `lv::load_error`, `lv::load_failure`, `lv::runtime` |
| `<lv/scaled_font.hpp>` | `lv::scaled_font` |
| `<lv/shown.hpp>` | `lv::page`, `lv::page_of`, `lv::shown`, `lv::transition`, `lv::when` |
| `<lv/theme.hpp>` | `lv::theme` |
| `<lv/list.hpp>` | `lv::bind(&row::field)`: a field of a row |
<!-- headers: end -->

These are not in `<lv/all.hpp>`, because they are a platform, a design system or a tool, and each is included by itself:

| Header | What it is |
|---|---|
| `<lv/signal.hpp>`, `<lv/signal_type.hpp>`, `<lv/signal_fonts.hpp>`, `<lv/signal_lamps.hpp>`, `<lv/signal_motion.hpp>` | Signal, the design system of the examples: its themes, its fonts (drawn in each file, or linked once from `lvgl26::signal_type`), its lamp colours, its durations and easings |
| `<lv/host.hpp>` | The operating system's clock |
| `<lv/ui_scheduler.hpp>` | Work posted from other threads |
| `<lv/mcp.hpp>` | The agent kit's server |
| `<lv/png.hpp>` | A screen written as a PNG file |

## When a header is missing

For a template or a function of an opt-in header, the compiler names the header itself:

```text
error: static assertion failed: lv::list is in <lv/list.hpp>: add #include <lv/list.hpp>
```

`lv::list`, `lv::each`, `lv::for_each`, `lv::for_each_field`, `lv::look`, `lv::image_file`, `lv::encoded_image`, `lv::runtime`, `lv::one_of` and `lv::pointer` say so on the error line. For `lv::theme({...})` GCC says `no matching function for call to 'theme(<brace-enclosed initializer list>)'`, and the note below it quotes the line `lv::theme is in <lv/theme.hpp>`.

A class that is no template (`lv::scaled_font`, `lv::typeface`, `lv::pointer_feed`) cannot be declared without its header, so for it the compiler says only that the name does not exist. These are the messages, and what to add:

| The message | Add |
|---|---|
| GCC: `'scaled_font' is not a member of 'lv'` | the header of `lv::scaled_font` in the table above, here `<lv/scaled_font.hpp>` |
| GCC: `'typeface' is not a member of 'lv'` | `<lv/font.hpp>` |
| GCC: `no matching function for call to 'bind(int track::*, ...)'` | `<lv/list.hpp>`: a field of a row |
| Clang, in the editor: `no template named 'list' in namespace 'lv'` | the header of the name, from the table |
| Clang, in the editor: `no member named 'theme' in namespace 'lv'` | the header of the name, from the table |

Any of these messages for a name `lv::X` means: find `lv::X` in the table and include its header.

## The build says which header

A program that takes lvgl26 through CMake (`FetchContent`, `add_subdirectory`) gets a check of its includes with no line of its own. Before a target links, every file of it that names something of lvgl26 is read, and a missing header stops the build with the line:

```text
src/parts.hpp:4: lv::list needs #include <lv/list.hpp>
```

It checks the headers of the program that a file includes with quotes too, skips comments and strings, and reads a file again only when it, or a header it includes, changes. `-DLVGL26_CHECK_INCLUDES=OFF` turns it off; `lvgl26_check_includes(target)` checks one target when it is off. It needs Python 3; without it CMake says so and the build goes on.

## The editor says which header

With [editor support](editor-support.md), the editor marks the line with the same text as the compiler: `lv::list is in <lv/list.hpp>: add #include <lv/list.hpp>`. clangd reads it from the stubs, so it shows at once.

For a class that is no template (`lv::scaled_font`, `lv::typeface`, `lv::pointer_feed`), clangd offers a quick fix at the name instead: `Include <lv/scaled_font.hpp> for symbol lv::scaled_font`. It finds the header in its index of lvgl26, which it builds in the background the first time it opens the project.

## By hand

`tools/check_includes.py` finds every such name in a list of files and says which header it needs:

```sh
python3 tools/check_includes.py src/main.cpp src/parts.hpp
# src/main.cpp:12: lv::list needs #include <lv/list.hpp>
```

It reads the headers of the program that the files include with quotes, and skips comments and strings. It uses only the Python standard library.

## For an agent

The plugin of this repository has a skill, [skills/write-ui](../skills/write-ui/SKILL.md), that sends an agent to this page when a name is missing. An agent without the plugin reads this page in `llms.txt`: the message `main.cpp:12: lv::list needs #include <lv/list.hpp>` is to be followed as it stands.
