# lvgl26 documentation

lvgl26 is a C++26 framework over LVGL for embedded user interfaces. The compiler checks a screen before it runs, and your code calls no C.

| Page | What it gives you |
|---|---|
| [Quick start](quick-start.md) | A speedometer screen in ten minutes: a tree, a typed value, a picture |
| [Themes](themes.md) | One look for every widget: the built-in Signal theme with its typefaces, and a theme of your own from a token file |
| [Images](images.md) | A PNG file becomes pixels in read-only memory, decoded by the compiler into the display's own format |
| [Fonts](fonts.md) | A TrueType file becomes a font in read-only memory, made by the compiler and chosen by a typography token |

More pages are being written: the reference, guides, and pages for readers who know Qt, Slint or TouchGFX. Until then the [README](../README.md) tours the features.

## How these pages are kept right

- Every code block that shows a file comes from that file, and the project's tests build and run it.
- A page shows no call into LVGL's C API. lvgl26 has a C++ form for it, or the page says why not.

## For AI agents

- [llms.txt](https://filipsajdak.github.io/lvgl26/llms.txt) lists the pages, and `llms-full.txt` next to it holds them all in one file.
- Every page is also published as Markdown: replace `.html` with `.md` in its address.
- The repository is a Claude Code plugin. Its skills point to these pages.
