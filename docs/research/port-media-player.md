# Port design: a media player

A design for the port of a media player to lvgl26, written on 2026-11-22 before any code.
Marks follow [README.md](README.md). The port starts only once the property order (pull request 204) and the reformat of the examples are merged.

## Which original

| Candidate | What it is | Licence | Fit |
|---|---|---|---|
| Qt Quick's Media Player demo, `qtdoc/examples/demos/mediaplayer` | A player for audio and video files: a cover or the video, playback controls, a seek bar, volume and rate, panels for the playlist, the metadata, the tracks and the settings | QML: `LicenseRef-Qt-Commercial OR BSD-3-Clause`. Its 48 icons carry no `REUSE.toml` entry; their licence is to be read before any is copied | Its screens map onto lvgl26's state, lists, `lv::shown` and tasks. Video and files are out of scope on a device |
| LVGL's music demo, `demos/music` of LVGL v9.6.0 | A music player for 480 x 272 and larger: album art, title, a spectrum, controls, a progress bar, a track list | MIT, with its own pictures and fonts | Native to small devices, and comparable pixel for pixel with the C demo. Its spectrum is drawn in a draw event, its intro fades opacity, and it scrolls with `lv_obj_scroll_by`: three gaps |

Chosen by the owner on 2026-11-22: Qt's demo, as audio only with a playlist of constant tracks and playback simulated by a task. It shows more of lvgl26 and hits fewer gaps. `source` for both rows, read on 2026-11-22 (Qt's files on GitHub, LVGL's in the local v9.6.0 checkout).

## Screens of the recommended port

At 800 x 480, landscape.

| Screen or panel | What it shows | lvgl26 it uses | Gap |
|---|---|---|---|
| Player | The cover art, the title and artist of the current track, the elapsed and total time | An image made by the compiler, labels bound to fields of the current track | The time as `1:05`: a text binding formats one number. The port shows seconds (see the gaps below) |
| Controls | Previous, play or pause, next, shuffle, loop; a seek bar | Buttons whose handlers change states, `.hidden` bound to a computed of the playing state, a slider bound to the position | None |
| Volume and rate | A volume slider with mute, a choice of rate | A slider bound to a percentage, a dropdown of an enum (`lv::options_of`) | None |
| Playlist panel | The tracks, the current one marked, a delete button per row | `lv::list` with `lv::each`, `.selected` bound to the current track, `.on_row` with a task per row | A handler that knows its row (see the gaps below) |
| Metadata and tracks panels | Fields of the current track | Labels bound to fields | None |
| Settings panel | The theme | | Light and dark at run time (`gap-runtime-theming`): the port keeps one look |
| Panels on top | One panel at a time over the player | `lv::shown` on a state of an enum of panels, with a transition | None |

Left out, with the reason in the port's README:

- the video output, since a device here decodes no video;
- opening a file or a URL: there is no file system, and a keyboard joined to a text area is a gap;
- the error popup, which only a failed file causes.

## State

| State | Type | Where |
|---|---|---|
| The playlist | `lv::list<track, 16>`; a track has a named id, title, artist, album, a length as a quantity of seconds, and a kind (music or film) | `state.hpp` |
| The current track | `track_id` | `state.hpp` |
| Playing, shuffle, loop | `bool` each | `state.hpp` |
| The position | a quantity of seconds, written by the playback task | `state.hpp`; the task lives in `screens/player.cpp`, its root as owner |
| Volume | a percentage | `state.hpp` |
| Rate | `enum class rate { half, normal, one_and_half, double_speed }` | `state.hpp` |
| The open panel | `enum class panel { none, playlist, metadata, tracks, settings }` | `shell.hpp` |

## Files

The layout of the owner's decision "Projects split into files" (`documentation.md`): `main.cpp`; `shell.cpp .hpp` with the panels; `state.hpp`; `screens/` (player, playlist, metadata, tracks, settings); `parts/` (an icon button, a labelled value, a time label); `design/` (tokens, looks, a file per font); `platform/window.cpp`; `icons/` with their notice; `lvgl26.json`; `CMakeLists.txt`; `README.md` with what differs.

## Tests

- A picture per screen and panel, written by a test and compared byte for byte.
- Steps: play, ten seconds of playback, next track, a row selected in the playlist, the volume moved, each panel opened.

## The owner's decisions (2026-11-22)

- The original is Qt Quick's Media Player demo, audio only. The media player is ported before the thermostat, once the property order (pull request 204) and the reformat of the examples are merged.
- Each icon's licence is read before it is copied. An icon with no stated licence is replaced by one of our own drawing.
- Copied icons may be converted to PNG. Each icon set gets one entry in `NOTICE.md` with its licence text, as font-rs has.
- One look. Switching light and dark at run time is a known gap, named in the port's README.
- No plumbing to work around a gap. Each one below is recorded, and the port uses the best form that exists today; the ones that matter go to the owner as features.

## Gaps, and the form the port uses today

| Gap | The form today |
|---|---|
| A text made of two numbers, such as `1:05`: a text binding formats one value | The time is bound as a quantity of seconds, which shows `65 s`. The README says that the original shows minutes and seconds |
| A handler in a row template that needs its row, such as the playlist's delete button | `.on_row` starts a task for each row, with the row as its owner. It waits for a click on the row's delete button and erases the row by its key |
| Light and dark at run time | One look |
