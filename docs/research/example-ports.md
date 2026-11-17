# Examples of other frameworks

What other frameworks publish as examples, under which licence, and what we may do with each. Marks follow [README.md](README.md).

Read the licence file of the exact version before copying any code or asset. A search summary is not enough.

## What we may do

| Source | Examples | Licence of the examples | What we may do | Mark |
|---|---|---|---|---|
| LVGL's own examples, v9.6.0 | 299 files, 163 of them widgets | MIT | Rewrite freely | `source` 2026-11-13 |
| Slint demos | Printer, energy monitor, home automation, weather, usecases | MIT for the `.slint` files; assets vary, see below | Port structure and assets, with attribution | `source` 2026-11-15 |
| Qt for MCUs demos | Automotive cluster, motorcycle cluster, thermostat, watch, a game | Qt commercial | Clean-room only | `source` 2026-11-13 |
| Qt for MCUs examples | About 30 documented | Qt commercial, checked on one file | Clean-room only | `source` 2026-11-13 |
| Qt 6 examples with an industrial use | Robot arm, OPC UA water pump, CAN and Modbus, MQTT, thermostat, coffee machine | BSD-3-Clause or commercial | Port code and assets, keeping Qt's notice | `source` 2026-11-13 |
| Qt Design Studio examples | Washing machine, e-bike, treadmill, clusters; 39 folders | BSD-3-Clause | Same | `source` 2026-11-13 |
| Qt's marketing demos | Industrial automation, FactoryPulse, vehicle HMIs | No source published | Appearance only, from videos | `source` 2026-11-13 |
| TouchGFX | Feature examples and larger demos in the Designer | ST's mixed licence SLA0048, tied to ST hardware | Clean-room | `summary` 2026-11-15 |
| SEGGER emWin and AppWizard | Temperature controller, washing machine, weather, oven, thermostat, smart watch, analog clock | Not found | Clean-room | `summary` 2026-11-15 |
| Embedded Wizard | ClimateCabinet, PaperCutter, BezierClock | A tool licence; the demos' own terms not found | Clean-room | `summary` 2026-11-15 |
| Crank Storyboard | Samples inside the Designer, demo images per board | Not found | Clean-room | `summary` 2026-11-15 |
| Altia | Nothing found | Unknown | Skip | `summary` 2026-11-15 |

Clean-room means: rebuild from published screenshots and behaviour, with our own or openly licensed assets, and without reading the source.

## Slint, checked in its repository

Checked on a clone of `slint-ui/slint` at commit `f3c4537` of 2026-11-15. `source`.

| Part | Licence |
|---|---|
| The `.slint` files of the printer demo | MIT, copyright SixtyFPS GmbH |
| Its icons | The Unlicense (system-uicons) |
| The Inter font | OFL-1.1 |
| Its photos | CC-BY-2.0 and CC-PDDC |
| The Slint logo | CC-BY-ND; we do not use it |

