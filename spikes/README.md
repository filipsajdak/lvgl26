# Risk spikes (step 1)

Throwaway experiments. They are not built by CMake. GCC 16.2.0, macOS arm64, 2026-11-13.

| Spike | Files | Result |
|---|---|---|
| Reflect the real `lvgl.h` and splice a setter from its name | `reflect_by_name.cpp` | Pass on v9.6.0 (1978 `lv_*` functions) and on the v10 pin (1927). Compiles in 1.8 s |
| Typedef'd C enums | `reflect_by_name.cpp` | Pass, but compare types through `dealias` |
| Reflection in a template of a module, instantiated by the importer | `module_lazy.cppm`, `module_lazy_use.cpp` | Fail. `members_of(^^::)` runs in the importer, which does not see the LVGL functions of the module's global fragment |
| Reflection resolved while the module itself is compiled | `module_eager.cppm`, `module_eager_use.cpp` | Pass |
| Table of reflections built in the module, importer picks from it by name and splices | `module_table.cppm`, `module_table_use.cpp` | Pass. `set<"text">(...)` works from an importer |
| `#embed` a DTCG file and read it in `consteval` | `embed_tokens.cpp`, `tokens.json` | Pass |
| arm-none-eabi cross build | none | Not run. No `arm-none-eabi-g++` on this machine |

## Design consequences

1. The global-namespace scan is eager. Each widget gets a table of its LVGL functions, built with
   `define_static_array` where `lvgl.h` is visible. User-facing templates search that table by name
   and splice. They never scan the global namespace themselves.
2. In a file that mixes both, standard `#include`s come before `import lv;`. The other order fails
   in GCC 16 with redefinition errors.
3. The header build and the module build share the same eager code, so both stay equivalent.
