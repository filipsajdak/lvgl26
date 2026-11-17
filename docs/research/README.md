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
| [components.md](components.md) | Components: reusable behaviour and its lifetime, dynamic content on small devices, slots, parameters and invariants, in other frameworks and for our trees. |
| [lists.md](lists.md) | Lists whose rows change: a fixed-capacity model, keyed rows, bindings to a row's fields, the selected row, the heap and code a row costs, and the API of recycling and sorted views. |
| [node-storage.md](node-storage.md) | Run-time storage of structural nodes without heap: sized at compile time, placed by the caller, open to new kinds. When a shown tree switches, a style value that follows a state, and what a region for the whole program still needs. |
| [shown-theme-format.md](shown-theme-format.md) | Design of four planned features: a node that shows one of several trees for a state, a look that changes at run time, text from several values in a fixed buffer, and handler arguments injected by reflection. Sketches compiled with GCC 16.2, decisions for the owner. |
| [lsp.md](lsp.md) | Editor support for code that only GCC compiles: clangd and reflection, stub headers, completion inside a string, GCC's diagnostics, what agents can use. |
| [fonts.md](fonts.md) | LVGL's font format, what its converter does, the TrueType a rasteriser needs, SDF and MSDF, and our spike that rasterises a TTF in the compiler. |
| [image-formats.md](image-formats.md) | Web and embedded image formats: ratio, decoder size and RAM, what decodes line by line, and whether an own format is justified. |
| [images.md](images.md) | What LVGL draws from read-only memory, its compression and decoders, what other frameworks choose at build time, and a rule for the compile-time path. |
| [memory.md](memory.md) | Where lvgl26 duplicates or copies data that LVGL already holds, what each copy costs in bytes, and which copies can go without changing LVGL. |
| [memory-region.md](memory-region.md) | Everything an lvgl26 program allocates, measured; how a program declares its whole memory as one typed layout reserved at the start, with LVGL's heap and task frames inside it; how to prove it asks for no more. |
| [web-emulator.md](web-emulator.md) | Whether a small own emulator can run the firmware in a browser instead of QEMU, with a survey of small emulators and measured size and speed. |
| [dev-container.md](dev-container.md) | Which base image and install route give a dev container with GCC 16 on both architectures. |
| [constexpr-limits.md](constexpr-limits.md) | What the constant-evaluation limit costs us: the operations each file needs, what a very high limit risks, and how fonts can need none. |
| [compile-time.md](compile-time.md) | What GCC 16.2 and plugins show about constant evaluation, where the compile time goes (the analyzer, the ports), and a ranked list of levers that cut it. |
| [dependency-cves.md](dependency-cves.md) | Known vulnerabilities in what lvgl26 builds against, and whether our API makes them reachable. |
| [rust-binding.md](rust-binding.md) | The official Rust binding of LVGL: how it is generated and owned, why it froze on LVGL 8.3.7, what users hit, and eleven lessons for lvgl26. |
| [less-boilerplate.md](less-boilerplate.md) | How a port names fewer design tokens and repeats less: looks by name, text by inheritance, trees from enums and annotated members, option lists, a strings file, sub-trees. Counted against Slint. |

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
