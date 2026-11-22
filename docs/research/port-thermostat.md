# Port design: a smart thermostat

A design for the port of Qt Quick's Thermostat demo to lvgl26, written on 2026-11-22 before any code.
Marks follow [README.md](README.md). The port starts only once the property order (pull request 204) and the reformat of the examples are merged.

## The original

| Fact | Mark |
|---|---|
| Qt's `qtdoc` repository, `examples/demos/thermostat`: about 60 QML files, the doc page "Thermostat", a home thermostat that adapts to four window sizes | `source`, read on GitHub 2026-11-22 |
| The QML is `LicenseRef-Qt-Commercial OR BSD-3-Clause`: it may be ported, keeping Qt's notice in the port's folder | `source`, file headers |
| Icons: Phosphor Icons under CC-BY-4.0 (20 files) and Google's Material icons under Apache-2.0 (8 files), per `REUSE.toml`. Both allow use with attribution. They are SVG, so the port converts them to PNG | `source`, `REUSE.toml`, `qt_attribution.json` |
| The Qt logo in the header is a mark: not carried over | `source` |
| Five views, chosen in a side bar or a bottom bar: Rooms, Thermostat Control, Schedule, Stats, Settings | `source`, `HomePageForm.ui.qml` |
| Data: a list of rooms (name, floor, icon, mode, on or off, temperature, set temperature, humidity, energy) and twelve months of statistics per room | `source`, `RoomsModel.qml`, `Room.qml` |

The port shows the layout the demo uses at 800 x 480, landscape: the bottom bar and the narrow forms of each view.

## Screens

| Screen | What it shows | lvgl26 it uses | Gap |
|---|---|---|---|
| Rooms | A card per room: icon, name, floor, temperature, humidity, an on switch | `lv::list` of rooms with `lv::each`, `lv::bind(&room::field)`, a look per card | None |
| Thermostat Control | A dial for the set temperature of the chosen room, its mode, its power | An arc bound to a temperature state, a dropdown of rooms, a dropdown of modes (`lv::options_of`) | The mode buttons: one checked by a state of an enum (see the gaps below) |
| Schedule | A calendar, and times for each room | A calendar, rollers for hours and minutes | The calendar's header is an `add_` child: the port waits for it |
| Stats | Temperature, humidity and energy over twelve months, per room | A chart, a dropdown of rooms | A chart's series is an `add_` child: the port waits for it |
| Settings | Power, mode and the room's options | Switches and buttons bound to states | None |
| Bottom bar | The five views | `lv::shown` of pages in other files, a state of an enum for the view | None |

The theme button of the original switches light and dark while the program runs. A look cannot follow a state yet (`gap-runtime-theming` on the status page), so the port has the dark look only.

## State

| State | Type | Where |
|---|---|---|
| The view on screen | `enum class view { rooms, control, schedule, stats, settings }` | `state.hpp`: the bottom bar and the shell |
| The rooms | `lv::list<room, 8>`, a room with a named id, its texts, a mode enum, `bool` power, temperatures as mp-units quantities, humidity as a percentage | `state.hpp`: every screen reads it |
| The chosen room | `room_id` | `state.hpp` |
| A simulated room temperature | a task per screen that moves the temperature toward the set one | `screens/control.cpp`, its root as owner |

A temperature is a quantity of degrees Celsius with an integer number, as a state needs. It is a quantity, not a point of mp-units: a state holds no point. The original's statistics have one decimal, so the Stats screen keeps tenths of a degree as its unit.

## Files

The layout of the owner's decision "Projects split into files" (`documentation.md`):

```
examples/ports/qt/thermostat/
  main.cpp            starts the application, mounts the shell
  shell.cpp .hpp      the header, the bottom bar, lv::shown of the screens
  state.hpp           the rooms, the chosen room, the view
  screens/            rooms, control, schedule, stats, settings: a screen and its own states each
  parts/              a room card, a mode button, a labelled value
  design/             tokens.json, design.hpp (the token file and the looks), fonts.hpp, one file per font
  platform/window.cpp the only file that names SDL
  icons/              the PNG icons, with NOTICE.txt (CC-BY-4.0, Apache-2.0)
  lvgl26.json         settings: no files, no threads
  CMakeLists.txt, README.md (what differs, the licences)
```

## Tests

- A picture per screen, written by a test into the port's folder and compared byte for byte, as the printer port does.
- Steps: a room chosen, the dial moved, a mode chosen, the power off, a view switched by the bottom bar.

## The owner's decisions (2026-11-22)

- The thermostat waits for the `add_` children: node kinds of their own for each parent (`lv::tab`, `lv::menu_page`, `lv::series`, `lv::list_item`), not built yet. Then it is ported with all five screens. The media player goes first.
- The icons are converted to PNG. `NOTICE.md` gets one entry for each set with its licence text: Phosphor Icons under CC-BY-4.0, Google's Material icons under Apache-2.0, as font-rs has.
- One look. Switching light and dark at run time is a known gap, named in the port's README.
- No plumbing to work around a gap. Each one below is recorded, and the port uses the best form that exists today; the ones that matter go to the owner as features.

## Gaps, and the form the port uses today

| Gap | The form today |
|---|---|
| A calendar header and a chart series are `add_` children | None: the port waits for them |
| A group of buttons, one checked by a state of an enum (the modes) | A dropdown of the modes, `lv::options_of<mode>`, bound to the state. The README says that the original shows a row of round buttons |
| Light and dark at run time | One look |
