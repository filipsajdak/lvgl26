#!/usr/bin/env python3
"""Checks that the examples name nothing of LVGL's C API, and include what they use.

    python3 tools/check_examples.py

An example shows one level of abstraction: lvgl26. A call such as lv_init(), a type
such as lv_obj_t or an enumerator such as LV_ALIGN_CENTER in an example means that
the C++ form is missing, or was not used. A line that goes through the door on
purpose says so with the comment `// C API`.

An example also includes the header of each name of lvgl26 it uses, and no opt-in header it
does not use (tools/check_includes.py --exact): what it includes is what it teaches. The quick
start includes <lv/all.hpp>, as the documentation does.

Uses the standard library only. Exits with 1 and a list of the lines if any.
"""

import pathlib
import re
import sys

import check_includes

EXAMPLES = pathlib.Path(__file__).resolve().parent.parent / "examples"
# The one example that shows <lv/all.hpp>.
EVERYTHING_ALLOWED = {EXAMPLES / "quick_start" / "main.cpp"}
C_NAME = re.compile(r"\blv_[a-z0-9_]+\s*\(|\blv_[a-z0-9_]+_t\b|\bLV_[A-Z0-9_]+")


def problems(path: pathlib.Path) -> list[str]:
    found = []
    for number, line in enumerate(path.read_text().splitlines(), 1):
        code = line.split("//")[0]
        name = C_NAME.search(code)
        if name and "// C API" not in line:
            found.append(f"{path.relative_to(EXAMPLES.parent)}:{number}: {name.group(0)}")
    return found


def main() -> int:
    files = sorted(p for p in EXAMPLES.rglob("*") if p.suffix in (".cpp", ".hpp"))
    table, _ = check_includes.load_table()
    found = [problem for path in files for problem in problems(path)]
    found += [problem for path in files
              for problem in check_includes.problems(path, table, exact=path not in EVERYTHING_ALLOWED)]
    for problem in found:
        print(problem)
    print(f"{len(files)} files, {len(found)} C names")
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main())
