# Notices

lvgl26 is under the MIT licence (see [LICENSE](LICENSE)). The parts below come from other works and keep their own terms.

| Part | From | Licence |
|---|---|---|
| `include/lv/detail/coverage.hpp` | The anti-aliasing method of [font-rs](https://github.com/raphlinus/font-rs), Copyright 2015 Google Inc. | [Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0) |
| `site/media/fonts/*.ttf` | [Barlow](https://github.com/jpt/barlow), Copyright 2017 The Barlow Project Authors | SIL Open Font License 1.1 (`site/media/fonts/OFL.txt`) |
| `include/lv/signal/fonts/*.ttf` | Copies of the three files above, for `lv/signal_type.hpp` to embed | SIL Open Font License 1.1 (`include/lv/signal/fonts/OFL.txt`) |
| `examples/coffee/*/fonts/*.ttf`, `examples/coffee/*/design/fonts/*.ttf` | Copies of the files in `site/media/fonts/`, for the steps of the coffee machine tutorial to embed | SIL Open Font License 1.1 (the `OFL.txt` beside each copy) |
| `examples/coffee/assets/*.png` | The four drink photos and the steam mask of the coffee machine, generated with AI for this repository (fal nano-banana-2; the photos cut out by background removal) | Free to use under the repository's licence (MIT) |
| `include/lv/icons/Phosphor-Bold.ttf`, `include/lv/icons/phosphor.names` | [Phosphor Icons](https://github.com/phosphor-icons/web) 2.1.1, Copyright 2020-2021 Phosphor Icons: the Bold font, and the names and code points of its icons | MIT (`include/lv/icons/LICENSE-Phosphor.txt`) |
| `tools/dtcg/2025.10/*.json` | The JSON Schemas of the [Design Tokens Community Group](https://github.com/design-tokens/community-group), unchanged | W3C Software and Document License, and W3C 3-clause BSD License (`tools/dtcg/LICENSE.md`) |
| `design/signal` | See [design/signal/NOTICE.md](design/signal/NOTICE.md) | |

A program that uses `lv::font` or `lv::typeface` is built with `coverage.hpp`. That code runs in the compiler only: none of it reaches the program. The font file you embed has its own licence, which says whether its glyphs may be put into a device.

A program that includes `lv/signal_type.hpp` holds glyphs drawn from Barlow. The font files themselves do not reach the program. The licence of the glyphs is in `include/lv/signal/fonts/OFL.txt`: read it for what it asks of a product.

A program that uses `lv::icons::phosphor_bold_ttf` holds glyphs drawn from Phosphor. The font file itself does not reach the program. The licence is in `include/lv/icons/LICENSE-Phosphor.txt`.
