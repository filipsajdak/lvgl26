# Project settings: what a project allows, in one file

Research and design of 2026-11-19, against `origin/main` at `b67550f`.
Marks follow `docs/research/README.md`.
Every mark is dated 2026-11-19 unless its row says otherwise.

The owner's words: "if we want to make this framework cross-platform (without defines/ifdefs) we should be able to enable/disable some features ... E.g. exceptions, multithreading, filesystem, dynamic memory allocation, etc."
His standing goals apply: less code with more meaning, facts for the compiler, buffers sized at compile time, no run-time allocation.

How the work was done.

- Primary sources were fetched with `curl` and read as text; the links are in each row.
- The repository was searched with `grep` at `b67550f`.
- The Cortex-M4 slice (`examples/mcu/slice.cpp`) was built with the preset `arm-m4` (LVGL v9.6.0, `MinSizeRel`), and its symbols read with `arm-none-eabi-nm` and `-objdump`.
- A prototype of the reader was compiled with GCC 16.2 and the flags of `host-v96`.
  It lived in a scratch directory outside the repository.
- Compile cost was counted as instructions retired by `cc1plus`, with `/usr/bin/time -l`, five runs alternating between variants.

## Short answers

| Question | Answer |
|---|---|
| What is a setting? | A restriction, which only forbids, or a variant, which changes what is generated. |
| Where does it live? | In `lvgl26.json` in the project, read by CMake and embedded by the compiler. |
| How does the compiler find it? | CMake copies it into the build tree, writes a header beside the copy, and force-includes that header in every C++ file. |
| What does reading it cost? | 0.04 G instructions in a file that uses no setting, if the force-included header stays thin. |
| Can two files disagree? | Not inside one CMake build, and a pre-link scan of objects catches a library built elsewhere. |
| What do the four restrictions cover? | lvgl26's own API, found by grep; the user's own code is out of reach of a compile error. |
| Is a check of the linked program worth it? | Yes, as a second level after the stack budget, which needs the same step. |
| Where are settings described? | As annotations on the members of `lv::config`, printed by reflection to `docs/reference/` with a JSON Schema. |
| What does the skill do? | It asks what the program is and where it runs, proposes a file, and lets the developer adjust it. |

## What others learned

