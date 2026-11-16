# Editor support (LSP) for lvgl26: what is feasible

Researched on 2026-11-16. Marks follow `docs/research/README.md`: `source`, `measured`, `summary`, `recalled`.

Machine for all measurements: macOS arm64, GCC 16.2.0 (Homebrew), clangd 23.1.5 (Homebrew).

The machine was under heavy load (load average over 100), so times are CPU user seconds, not wall time.


## Short answer

- No ready tool gives completion for this code today, except possibly CLion 2026.2, which claims reflection support with GCC 16.1.
- Generated plain C++20 stub headers make clangd complete and document the members. A spike confirmed it in about 10 ms.
- No C++ server completes inside a string literal. Quoted values need our own small server, or a check inside the stub.
- GCC is too slow for typing feedback: 4.5 to 11 CPU seconds per file. It fits "check on save".
- Claude Code allows one language server per file extension and its LSP tool has no completion. Agents need an MCP tool or a hook as well.

## 1. State of tooling for C++26 reflection in editors

| Finding | Link | Mark |
|---|---|---|
| Upstream Clang lists P2996 reflection as "No". Annotations, contracts and `define_static_*` are "No" too. | [Clang C++ status](https://clang.llvm.org/cxx_status.html) | `source` |
| Upstream Clang lists expansion statements (P1306) as "Clang 23 (Partial)". | [Clang C++ status](https://clang.llvm.org/cxx_status.html) | `source` |
| Clang 23.1.5 ignores `-freflection` ("argument unused") and has no `<meta>` header. | spike: `clang++ -std=c++26 -freflection` | `measured` |
| Clang 23.1.5 rejects `pre(...)` on a declaration. It accepts `#embed`, a string as template argument, a `consteval` constructor and a simple `template for`. | spike: `c1.cpp`, `c2.cpp`, `c3.cpp` | `measured` |
| clangd 23.1.5 on a real lvgl26 file stops at the first error in the headers. No call of `lv::column` resolves after that. | spike: `clangd --check=small.cpp` | `measured` |
| The Bloomberg fork implements P2996 and calls itself experimental; crashes are expected; not for production. | [clang-p2996](https://github.com/bloomberg/clang-p2996) | `summary` |
| A clangd can be built from the fork. An open issue (April 2026) reports that code compiles while clangd crashes on it. No maintainer reply. | [issue 275](https://github.com/bloomberg/clang-p2996/issues/275) | `source` |
| An earlier clangd crash on a spliced expansion statement was closed in April 2025. | [issue 104](https://github.com/bloomberg/clang-p2996/issues?q=clangd) | `summary` |
| The fork needs libc++ and `<experimental/meta>`; GCC 16 ships only `<meta>`. Our headers would need porting to a second compiler. | [WrocPP: GCC 16 hands-on](https://wrocpp.github.io/posts/gcc-16-hands-on/) | `summary` |
| CLion 2026.2 adds highlighting and code analysis for reflection, only with GCC 16.1 or newer. Nova is the only engine. | [CLion 2026.2](https://blog.jetbrains.com/clion/2026/07/clion-2026-2) | `summary` |
| ReSharper C++ 2026.2 brings "initial support for C++26 reflection". | [ReSharper C++ 2026.2](https://blog.jetbrains.com/rscpp/2026/07/22/resharper-cpp-2026-2-cpp-26-reflection-ispc-language-support-and-more/) | `summary` |
| cpptools lists `#embed` and `c++26` modes and one GCC 16 crash fix. Nothing was found on reflection. | [cpptools changelog](https://github.com/microsoft/vscode-cpptools/releases) | `summary` |
| ccls is built on Clang's libraries, so it has the same limit as clangd. | [ccls](https://github.com/MaskRay/ccls) | `recalled` |
| GCC has no language server. GCC 16 lists no LSP item in its release notes. | [GCC 16 changes](https://gcc.gnu.org/gcc-16/changes.html) | `source` |
| GCC 16 removed `-fdiagnostics-format=json`. SARIF is the machine-readable format. | [GCC 16 changes](https://gcc.gnu.org/gcc-16/changes.html) | `source` |
| GCC 16.2 accepts only `sarif-file`, `sarif-stderr` and `text` for `-fdiagnostics-format=`. | spike: error text of `g++-16` | `measured` |
| `-fdiagnostics-add-output=sarif:file=x.sarif` writes SARIF 2.1.0 beside the normal text output. | spike: `bad.sarif` | `measured` |
| GCC 16 adds `-fdiagnostics-add-output=experimental-html`, nested logical locations and fix descriptions in SARIF. | [GCC 16 changes](https://gcc.gnu.org/gcc-16/changes.html) | `source` |
| C++ errors are hierarchical by default in GCC 16. `-fdiagnostics-plain-output` restores flat text. | [Red Hat on GCC 16](https://developers.redhat.com/articles/2026/04/28/gcc-16-improved-error-messages-sarif-output) | `source` |
| With `EXPERIMENTAL_SARIF_SOCKET` set, GCC sends each diagnostic as a JSON-RPC notification to a socket. Experimental; may be removed. | [GCC commit for PR115970](https://mirrors.lzu.edu.cn/git/gcc/commit/8a155c170b443e332b3db4922f0f0d588a17604f) | `summary` |
| `libgdiagnostics` gained 37 entry points. It emits diagnostics; it does not parse C++. | [GCC 16 changes](https://gcc.gnu.org/gcc-16/changes.html) | `source` |
| libgccjit generates code from an API. It has no C++ front end, so it cannot answer "what can I write here". | [libgccjit](https://gcc.gnu.org/onlinedocs/jit/) | `recalled` |
| GCC 16 adds no new `-fdump` option for tooling. A plugin could walk the AST, but must be built per GCC version. | [GCC 16 changes](https://gcc.gnu.org/gcc-16/changes.html) | `source` |

## 2. How other frameworks with generated APIs support editors

| Finding | Link | Mark |
|---|---|---|
| Qt ships `qmlls`: completion, lint warnings, go to definition, rename, formatting, documentation hints. It reads the CMake build directory. | [qmlls](https://doc.qt.io/qt-6/qtqml-tooling-qmlls.html) | `source` |
| Slint ships `slint-lsp`: diagnostics, completion, go to definition, formatting and live preview, for any LSP editor. | [slint-lsp README](https://github.com/slint-ui/slint/blob/master/tools/lsp/README.md) | `source` |
| Both servers serve the framework's own language, not C++. The C++ side stays with clangd. | [qmlls](https://doc.qt.io/qt-6/qtqml-tooling-qmlls.html) | `source` |
| A community Zephyr extension completes devicetree properties and "certain `DT_` macros in C", next to the C server. | [vscode-devicetree](https://github.com/trond-snekvik/vscode-devicetree) | `summary` |
| Several devicetree servers exist; one is built on a tree-sitter grammar. | [nvim-lspconfig dts_lsp](https://github.com/neovim/nvim-lspconfig) | `summary` |
| Laravel ide-helper generates `_ide_helper.php`: declarations with doc comments, installed as a dev dependency, only for the IDE. | [laravel-ide-helper](https://github.com/barryvdh/laravel-ide-helper) | `summary` |
| Python `.pyi` stubs and TypeScript `.d.ts` files are the same pattern: declarations for tools, never executed. | [typing stubs](https://typing.python.org/en/latest/spec/distributing.html) | `recalled` |
| Unreal Header Tool and protoc write plain generated headers at build time. The editor parses ordinary C++. | [protobuf C++ generated code](https://protobuf.dev/reference/cpp/cpp-generated/) | `recalled` |
| Tools that feed clangd from a GCC build filter GCC-only flags out of the compile commands (RIOT, Linux, Pigweed `pw_ide`). | [clangd: compile commands](https://clangd.llvm.org/design/compile-commands) | `summary` |
| No published example was found of stub headers that stand in for C++26 reflection code. | none | not found |

## 3. Stub headers for clangd

| Finding | Link | Mark |
|---|---|---|
| With a plain stub struct, clangd completes designated members inside `lv::label({.` through a call operator. | spike: `stub/drive.py` | `measured` |
| Each item carries the member's type and its `///` comment. Answer time: 8 to 11 ms. | spike: `stub/drive.py` | `measured` |
| Hover on `.align` shows the doc comment, the type and the default value. | spike: `stub/drive.py` | `measured` |
| Completion inside `"cen` returns an empty list. | spike: `stub/drive.py` | `measured` |
| A stub type with a `consteval` constructor that throws on an unknown name gives a clangd error on the right line. | spike: `ce/use.cpp` | `measured` |
| That error reads "call to consteval function ... is not a constant expression". Our text is at best in a note. | spike: `ce/use.cpp` | `measured` |
| `.clangd` supports `CompileFlags: Add, Remove, Compiler, CompilationDatabase`, and `If: PathMatch` per file. | [clangd config](https://clangd.llvm.org/config) | `source` |
| `Add: [-I stubs]` alone does not redirect: the flag is appended after the real include path. | spike: `redir/` | `measured` |
| `Remove: [-I<exact real path>]` plus `Add: [-I<stubs>]` redirects `<lv/lv.hpp>`. Zero errors. | spike: `redir/` | `measured` |
| `Remove` matches a prefix only when the value ends in `*`. `-I*/real` does not match. | [clangd config](https://clangd.llvm.org/config), spike | `measured` |
| A Clang VFS overlay (`-ivfsoverlay`) maps the real header file to the stub file. Zero errors. | spike: `redir/vfs.yaml` | `measured` |
| `Remove: [-freflection, -fconstexpr-ops-limit=*]` is needed; clangd warns about the second flag. | spike: `redir/` | `measured` |
| `CompilationDatabase:` can point clangd at a second, generated `compile_commands.json` for the editor. | [clangd config](https://clangd.llvm.org/config) | `source` |

Measured on 2026-11-16 with the stubs of `tools/editor` (the whole public API), clangd and clang++ 23.1.5, on a loaded machine:

| Finding | Link | Mark |
|---|---|---|
| A VFS overlay with a `directory-remap` of `include/lv` redirects every `<lv/...>` header. No `Remove: [-I...]` is needed. | `tools/editor/generate.py`, test `editor_support` | `measured` |
| `#embed "tokens.json"` goes through the overlay too: Clang embeds the file the overlay names. | spike, test `editor_support` | `measured` |
| `clangd --check` on the quick start: preamble built in 1.8 s, 0 errors. On the theme example: 0 errors. | test `editor_support` | `measured` |
| Over LSP: 2.4 s from opening the file to its diagnostics, about 100 ms for a completion list of 100 items, 1 ms for a hover. | scratch script that drives clangd | `measured` |
| One run answered the first hover after 41 s. It did not happen again; the machine was building at the time. | the same script | `measured` |
| `clang++ -fsyntax-only` with the stubs: quick start 1.5 s user (mp-units is most of it), theme example 0.3 s. | `/usr/bin/time -p` | `measured` |
| The generated header is 1.1 MB and 21,600 lines for LVGL v9.6.0; all stubs together are 1.2 MB. | `wc` | `measured` |
| A wrong quoted name gives two errors in a `constexpr` variable: one at the literal, one at the variable. | `tests/editor/wrong_enumerator.cpp` | `measured` |
| Clang warns (`-Wundefined-internal`) about a declared-only function of a template whose arguments have no linkage. A stub needs bodies. | `tools/editor/stubs/lv/stub.hpp` | `measured` |
| Clang only warns about designated initializers out of order. `-Werror=reorder-init-list` makes it an error, as in GCC. | `tests/editor/unknown_member.cpp` | `measured` |
| libc++ of LLVM 23 has no `<contracts>`. A fallback header behind `-idirafter` lets `examples/mcu/slice.cpp` parse. | `tools/editor/fallback/contracts` | `measured` |
| Linux arm64 in a container (`gcc:16`, Debian 13): Clang takes libstdc++ 14 from `/usr/lib/gcc`, not GCC 16's from `/usr/local`. | `clang++ -v` in the image of `.github/ci-image/editor` | `measured` |
| Debian's clang 19.1.7 there reads the stubs, but not mp-units 2.5.0: `kelvin` "does not satisfy `Unit`". The quick start fails; examples without mp-units pass. | the `editor_*` tests in that container | `measured` |
| Clang and clangd 21.1.8 from apt.llvm.org there: all eleven `editor_*` tests pass. | the same | `measured` |
| Not tried: Linux x86-64, clang 20 and 22, and any editor other than clangd itself. | none | not confirmed |

What breaks in a stub, and what does not:

| Construct | In a stub for Clang 23 | Mark |
|---|---|---|
| `lv::widget<"spinbox">` (string as template argument) | Works | `measured` |
| `#embed` | Works, as an extension with a warning | `measured` |
| `consteval` checks | Work; the message is poor | `measured` |
| `template for` | Simple case compiles; upstream calls it partial | `measured` |
| `pre(...)` contracts | Syntax error. The stub must leave them out | `measured` |
| `^^`, `<meta>`, `define_aggregate` | Not available. The stub must contain the result, not the reflection | `measured` |

Limits of the idea:

- The stub must mirror every public name: views, `lv::bind`, `lv::state`, `lv::mount`, tokens, units. Any gap is a false error in the editor.
- The project bans `#if`, so the switch is an include path or a VFS overlay, never a macro.
- Token structs depend on the user's JSON file. The stub generator must run in the user's build, per token file.
- Two LVGL versions give two stub sets. The generator must run per configuration.
- clangd's diagnostics come from the stub, not from our checks. Wrong values pass unless the stub repeats the check.

## 4. Completion inside a string literal

| Finding | Link | Mark |
|---|---|---|
| clangd returns nothing inside a string literal. | spike: `stub/drive.py` | `measured` |
| No clangd option or attribute for string completion was found. | [clangd config](https://clangd.llvm.org/config) | `source` |
| VS Code: "Multiple providers can be registered for a language." Groups of equal score are asked in turn. | [VS Code API](https://code.visualstudio.com/api/references/vscode-api) | `source` |
| VS Code asks only providers that registered the typed trigger character. A server can register `"`. | [VS Code API](https://code.visualstudio.com/api/references/vscode-api) | `source` |
| VS Code does not open suggestions in strings by default. Tailwind tells users to set `editor.quickSuggestions.strings`. | [Tailwind IntelliSense](https://github.com/tailwindlabs/tailwindcss-intellisense) | `source` |
| Tailwind finds its strings by attribute names and by function names matched with a regular expression. No AST of the host language. | [Tailwind IntelliSense](https://github.com/tailwindlabs/tailwindcss-intellisense) | `source` |
| Helix runs several servers per language. Diagnostics, code actions, completion and symbols "are merged together". | [Helix languages](https://docs.helix-editor.com/languages.html) | `source` |
| Helix can limit a server with `only-features = ["completion", "diagnostics"]`. | [Helix languages](https://docs.helix-editor.com/languages.html) | `source` |
| Zed takes a `language_servers` list per language, with a primary server. It does not document how results combine. | [Zed: configuring languages](https://zed.dev/docs/configuring-languages) | `source` |
| Neovim attaches several clients to one buffer and combines their diagnostics and completion items. | [Neovim LSP](https://neovim.io/doc/user/lsp.html) | `recalled` |
| LSP4IJ adds any server to a JetBrains IDE by command and file pattern. Coexistence with CLion's own C++ engine is not documented. | [LSP4IJ user-defined server](https://github.com/redhat-developer/lsp4ij/blob/main/docs/UserDefinedLanguageServer.md) | `source` |
| An LSP4IJ issue reports wrong results when a native plugin and an LSP server both complete. | [LSP4IJ issue 832](https://github.com/redhat-developer/lsp4ij/issues/832) | `summary` |
| Claude Code: "each file extension gets one server"; the first registered wins. | [Claude Code: plugin components](https://code.claude.com/docs/en/plugins/components.md) | `source` |
| The LSP specification itself says nothing about several servers. Merging is the client's business. | [LSP 3.17](https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/) | `recalled` |

Options for `.align = "cen|"`:

| Option | Works? | Note |
|---|---|---|
| (a) Constants in the stub (`lv::align::center`) | Yes in clangd | Contradicts the owner's choice of quoted names. Not proposed |
| (b) Own small server next to clangd | Yes | Needs the property left of `=`. A backward token scan finds `.align =` |
| (c) clangd extension or attribute | No | Nothing found |
| (d) tree-sitter for context | Yes | Finds the enclosing `lv::arc({` reliably; adds a dependency |
| (e) Stub repeats the name check in `consteval` | Diagnostics only | Error within milliseconds in clangd; no completion |

What option (b) must resolve: the widget (`lv::arc`), the member (`.align`, also nested `.style = {.bg_color = `), then the value type.

The nested and positional cases need care: `.flex_align = {"center", "center", "center"}` has three enumerator types by position.

## 5. Fast diagnostics from GCC

| Finding | Link | Mark |
|---|---|---|
| `#include <meta>` alone: 0.29 s. `#include <lv/lv.hpp>` alone: 3.3 s. | spike: `meta.cpp`, `inc.cpp` | `measured` |
| A five-widget tree with `-fsyntax-only`: 7.2 s. | spike: `small.cpp` | `measured` |
| `examples/theme/main.cpp` with `-fsyntax-only`: 11.4 s. | spike | `measured` |
| A precompiled header of `lv/lv.hpp` builds with `-freflection`: 5.0 s, 163 MB. | spike: `lvpch/` | `measured` |
| With that header: the small tree 4.5 s, the theme example 7.8 s. About one third less. | spike | `measured` |
| A file with an error costs the same as a clean one (4.5 s with the header). | spike: `bad.cpp` | `measured` |
| The printer demo screen takes about 176 s. No check-on-save loop can hide that. | `docs/research/cpp26.md` | `measured` |
| Modules: reflection in a module template fails in the importer; the eager table form passes. | `spikes/README.md` | `measured` |
| Modules were about 2.2 times slower than a plain include in GCC 16. | [Vittorio Romeo](https://isocpp.org/blog/2026/03/the-hidden-compile-time-cost-of-cpp26-reflection-vittorio-romeo) | `summary` |
| One wrong enumerator prints the same `lv:` error four times, then "dereferencing a null pointer". | spike: `bad.cpp` | `measured` |
| GCC reports the error at `include/lv/detail/api.hpp:53`. The user's line is only in "in 'constexpr' expansion of" notes. | spike: `bad.cpp` | `measured` |
| The text output names the exact literal: `bad.cpp:4:54`, with the caret under `"centre"`. | spike: `bad.cpp` | `measured` |
| The SARIF file does not contain that location. For `bad.cpp` it has only the include chain, line 1. | spike: `bad.sarif` | `measured` |
| LSP diagnostics carry `relatedInformation` and `codeDescription.href`; code actions can attach to a diagnostic. | [LSP 3.17](https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/) | `recalled` |

What follows for a check-on-save loop:

- Parse GCC's text output, not SARIF, until SARIF carries the constexpr expansion context. Use `-fdiagnostics-plain-output`.
- Move each `lv:` error to the outermost user location in its expansion notes. Drop duplicates and the null-pointer follow-up.
- Show the first sentence as the message. Put the "Its values: a, b, c" list into quick fixes, nearest spelling first.
- Keep a precompiled header per flag set, rebuilt when a header or the token file changes.
- Expect 4 to 8 s per save for a normal screen. This is a second tier, not typing feedback.

## 6. AI agents

| Finding | Link | Mark |
|---|---|---|
| Claude Code's LSP tool reports errors and warnings after each edit, and offers definition, references, type at a position and symbols. | [Claude Code tools](https://code.claude.com/docs/en/tools-reference.md) | `source` |
| That tool has no completion operation. An agent cannot ask a server "what can I write here". | [Claude Code tools](https://code.claude.com/docs/en/tools-reference.md) | `source` |
| A plugin declares a server in `.lsp.json` or under `lspServers`: `command`, `extensionToLanguage`, optional `args`, `env`, `diagnostics`. | [plugin manifest reference](https://code.claude.com/docs/en/plugins-reference) | `source` |
| The plugin does not install the binary; it must be on `PATH`. `bin/` in a plugin is on the Bash tool's `PATH`. | [plugin components](https://code.claude.com/docs/en/plugins/components.md) | `source` |
| Two servers for `.cpp` conflict: the first registered wins. Our server would compete with the official `clangd-lsp` plugin. | [plugin components](https://code.claude.com/docs/en/plugins/components.md) | `source` |
| Plugin language servers do not start in cloud sessions. | [code intelligence plugins](https://code.claude.com/docs/en/plugins/code-intelligence.md) | `source` |
| A plugin can also ship an MCP server (`.mcp.json`) and a `PostToolUse` hook on `Write|Edit`. | [plugin components](https://code.claude.com/docs/en/plugins/components.md) | `source` |
| Generic bridges expose a language server's hover, completion and diagnostics as MCP tools. | [lsp-mcp](https://github.com/tritlo/lsp-mcp) | `summary` |
| Svelte and Next.js ship an MCP tool that checks code or explains compile errors. | `docs/research/agent-docs.md` | `source` |
| lvgl26 has an MCP server in `include/lv/mcp.hpp`. It drives a running UI: `load_document`, `apply_patch`, `dump_tree`, `screenshot`. | repository | `source` |
| That server runs inside the application. A code checker is a different process; it should be a separate tool. | repository | `source` |
| No generated reference data (widgets, properties, value names as JSON) exists in the repository yet. | repository | `source` |

One engine can serve both. The core is two functions: `check(file)` and `complete(file, position)`.

The LSP front maps them to `publishDiagnostics` and `textDocument/completion`. The MCP front maps them to two tools.

A command-line front (`lv-check file.cpp`) serves hooks, CI and agents without MCP.

## 7. Libraries and hookups

| Finding | Link | Mark |
|---|---|---|
| `lsp-framework`: C++20, CMake, no other dependency, typed messages generated from the LSP meta model, stdio and TCP, MIT. | [lsp-framework](https://github.com/leon-bckl/lsp-framework) | `source` |
| `LspCpp`: an older C++ implementation, MIT. | [LspCpp](https://github.com/kuafuwang/LspCpp) | `summary` |
| `pygls`: Python; listed in Microsoft's SDK table. | [LSP SDKs](https://microsoft.github.io/language-server-protocol/implementors/sdks/) | `summary` |
| `vscode-languageserver` (Node) is Microsoft's own library; a VS Code extension needs Node anyway. | [vscode-languageserver-node](https://github.com/microsoft/vscode-languageserver-node) | `recalled` |
| `tower-lsp` continues as a community fork, `tower-lsp-server` (Rust). | [LSP SDKs](https://microsoft.github.io/language-server-protocol/implementors/sdks/) | `summary` |
| VS Code needs an extension: a manifest, a client that starts the server, and `"` as a trigger character. | [VS Code language features](https://code.visualstudio.com/api/language-extensions/programmatic-language-features) | `source` |
| Helix needs two entries in `languages.toml`. No extension. | [Helix languages](https://docs.helix-editor.com/languages.html) | `source` |
| Neovim needs a few lines: `vim.lsp.config` with `cmd`, `filetypes`, `root_markers`, then `vim.lsp.enable`. | [Neovim LSP](https://neovim.io/doc/user/lsp.html) | `recalled` |
| Zed needs an extension to register a new server for C++. | [Zed: configuring languages](https://zed.dev/docs/configuring-languages) | `recalled` |
| JetBrains: LSP4IJ takes a command and a file pattern, with an exportable template. | [LSP4IJ](https://github.com/redhat-developer/lsp4ij/blob/main/docs/UserDefinedLanguageServer.md) | `source` |
| Claude Code: a `.lsp.json` at the plugin root; unknown keys make the whole file invalid. | [plugin manifest reference](https://code.claude.com/docs/en/plugins-reference) | `source` |

A choice that the repository's own rules suggest: write the server in C++26 with GCC 16.

Then the server reads the reference data by the same reflection, and needs no generated JSON for the fixed part of the API.

Token names are the exception: they come from the user's JSON file, which the server can read directly at run time.

## Not confirmed

- Whether CLion 2026.2 completes members of a `define_aggregate` struct. The JetBrains pages returned 403; only search summaries were read.
- Whether CLion evaluates our `consteval` checks and `#embed`ded token files. Needs a hands-on test.
- Whether a clangd built from the Bloomberg fork parses our headers. The fork uses libc++ and other flags; not tried.
- Which upstream Clang release will get P2996. The status page says "No" and names no version.
- The Clang status page lists `#embed` (P1967) under "Clang 24"; the local Clang 23 accepts it as an extension. The two were not reconciled.
- Whether cpptools' own engine parses `^^` or `<meta>`. Nothing found either way.
- How Zed combines completions from two servers. Its page does not say.
- Whether LSP4IJ can run a second server on `.cpp` beside CLion Nova without conflicts.
- Neovim's merging of completion items across clients is from memory.
- Whether `EXPERIMENTAL_SARIF_SOCKET` carries the constexpr expansion notes. Not tried; the SARIF file does not.
- All timings were taken on a loaded machine. Repeat them on an idle one before quoting them.
- The stub spike used a hand-written stub of one widget. The real API (binds, nested styles, tokens, units) was not stubbed.
- Whether clangd shows the thrown `lv:` text of a failed `consteval` call in a note. Only the main message was read.
- No source was found that describes this stub technique for reflection code. We would be first.

## What it means for us

1. **Architecture A: generated stubs, clangd does the work.**
   - What: a small GCC-built tool prints plain C++20 headers from our own reflection; `.clangd` or an editor compile database points at them.
   - Delivers: member completion with docs (10 ms), hover, go to definition, in every clangd editor and in Claude Code's `clangd-lsp`.
   - Delivers partly: wrong quoted values as errors, if the stub repeats the name check in `consteval`. The message is clangd's, not ours.
   - Does not deliver: completion inside `"..."`, our long `lv:` messages, quick fixes.
   - Cost: the generator, a stub for every hand-written public type, a CMake step per LVGL version and token file.
   - Risk: stub and real API drift apart, and the editor then lies. A test must compile every example against both.

2. **Architecture B: our own `lv` language server beside clangd, on top of A.**
   - What: one small server with two tiers. Tier 1 answers from reference data in milliseconds. Tier 2 runs GCC on save.
   - Delivers: completion of quoted values and token names, hover on them, real `lv:` diagnostics at the user's line, quick fixes from the value lists.
   - Latency: tier 1 in milliseconds; tier 2 in 4 to 8 s per file with a precompiled header.
   - Editors: VS Code (own extension), Neovim and Helix (config only), Zed (extension). JetBrains through LSP4IJ is not confirmed.
   - Agents: Claude Code takes one server per extension. Either ours proxies clangd, or agents use the fronts in C.
   - Cost: the server, context detection for `.member = "..."`, a GCC output parser, one VS Code extension, user documentation per editor.
   - Risk: context detection in nested and positional cases; two servers reporting the same error twice.

3. **Architecture C: one engine, three fronts (LSP, MCP, command line).**
   - What: the engine of B as a library with `check(file)` and `complete(file, position)`. Fronts: the LSP server, an MCP server, `lv-check`.
   - Delivers for agents: "check this file" and "what can I write here" as MCP tools, and a `PostToolUse` hook that runs `lv-check`.
   - Delivers for people: the same as B. CI gets the same checker.
   - Cost: little above B, if the engine is a library from the start.
   - Risk: scope. Without A the member completion is missing for people; C complements A, it does not replace it.

4. **Before any of this: try CLion 2026.2 on the quick start (one hour).**
   - If its engine completes `lv::arc({.` from the real headers, CLion users need nothing from us.
   - It does not help VS Code, Neovim or agents, so A to C stay relevant.

5. **Suggested order.**
   - First A, because it is the cheapest route to member completion and it also improves agents through `clangd-lsp`.
   - Then the engine of C with the command line and MCP fronts, because agents gain most from the true `lv:` errors.
   - Then the LSP front and the VS Code extension for string completion.

6. **The spike that settles the biggest unknown.**
   - Question: can a reflection-driven generator produce stubs that make clangd accept the real examples with zero errors?
   - Do: generate stubs for the widgets, style and tokens used by `examples/quick_start/main.cpp` and `examples/theme/main.cpp`.
   - Pass: `clangd --check` reports zero errors on both, completion inside `lv::arc({.` lists the real members with docs.
   - Also measure: generator run time, stub size, clangd preamble time (the hand stub took 0.6 s in total).
   - Second spike, smaller: tier 1 context detection on those two files, as a script that prints the value type at each quoted literal.

7. **Decisions to ask the owner (not to assume).**
   - Is a different, shorter error text in the editor acceptable for wrong values, when it comes from the stub?
   - In which language may the server be written: C++26 with GCC 16 (reflection gives the data), or a scripting language reading generated JSON?
   - Which editors are in scope for the first release: VS Code only, or also Neovim, Helix, Zed and JetBrains?
   - May the plugin ship a server for `.cpp` in Claude Code, although it then replaces `clangd-lsp` for those files?
