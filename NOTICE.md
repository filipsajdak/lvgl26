# Notices

lvgl26 is under the MIT licence (see [LICENSE](LICENSE)). The parts below come from other works and keep their own terms.

| Part | From | Licence |
|---|---|---|
| `include/lv/detail/coverage.hpp` | The anti-aliasing method of [font-rs](https://github.com/raphlinus/font-rs), Copyright 2015 Google Inc. | [Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0) |
| `site/media/fonts/*.ttf` | [Barlow](https://github.com/jpt/barlow), Copyright 2017 The Barlow Project Authors | SIL Open Font License 1.1 (`site/media/fonts/OFL.txt`) |
| `design/signal` | See [design/signal/NOTICE.md](design/signal/NOTICE.md) | |

A program that uses `lv::font` or `lv::typeface` is built with `coverage.hpp`. That code runs in the compiler only: none of it reaches the program. The font file you embed has its own licence, which says whether its glyphs may be put into a device.