| Who | What they do | Link | Mark |
|---|---|---|---|
| Cargo | When several packages enable features of one dependency, "Cargo will use the union of all features enabled on that dependency". | [features](https://doc.rust-lang.org/cargo/reference/features.html) | `source` |
| Cargo | "features should be additive": enabling one "should not disable functionality". | same | `source` |
| Cargo | For `no_std`, "do not use a `no_std` feature. Instead, use a `std` feature that enables `std`". | same | `source` |
| Cargo | Mutually exclusive features "should be avoided"; if not, add "a compile error to detect this scenario". | same | `source` |
| Qt | `configure -no-feature-<name>` removes a feature from the build of Qt itself. | [configure options](https://doc.qt.io/qt-6/configure-options.html) | `source` |
| Qt | "features can depend on other features, so disabling a feature might have side-effects on other features." | same | `source` |
| Qt | `QT_CONFIG(feature)` is `(1/QT_FEATURE_##feature == 1)`: a feature that is neither on nor off is a compile error, not a silent false. | [qtconfigmacros.h](https://github.com/qt/qtbase/blob/409e6a4df9124f0cb7ef48393b58b31feb429f1e/src/corelib/global/qtconfigmacros.h) | `source` |
| Qt | `QT_REQUIRE_CONFIG(feature)` is a `static_assert` with "Required feature ... for file ... not available." | same | `source` |
| Qt Lite | The blog post fetched as script only; its text was not read. | [blog](https://www.qt.io/blog/2017/05/31/qt-lite-in-qt-5-9) | not read |
| Zephyr | A board's `_defconfig` is merged with the application's `prj.conf`. | [setting values](https://docs.zephyrproject.org/latest/build/kconfig/setting.html) | `source` |
| Zephyr | "Assignments in configuration files are only respected if the dependencies for the symbol are satisfied. A warning is printed otherwise." | same | `source` |
| Zephyr | `KCONFIG_WARNING_AS_ERROR` exists because "a silently ignored configuration setting is likely to be a mistake". | same | `source` |
| LVGL | `lv_conf_template.h` of the v10 pin has 2,599 lines and 210 `LV_USE_` lines; its `Kconfig` has 482 `config` entries. | `build/host-v10/_deps/lvgl-src` | `source` |
| LVGL | `lv_conf_internal.h` gives each option a default: `#ifndef LV_MEM_SIZE`, then `CONFIG_LV_MEM_SIZE` from Kconfig, then 65536. | same, `include/lvgl/config/lv_conf_internal.h` | `source` |
| LVGL | lvgl26 sets `LV_CONF_SKIP` and passes LVGL's options as compile definitions from CMake. | `cmake/lvgl.cmake` | `source` |
| libc++ | Hardening modes are encoded into ABI tags "to allow translation units built with different hardening modes to interact with each other without causing ODR violations". | [Hardening](https://libcxx.llvm.org/Hardening.html) | `source` |
| libc++ | The tags separate the builds' symbols; they do not report a mismatch. | same | `source` |
| MSVC | `#pragma detect_mismatch("name", "value")` makes the linker fail with LNK2038 when two objects carry the same name with different values. | [detect_mismatch](https://learn.microsoft.com/en-us/cpp/preprocessor/detect-mismatch) | `source` |
| ETL | A project writes an `etl_profile.h`, found by `__has_include`, with macros such as `ETL_NO_STL` and `ETL_TARGET_OS_NONE`. | [platform.h](https://github.com/ETLCPP/etl/blob/fc45e7a0915baf889ba3e676e42c6e7a0fe4cd46/include/etl/platform.h), [profiles](https://github.com/ETLCPP/etl/tree/fc45e7a0915baf889ba3e676e42c6e7a0fe4cd46/include/etl/profiles) | `source` |
| ETL | It ships ready profiles per target and standard, such as `armv7_no_stl.h` and `cpp17_no_stl.h`. | same | `source` |
| C++ contracts | "The mechanism by which the evaluation semantic ... is chosen is implementation-defined." | [P2900R14](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2900r14.pdf), 3.5.5 | `source` |
| C++ contracts | Different semantics in different translation units ("mixed mode") are "not, in and of itself, an ODR violation". | same, 3.5.11 | `source` |
| C++ contracts | Then which inline definition runs "will be effectively random" without linker changes. | same | `source` |
| GCC 16.2 | `-fcontract-evaluation-semantic=[ignore\|observe\|enforce\|quick_enforce]` chooses the semantic per compile. | `g++-16 --help=c++` | `measured` |
| Stroustrup | "the zero-overhead principle: What you don't use, you don't pay for. And further: What you do use, you couldn't hand code any better." | [Foundations of C++](https://www.stroustrup.com/ETAPS-corrected-draft.pdf) | `source` |
| DTCG | A dimension is an object `{"value": 16, "unit": "px"}`; the string and bare-number forms were removed. | [dtcg.md](dtcg.md), PR 244 | `source` 2026-11-18 |

What it means for us.

1. **Restrictions are additive in Cargo's sense, read backwards.**
   Cargo spells a capability as on (`std`), never as its absence (`no_std`).
   Our `allow.threads` is spelled the same way, true by default.
   One program has one settings file, so no union of requests from several libraries arises.
2. **An unknown or impossible setting is an error, never a warning.**
   Qt's three-state macro and Zephyr's warning-as-error flag both exist because silence was the bug.
3. **A required feature names the file that requires it.**
   `QT_REQUIRE_CONFIG` is the shape of our message: what needs it, and the key that forbids it.
4. **A mismatch between objects needs a mark in the objects.**
   MSVC has a linker record for it; libc++ only separates the symbols.
   We have neither in GCC, so the mark is a symbol named by the file's hash (measured below).
5. **A restriction is like a contract semantic: it changes no code of the library.**
   Contracts allow mixed mode only because the definition stays the same.
   A variant changes definitions, so mixed variants are an ODR violation, and must be caught.
6. **Ready profiles help.**
   ETL ships one per target; the skill's proposals play that part, as files the tests compile.
7. **Zero overhead is the test of a restriction.**
   A restriction that is not used must leave the object code as it was.

## What the owner decided

Decided 2026-11-19, in an interview.

| # | Decision |
|---|---|
| 1 | A project states switches for single features and limits the compiler checks: "All is accepted as long as we can check it in compile time." |
| 2 | The reason is portability as much as cost: a platform may lack exceptions, threads, files or allocation, or a project may forbid them. |
| 3 | A restriction only forbids and costs a compile-fail test; a variant changes what is generated, stays rare, and needs a measured reason. |
| 4 | Everything is on by default, and a project switches off to shrink; optional parts of LVGL that cost code or heap stay off. |
| 5 | Settings hold for the whole program; two files compiled with different settings are a build error. |
| 6 | A stack limit, later: a compile error at the call, and a build step over GCC's stack report, `lvgl26_stack_budget(target bytes)`. |
| 7 | The first restrictions: no dynamic allocation, no threads, no files or standard streams, no floating point at run time. |
| 8 | The spelling is `"allow": {"allocation": false, ...}`; a message reads "lv: lv::ui_scheduler needs threads, and this project does not allow them (allow.threads)." |
| 9 | One JSON file, such as `lvgl26.json`, read by CMake (`lvgl26_settings(target lvgl26.json)`) and embedded by the compiler. |
| 10 | LVGL's own options join the same file later, with a compile-time check that LVGL was built to match. |
| 11 | A skill first, which interviews the developer and proposes a file; a wizard built with lvgl26 later, driven through the MCP server. |
| 12 | The first pull request is a thin slice: this note, the four restrictions, the settings as data, the first skill. |
| 13 | Bindings in `set()` are the first variant, observed, not built here. |

### The design checkpoint

Decided 2026-11-19, over the open choices below.
The owner decided 2, 3, 4 and 5; the team lead decided 1, 6 and 7 on this note's recommendation.

| # | Choice | Decision |
|---|---|---|
| 14 | 1, thin or eager | The thin force-included header with a lazy read. The real thin header is measured on an empty file, `smoke` and `large_tree`, and `large_tree` must not get dearer. |
| 15 | 2, file and C++ form | "Using both is an error; one or the other". A project with `lvgl26.json` may not also specialize `lv::cfg`: a compile error that names the file. The C++ form stays for a build without CMake and for the library's tests. |
| 16 | 3, the strict settings | "A section of their own, rules": `"rules": {"raw_style_values": true}`, with `raw_integer_state` beside it, strict by default. `allow` keeps one meaning: everything permitted by default. `compile_time_image_pixels` goes to `limits`. |
| 17 | 4, what allocation is | "The system heap only; LVGL's fixed pool is allowed". `"allocation": false` forbids `new`, `malloc` and growing standard containers. Tasks are an error under it until the fixed task region exists, a known limit in the docs and the status page. |
| 18 | 5, units | "A value and a unit, as design tokens do": `{"value": 4, "unit": "KiB"}`. A unit typo is a compile error that lists the units, and the schema lists them too. A fraction that gives no whole number is refused. |
| 19 | 6, the mismatch check | Both the pre-link object scan and the undefined reference. |
| 20 | 7, files and LVGL | `allow.files` covers lvgl26's own API now, and LVGL's file drivers when its options join the file. |

## The design

### The file

The file is `lvgl26.json` in the project's source directory, by convention.
The project names it once: `lvgl26_settings(app lvgl26.json)`.

```json
{
  "$schema": "https://filipsajdak.github.io/lvgl26/reference/lvgl26-settings.schema.json",
  "allow": {"allocation": false, "threads": false, "files": false, "floating_point": false},
  "limits": {"compile_time_image_pixels": 16384}
}
```

What `lvgl26_settings(target file)` does.

1. It copies the file into `${CMAKE_BINARY_DIR}/lvgl26-settings/lvgl26.json` with `configure_file(COPYONLY)`, so CMake runs again when it changes.
2. It writes `lvgl26-settings/settings.hpp` beside the copy: the `#embed` of its neighbour, and the specialization that names it.
   The rule "a header embeds only a file that lies next to it" holds.
3. It force-includes that header (`-include`) in every C++ file of every target that links `lvgl26::lvgl26`, through the interface target.
   So every file of the build sees the same settings, and no file can forget them.
4. A second call with a different file is a configure error: "a program has one settings file".
5. The target named in the call is the program the later checks run on: the stack budget, the symbol scan.

The generated header, as prototyped:

```cpp
#include <lv/config.hpp>
constexpr char lvgl26_settings_json[] = {
#embed "lvgl26.json"
    , 0};
template <>
inline constexpr const char* lv::settings_file<lv::override> = lvgl26_settings_json;
```

It also carries the file's hash as text, for two reasons.

- ccache before 4.14 does not see a changed `#embed` file in direct mode (`CLAUDE.md`).
  A hash in the header changes the header, so no stale object survives an edit.
- The hash names a symbol that marks each object, for the mismatch check below.

### A mismatch between objects

Inside one CMake build a mismatch cannot arise: one file, one header, every file.
It can arise when a library compiled elsewhere is linked in.

| Mark | Result | Mark |
|---|---|---|
| Each object references `lvgl26_settings_<hash>` from a `[[gnu::used]]` inline function; the program defines only its own hash. | Apple's linker: "Undefined symbols ... `_lvgl26_settings_bbbb`". | `measured` |
| The same, GNU ld 16.2 for Arm, without `--gc-sections`. | "undefined reference to `lvgl26_settings_bbbb`". | `measured` |
| The same, GNU ld with `--gc-sections`. | No error: the referencing section is discarded first. | `measured` |
| `[[gnu::retain]]` to keep that section. | arm-none-eabi GCC 16.2 ignores it, "'retain' attribute ignored", for a reason not known. | `measured` |
| `nm` of each object and archive before the link. | Every object shows `U lvgl26_settings_<hash>`. | `measured` |

Recommendation: a pre-link step in `lvgl26_settings` that runs `nm` over the program's objects and archives.
It fails when it finds a hash other than the program's.
It gives a build error on every linker, with a message that names both files.
The undefined reference stays as a fallback where no CMake step runs.

### From the file to `lv::config`

`lv::config` stays the one source.
It gets nested structs for the sections of the file: `allow` now, `limits` and `lvgl` later.
The reader walks the JSON object and the struct's members by reflection, in constant evaluation.

| Mistake | Message, as the prototype prints it | Mark |
|---|---|---|
| Unknown key | "lv: settings: unknown key 'thread' in allow. Valid keys: allocation, threads, files, floating_point." | `measured` |
| Wrong type | "lv: settings: allow.threads takes true or false" | `measured` |
| Not an object | "lv: settings: the file is not a JSON object" | prototyped, not run |

As built in the slice, after the checkpoint.

- "Did you mean" by edit distance, with the helpers that `lv::look` uses, now in `include/lv/detail/nearest.hpp`.
- Only `$schema`, at the top, is skipped.
- A limit refuses a negative value, one beyond 2^53 and a fraction that gives no whole number of its base unit.
- GCC 16.2 prints the message of a failed constant evaluation three times, in every context tried (`measured`).
  A `static_assert` with a computed message it prints once (`measured`).
  So the reader returns its first mistake as text, and `settings_from` asserts on it.

The three settings of today, as decided.

| Before | In the file |
|---|---|
| `raw_style_values` | `rules.raw_style_values`, default false |
| `raw_integer_state` | `rules.raw_integer_state`, default false |
| `compile_time_image_pixels` | `limits.compile_time_image`, `{"value": 16384, "unit": "px"}` |

The C++ form stays for a build without the file and for the library's tests: `lv::cfg<lv::override>{.rules = {.raw_style_values = true}}`.

Both at once is a compile error without the eager cost.
The header that CMake writes specializes `lv::cfg<lv::override>` with the defaults, on one line whose comment says what to do.
A second specialization is then a redefinition, and GCC shows that line: "lv: the settings of this project are in lvgl26.json; use the file or a specialization of lv::cfg, not both" (`measured`, `fail/settings_both_forms.cpp`).
The settings themselves come from the file, read on first use by `detail::project<O>`; the specialization only takes the place.

### Numbers with units

Decided: a value and a unit, as design tokens do: `{"value": 4, "unit": "KiB"}` (decision 18).

- A limit's type in C++ carries its table of units: `lv::pixel_count` takes `px`, `lv::byte_count` takes `B`, `KiB` and `MiB`.
- The reader finds the table by the type, and the schema lists the same units as an `enum`.
- No limit of bytes exists in this slice; `lv::byte_count` is read by the tests, for the stack and the heap that follow.

### The four restrictions

Evidence for each: `grep` of `include/lv` at `b67550f`, and the symbols of the linked Cortex-M4 slice.

The slice links no `operator new`, no `__aeabi_` floating-point helper, and nothing of threads (`measured`).
It links newlib's `_malloc_r`, called only from `__smakebuf_r`, the buffer of `printf` in the example's own `main` (`measured`).
Its FPU is not enabled (`-mcpu=cortex-m4 -mthumb`), so floating point would show as helper calls (`source`, `cmake/arm/toolchain.cmake`).

| Restriction | What needs it in lvgl26 | A template? | Where the check goes |
|---|---|---|---|
| `allow.threads` | `lv::ui_scheduler` (`std::mutex`, `new` of posted work) | No | A `static_assert` in `ui_scheduler.hpp`: fires when the header is included. |
| `allow.threads` | `lv/host.hpp` includes `<thread>` for `sleep_for` only | No | Not a thread: allowed. |
| `allow.files` | `lv::write_png` (`std::fopen`) in `png.hpp` | No | In the header. |
| `allow.files` | `lv::mcp::server::serve` (`std::FILE*`, stdin and stdout) | Yes | At instantiation. |
| `allow.allocation` | `lv::encode_png` returns a `std::vector` | No | In the header, with files. |
| `allow.allocation` | `lv::ui_scheduler::post` (`new (std::nothrow)`) | No | In the header, with threads. |
| `allow.allocation` | `lv::mcp::server` (`std::string` answers) | Yes | At instantiation. |
| `allow.allocation` | A row field of type `std::string` in a list | Yes | In `check_field`, where row types are checked today. |
| `allow.allocation` | A task's frame, from `lv_malloc` per spawn (`task.hpp`) | Yes | In the promise type; see below. |
| `allow.floating_point` | `lv::runtime` reads numbers as `double` at run time | Yes | At instantiation. |
| `allow.floating_point` | A state, computed value or row field of a floating type | Yes | In `lv::state` and `lv::computed`, beside today's integer check. |

Compile time is free: fonts, images and tokens use `float` in constant evaluation, which no restriction touches.
A check in a non-template fires on include; `lv/lv.hpp` includes none of `png.hpp`, `host.hpp`, `ui_scheduler.hpp` or `mcp.hpp` (`source`).

"Dynamic allocation", defined.

- `allow.allocation: false` means the program takes nothing from the system heap: no `malloc`, no `operator new`, no standard container at run time.
- LVGL's heap is a fixed pool in the program's memory, and stays allowed.
- A task frame is taken from LVGL's pool on each spawn, in a number the compiler does not know.
  Under the restriction it is an error, until frames come from the task region of `lv::memory`.
- `lv::memory` and `lv::tasks` do not exist on `main` yet (`source`, grep).
  So the Cortex-M4 slice, which runs a task, cannot set `allocation: false` in this slice.

Four more sites take from LVGL's pool while a tree is built or used: open choice 4.

| Site | When | File at `b67550f` |
|---|---|---|
| The node of a binding | When a bound widget is built | `props.hpp`, `make_node` |
| `each_view` | When a list is mounted | `list.hpp` |
| `widget_watch` | When a task waits on a widget | `task.hpp`, `watch` |
| A probe of the style list | Before a style is added | `detail/style_add.hpp`, `room_for_a_style` |

Another agent is changing the binding code now; the rows may move.

What a compile error can and cannot cover.

- It covers lvgl26's own API, each entry point above.
- It cannot cover the user's own `malloc`, `std::thread` or `fopen`: the compiler has no hook into the user's code.
- A scan of the linked program covers both, as `tools/arm-run.sh` already fails on the exception run-time.
  It needs the post-link step that the stack budget needs too, so it comes after it.
  On a target with an FPU, floating point shows as instructions, not symbols, and needs a scan of the code.

A test that keeps restrictions honest: compile one file with no restriction and with all four, where none is violated, and compare the objects.
They must be identical, since a restriction changes no generated code.

### The settings as data

One source, the struct, with descriptions as annotations, as `lv::caption` already is in `repeat.hpp`:

```cpp
struct allowances {
  [[=lv::described{"Taking memory from the system heap at run time."}]] bool allocation = true;
  // ...
};
```

A program like `tools/reference/reference.cpp` prints two files by reflection.

- `docs/reference/settings.json`: every key, its type, its default, its description, its kind (restriction or variant), and what needs it.
- `docs/reference/lvgl26-settings.schema.json`: a JSON Schema with `additionalProperties: false`, so an editor completes keys and flags a typo.

A test like `reference_data` fails when either file is stale.
The annotation must be a structural type: GCC 16.2 refuses `std::string_view` there (`CLAUDE.md`), so it holds a fixed string.

### Compile cost

Measured with the prototype, five runs each, median instructions retired by `cc1plus`.

| File | Main | Thin header | Full reader in every file | Full reader, read eagerly | Mark |
|---|---|---|---|---|---|
| An empty `.cpp` | 0.221 G | 0.265 G | 4.071 G | 4.194 G | `measured` |
| `tests/smoke.cpp` | 18.54 G | 18.65 G (3 runs) | 19.88 G | 20.09 G | `measured` |
| `tests/large_tree.cpp` | 107.16 G | not measured | 108.01 G | 107.74 G | `measured` |

What the numbers say.

- The read itself is cheap: eager against lazy is 0.12 to 0.2 G in the small files.
- On `large_tree` the full reader costs 0.6 to 0.85 G, under 1 percent, and eager against lazy is within the noise.
- The includes are what cost: `<string>`, `<meta>` and the JSON reader add 3.8 G to an empty file.
- So the force-included header must be thin: the struct and the pointer to the file, no reader.
  The reader lives in a header that only the users of settings include, which include `<string>` and `<meta>` already.
- "Thin" in the table defines no `cfg`; it is a lower bound of that design, not the design.
- `large_tree` reads the settings: a misspelled key fails its compile with the prototype (`measured`).

### The skill

`skills/settings/SKILL.md`, in the plugin.

The conversation, in the order the owner asked for.

1. "What does the program run on: a desktop, a microcontroller, a browser?"
   A microcontroller: which core, how much RAM and flash.
2. "Does your project have rules that forbid something: heap, threads, floating point, files?"
   A coding standard such as MISRA usually answers several at once.
3. "Does the program read files, run threads, load screens at run time, or talk to an agent?"
4. The agent proposes a whole file, with a reason per line, starting from the preset that fits.
5. The developer adjusts; the agent writes `lvgl26.json` and the `lvgl26_settings` line, then builds.
   A compile error from a restriction is shown with the key that caused it.

What it reads: the generated `docs/reference/settings.json` and the docs page, by relative link, never a copy.
What it writes: the file and one CMake line.

How it is tested.

- `tools/check_plugin.py` checks its links.
- The presets it starts from (desktop, microcontroller, browser) are files in `examples/settings/`, each compiled by a test.
- A test checks that every key the skill names exists in the generated reference.
- The conversation itself is not tested automatically.

### Later slices, in order

1. **Limits**: `limits.stack_bytes` and the two-level stack check, then the post-link symbol scan on the same step.
2. **LVGL's options through the file**: an `lvgl` section that CMake turns into compile definitions.
   The compile-time match uses the macro's value in an expression, as `image.hpp` reads `LV_COLOR_DEPTH`; no `#if`.
3. **The task region**: when `lv::memory` and `lv::tasks` exist, tasks work under `allocation: false`.
4. **Variants**: bindings in `set()` first, each with both states in the tests.
5. **The wizard**, driven through the MCP server.

Nothing in this slice blocks them: the sections are nested structs, and the reader walks any struct.

## Open choices, as they were put

They are decided: see "The design checkpoint" above.


1. **Thin header, or eager read?**
   (a) Thin: the force-included header names the file, and `cfg` reads it on first use, for 0.04 G a file.
   (b) Eager: the header specializes `lv::cfg<lv::override>` itself, so a second specialization is a redefinition error.
   It costs about 4 G in a file that uses no setting.
   Recommendation: (a).
2. **A file and the C++ form at once?**
   Under (a), a file that specializes `lv::cfg<lv::override>` overrides the settings file in that file only.
   Options: forbid by documentation; or move the C++ form to a test-only name; or pay (b).
   Recommendation: the C++ form stays for builds without a file, and the docs forbid both.
   The object scan does not see that mismatch, so it stays a documented rule.
3. **Where do `raw_style_values` and `raw_integer_state` go?**
   Under `allow`, with false as their default against decision 4, or in a section of their own such as `rules`.
   Recommendation: `allow`, with the exception written in the reference.
4. **Is LVGL's pool, used while a tree is built, allocation?**
   (a) No: only the system heap and task frames count, as defined above.
   (b) Yes: every site in the table becomes an error until `lv::memory` gives it a region.
   Recommendation: (a) now, revisit with `lv::memory`.
5. **Units in the file**: the unit in the key's name, or the DTCG object.
   Recommendation: the unit in the name.
6. **The mismatch check**: the pre-link scan in CMake, or the undefined reference alone.
   Recommendation: the scan, with the reference as a fallback.
7. **`allow.files` and LVGL's file drivers**: the tests turn on `LV_USE_FS_STDIO`.
   Recommendation: the restriction covers lvgl26's API now, and LVGL's drivers when its options join the file.

## Not confirmed

- Why arm-none-eabi GCC 16.2 ignores `[[gnu::retain]]`.
- Whether GCC's dependency file lists an `#embed` file; the hash in the header makes it moot.
- How a force-included header behaves in a project that builds without CMake.
- The cost of the slice's real reader with "did you mean", until it is written.
- Qt Lite's own account, which was not read.
