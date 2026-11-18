# Notices

lvgl26 is under the MIT licence (see [LICENSE](LICENSE)). The parts below come from other works and keep their own terms.

| Part | From | Licence |
|---|---|---|
| `include/lv/detail/coverage.hpp` | The anti-aliasing method of [font-rs](https://github.com/raphlinus/font-rs), Copyright 2015 Google Inc. | [Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0) |
| `site/media/fonts/*.ttf` | [Barlow](https://github.com/jpt/barlow), Copyright 2017 The Barlow Project Authors | SIL Open Font License 1.1 (`site/media/fonts/OFL.txt`) |
| `include/lv/signal/fonts/*.ttf` | Copies of the three files above, for `lv/signal_type.hpp` to embed | SIL Open Font License 1.1 (`include/lv/signal/fonts/OFL.txt`) |
| `tools/dtcg/2025.10/*.json` | The JSON Schemas of the [Design Tokens Community Group](https://github.com/design-tokens/community-group), unchanged | W3C Software and Document License, and W3C 3-clause BSD License (`tools/dtcg/LICENSE.md`) |
| `design/signal` | See [design/signal/NOTICE.md](design/signal/NOTICE.md) | |

A program that uses `lv::font` or `lv::typeface` is built with `coverage.hpp`. That code runs in the compiler only: none of it reaches the program. The font file you embed has its own licence, which says whether its glyphs may be put into a device.

A program that includes `lv/signal_type.hpp` holds glyphs drawn from Barlow. The font files themselves do not reach the program. The licence of the glyphs is in `include/lv/signal/fonts/OFL.txt`: read it for what it asks of a product.
