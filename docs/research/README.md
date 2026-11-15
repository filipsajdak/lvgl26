# Research notes

These files hold what has already been looked up for lvgl26, with sources. Read the topic file before a web search. Add what you find to it in the same change.

## Topics

| File | What it answers |
|---|---|
| [lvgl-upstream.md](lvgl-upstream.md) | LVGL releases and what v10 removes. Its machine-readable API, introspection, observer, XML and Pro, styles, assets, threading, memory and testing. Existing C++ wrappers, name conflicts, and what the maintainers want. |
| [cpp26.md](cpp26.md) | Pattern matching papers. The compile-time cost of reflection. How others read semantics from doc comments. The boost-ext/ut configuration pattern. Coroutines on microcontrollers. |
| [example-ports.md](example-ports.md) | The examples of Qt, Slint, TouchGFX, emWin, Embedded Wizard, Crank and Altia: what exists, under which licence, and what we may do with it. |
| [device-ui.md](device-ui.md) | What drives the UI of each device class. Standards for process panels, medical alarms and charger prices. Touch target sizes. Text size by viewing distance. |
| [instrument-cluster.md](instrument-cluster.md) | The information hierarchy of a cluster, glance limits, what the steering wheel hides, telltales, warning priority, tyre pressure, speedometer studies. |
| [3d-and-effects.md](3d-and-effects.md) | 3D in LVGL, the plan for microcontrollers, generating 3D models, shadows and blur. |
| [web-target.md](web-target.md) | Whether an lvgl26 application can run in a browser, and by which route. |
| [agent-docs.md](agent-docs.md) | How a library gives its documentation to AI coding agents: measurements, formats, what other projects ship. |

## How each claim is marked

| Mark | Meaning |
|---|---|
| `source` | Someone read the primary source, or checked LVGL's code or ours. |
| `measured` | Our own measurement. The note says how. |
| `summary` | A search engine's summary of the linked page. The page itself was not read. |
| `recalled` | From memory. No source was found. Verify it before you rely on it. |

A date after the mark says when it was checked.

## Rules

1. Read the topic file before a web search.
2. Search only for what the notes do not answer, or for a `summary` or `recalled` claim that your work depends on.
3. Add each new finding to its topic file: the claim, what it means for lvgl26, the link, the mark and the date.
4. Raise a mark to `source` only after reading the source.
5. Correct a wrong claim in place, with the date. Git keeps the history.
6. A new topic gets a new file and a row in the table above.
7. Keep private data out: no account names, no prices agreed with a vendor, no user names.