Re-read for `docs/port-slint-printer.md` on 2026-11-17, from [`demos/printerdemo/ui` at f3c4537](https://github.com/slint-ui/slint/tree/f3c4537/demos/printerdemo/ui). `source`. The quoted lines are verbatim and short.

| Fact | File |
|---|---|
| 21 `.slint` files, 2175 lines in all. Four themes in `common.slint`, switched by a menu in `components/sidebar.slint`. | `ui/` |
| The window shows a page with `if PrinterState.active-page == Page.home: home-page := HomePage { }`, one for each page. | `printerdemo.slint` |
| The queue is a `for queue-item[idx] in PrinterQueue.printer-queue` over a model. | `pages/printer_queue.slint` |
| `SpinBox` owns `value`, `minimum` and `maximum`. | `components/spinbox.slint` |
| The four sub-pages are children of the home page, each with `x` bound to the sub-page state. Only `PrintPage` has `animate x` (125 ms, `ease`). | `pages/home_page.slint` |
| The ink page starts a 30 ms `Timer`, then the state `inactive` leaves and the height animates for 750 ms with `ease-in-out`. | `pages/ink_page.slint` |

The toolkit itself is under GPLv3, a royalty-free licence or a commercial one. The royalty-free licence excludes embedded use. `summary` 2026-11-15, [licence summary](https://components.espressif.com/components/slint/slint/versions/1.17.0/license?language=en).

## Qt Design Studio examples, checked in their repository

Checked on `public-demos/qtdesign-studio` at <https://git.qt.io/public-demos/qtdesign-studio>, commit `0c1ccf18` of 2026-05-08. `source` 2026-11-17.

| Part | Licence |
|---|---|
| The QML of every example | `LICENSE.md` of the repository: a Qt header that offers a commercial licence or BSD-3-Clause. The QML files carry no header of their own |
| The pictures and fonts | The same file covers them, but they are Git LFS files: fetch them from `/-/raw/master/...`, not the API's file route, which returns a pointer |
| The Exo font of `TreadmillMcuDemo` | SIL Open Font License 1.1, read from the name table of each TTF (copyright 2017 The Exo Project Authors) |
| The Qt logo in the splash screen | A Qt mark: not carried over |

- The repository has 39 example folders under `examples/`. `washingMachineUI` lists its programs with a `ListView` over a `ListModel`; `EBikeDesign` has a `ListModel` and a scrolling map; `TreadmillMcuDemo` has neither.
- `TreadmillMcuDemo` is the one the port uses: 800 x 480, a splash screen, one screen, three looks, and no list, chart, shader or 3D. See `examples/ports/qt/treadmill`.

## Qt, in detail

- The QML and images of Qt for MCUs must not be copied into this MIT repository. `source` 2026-11-13.
- The Monotype fonts in the thermostat and text-input examples are proprietary.
- "Industrial" is not a Qt docs category. The industrial rows above are a selection.
- A BSD-3 file keeps its licence inside this repository, in its own directory with the notice.
- Not confirmed: the full list of examples that ship only inside the SDK, and per-project font licences in the Design Studio repository.

Lists: [Qt for MCUs demos](https://doc.qt.io/QtForMCUs/quldemos.html), [attributions](https://doc.qt.io/QtForMCU/attributions.html).

## Recommended order of ports

1. Slint printer demo. It has a live reference and exercises pages, lists and images.
2. Slint energy monitor. Charts and a dashboard layout.
3. Qt for MCUs thermostat, clean-room.
4. emWin washing machine or oven, clean-room.
5. A smart watch, clean-room.

What the ports force us to build: an image widget, lists from a data model, page transitions, a chart, and fonts beyond LVGL's default.

## Naming

- Name a port after what it shows, such as "printer panel".
- Do not use another framework's logo or trademarked name in a port.

## Other sources

- [Slint demos README, v1.16.0](https://cdn.jsdelivr.net/gh/slint-ui/slint@1.16.0/demos/README.md)
- [TouchGFX examples](https://support.touchgfx.com/docs/development/ui-development/working-with-touchgfx/examples) and [licence terms](https://www.st.com/resource/en/additional_license_terms/additional-license-terms-x-cube-touchgfx.html)
- [SEGGER emWin demos](https://www.segger.com/products/user-interface/emwin/technology/demos) and [samples](https://www.segger.com/emwin-samples.html)
- [Embedded Wizard on Arm Keil](https://www.keil.arm.com/boards/nxp-imx-rt1060-guis-powered-by-embedded-wizard-c280cd5/projects/?page=1)
- [Crank Storyboard samples](https://resources.cranksoftware.com/cranksoftware/v4.0.0/docs/ch12.html)

## Pictures of the originals

Where each original's own picture is, and whether the site may show it. Checked 2026-11-17. `source`.

| Original | The picture | Licence | On the site |
|---|---|---|---|
| Slint printer demo | `https://slint.dev/resources/printerdemo_screenshot.png`, linked from `demos/printerdemo/README.md`. A thumbnail in `demos/README.md` is a GitHub attachment | None stated. Neither file is in the repository, so its MIT licence does not cover them | No. The card links the demo and its live simulation, `https://snapshots.slint.dev/master/demos/printerdemo/` |
| Qt Design Studio `TreadmillMcuDemo` | `examples/dataImports/DataModels/images/treadmill_demo.png` at commit `0c1ccf18`, 250 x 160, the Sporty look. Fetch it from `https://git.qt.io/public-demos/qtdesign-studio/-/raw/0c1ccf18/examples/dataImports/DataModels/images/treadmill_demo.png` | `LICENSE.md` of the repository, BSD-3-Clause option | Yes, unchanged, in `site/media/originals/`. The copyright line and the licence are in `site/media/NOTICE.txt` |

- The Slint repository's `REUSE.toml` lists no licence for either screenshot. A written licence for the image would change the answer.
- The `readme_screenshots/` of `TreadmillMcuDemo` show the Design Studio editor, not the running screens. They are not used.
