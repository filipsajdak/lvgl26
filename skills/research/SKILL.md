---
name: research
description: Use before any web search or research about LVGL (releases, v10 changes, API, licence, existing C++ wrappers), C++26 (pattern matching, reflection compile time, coroutines on microcontrollers), the examples and licences of Qt, Slint, TouchGFX, emWin and other UI frameworks, device UI standards (touch target sizes, text size by viewing distance, ISA-101, IEC 60601-1-8 alarms, AFIR charger prices), instrument cluster design, or 3D, shaders, shadows and blur on embedded screens. It points to notes that already answer many of these, with sources.
version: 0.1.0
---

# Research already done for lvgl26

The notes are in the lvgl26 repository, not in this file. Read them from there, so that this skill never goes stale.

## Steps

1. Read the index: [docs/research/README.md](../../docs/research/README.md). It lists the topics and explains the four marks.
2. Read the one topic file that fits the question.
3. Answer from the notes when they cover it. Give the link the note gives, and say how the claim is marked.
4. Search the web only for what the notes do not answer, or for a claim marked `summary` or `recalled` that the work depends on.

## Topic files

| Question about | File |
|---|---|
| LVGL itself, its versions and API, other C++ wrappers | [lvgl-upstream.md](../../docs/research/lvgl-upstream.md) |
| C++26 features, compile time, coroutines on microcontrollers | [cpp26.md](../../docs/research/cpp26.md) |
| Examples of other UI frameworks and their licences | [example-ports.md](../../docs/research/example-ports.md) |
| What a device screen must do; touch targets; text size | [device-ui.md](../../docs/research/device-ui.md) |
| A vehicle instrument cluster | [instrument-cluster.md](../../docs/research/instrument-cluster.md) |
| 3D models, shaders, shadows, blur | [3d-and-effects.md](../../docs/research/3d-and-effects.md) |

## What the marks mean

- `source`: someone read the primary source. Rely on it.
- `measured`: a measurement made for lvgl26. The note says how.
- `summary`: a search engine's summary of the page. Say so when you pass it on.
- `recalled`: from memory, with no source. Verify it before anything depends on it.

## After a new search

Inside the lvgl26 repository, add the finding to its topic file in the same change. Write the claim, what it means for lvgl26, the link, the mark and the date. Outside that repository, tell the person what you found and that the notes do not have it yet.
