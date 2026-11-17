# What this example takes from others

The treadmill panel is a port of the example `TreadmillMcuDemo` of Qt Design Studio
(<https://git.qt.io/public-demos/qtdesign-studio>, `examples/TreadmillMcuDemo`, commit 0c1ccf18 of 2026-05-08).
The code is written for lvgl26. What follows is taken from the original.

| What | From | Licence |
|---|---|---|
| The layout, the colors, the sizes, the three looks and the behaviour of the screens | `examples/TreadmillMcuDemo/*.qml`, `imports/Constants`, `Generated/DesignSystem`, Copyright (C) The Qt Company Ltd | BSD-3-Clause (below) |
| The timeline of the dot on the track (`across` and `down` in `logic.cpp`) | `TrackView.ui.qml` | BSD-3-Clause |
| The pictures in `images/`, unchanged except `speedtrekSmall.png` | `examples/TreadmillMcuDemo/assets`, designed in Figma by The Qt Company | BSD-3-Clause, as the repository's licence says |
| `images/speedtrekSmall.png` | A crop of `backgroundItems_merge_1.png` (the small wordmark in the top bar) | BSD-3-Clause |
| The fonts in `fonts/`: Exo Medium and Exo SemiBold | Exo, Copyright 2017 The Exo Project Authors, <https://github.com/NDISCOVER/Exo-1.0> | SIL Open Font License 1.1 (`fonts/OFL.txt`) |

## What is not taken

- The Qt logo of the splash screen (`assets/qtLogo.png`). It is another framework's mark, and this repository does not carry it.
- The backdrop `bg.png` of the splash screen: its two end colors are tokens, and the lines drawn in it are left out.
- The Figma file and the Qt Bridge export in `figmadesign/`, the other two weights of Exo, and the screenshots of the original's README.
- Qt itself. The port uses no Qt code and no Qt library.

## How the licence was checked

- The repository `public-demos/qtdesign-studio` holds `LICENSE.md`, a Qt header with two options: a commercial licence, or the BSD licence below. This port uses the BSD option.
- The QML files carry no header of their own, so `LICENSE.md` of the repository is the licence that covers them.
- The pictures and fonts are Git LFS files. They were read from the repository at the commit above.
- The name tables of the font files give the font licence (OFL 1.1) and its copyright line.
- The wordmark "SPEEDTREK" is the invented brand of the original's story. It is kept because it is part of the screens.

BSD License, for the parts taken from the Qt Design Studio examples:

```text
Copyright (C) 2018 The Qt Company Ltd. Contact: https://www.qt.io/licensing/

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
- Neither the name of The Qt Company Ltd nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```
