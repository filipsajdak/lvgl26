# What this example takes from others

The printer panel is a port of the printer demo of Slint
(<https://github.com/slint-ui/slint>, `demos/printerdemo`, commit f3c4537 of 2026-11-15).
The code is written for lvgl26. What follows is taken from the original.

| What | From | Licence |
|---|---|---|
| The layout, the colors, the sizes and the behaviour of the screens | `demos/printerdemo/ui/*.slint`, Copyright (c) SixtyFPS GmbH, original design by Matthias Grimme | MIT |
| The icons in `images/` (converted from SVG to PNG) | system-uicons by CoreyGinnivan, <https://github.com/CoreyGinnivan/system-uicons> | Unlicense |
| The font in `fonts/`: Inter 24pt Regular and Medium | Inter, Copyright Google Inc., <https://fonts.google.com/specimen/Inter/license> | SIL Open Font License 1.1 (`fonts/OFL.txt`) |
| `photos/cat.jpg` | zaimoku_woodpile, <https://www.flickr.com/photos/11250735@N07/8561945042>, cropped and resized | CC-BY-2.0 |
| `photos/dog.jpg` | Imk0278, <https://commons.wikimedia.org/wiki/File:Kintamani_dog_white.jpg>, cropped and resized | Public domain (CC-PDDC) |
| `photos/elephant.jpg` | <https://commons.wikimedia.org/wiki/File:1968-elefante.jpg>, cropped and resized | Public domain in the United States |
| `photos/snake.jpg` | A US government employee, <https://commons.wikimedia.org/wiki/File:Gopher_Snake_(11970141595).jpg>, cropped and resized | Public domain in the United States |

The photos are the original's photos, cropped again to the size of this screen.
The Slint logo is not used. The Inter fonts are used unchanged; the compiler turns them into glyph pictures.

The Slint toolkit is not used.

MIT License, for the parts taken from Slint's demo:

Copyright © SixtyFPS GmbH <info@slint.dev>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
