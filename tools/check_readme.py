#!/usr/bin/env python3
"""Holds README.md to the code: its syntax, its names, its links and the facts it repeats.

    python3 tools/check_readme.py           checks README.md; exit 1 on a problem
    python3 tools/check_readme.py FILE      checks another file in the same way

The README is the front door, and a change of syntax or of a feature must reach it
(CLAUDE.md, "Definition of done for a feature"). This check finds what drifted:

1. An old form of a tree: children in brackets, `)[` or `lv::column[`, and `lv::state<int>`.
2. A name of LVGL's C API, `lv_...` or `LV_...`, outside a block after `<!-- c-api: shown -->`.
3. A relative link that leads nowhere.
4. "GCC X.Y or newer" with another version than the minimum in CMakeLists.txt.
5. "N of LVGL's M widget examples" or "N of LVGL's M examples" with other counts than
   examples/README.md.
6. "more than N compile-fail tests" when tests/fail/ holds N files or fewer.
7. A build line other than the one CLAUDE.md gives under "Build and test".

`python3 tools/docs.py check` holds the README's code blocks to the files they show. Uses the
standard library only.
"""

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
README = ROOT / "README.md"

OLD_FORMS = (
    (re.compile(r"\)\["), "children in brackets, `)[`: write them as arguments, `lv::group({...}, child)`"),
    (re.compile(r"\blv::\w+\["), "children in brackets, `lv::column[`: write them as arguments, `lv::column(child)`"),
    (re.compile(r"\blv::state<\s*int\s*>"), "`lv::state<int>`, which the library refuses: give the number a type"),
)
C_NAME = re.compile(r"\blv_[a-z0-9_]+|\bLV_[A-Z0-9_]+")
C_API_BLOCK = re.compile(r"<!-- c-api: shown -->\n```\w*\n.*?```\n", re.S)
LINK = re.compile(r"\]\(([^)#\s]+)(?:#[^)]*)?\)")
GCC_NEEDED = re.compile(r"GCC (\d+(?:\.\d+)+) or newer")
CMAKE_GCC = re.compile(r"CMAKE_CXX_COMPILER_VERSION VERSION_LESS (\d+(?:\.\d+)+)")
WIDGET_COUNT = re.compile(r"(\d+) of LVGL's (\d+) widget examples")
ALL_COUNT = re.compile(r"(\d+) of LVGL's (\d+) examples")
EXAMPLES_WIDGETS = re.compile(r"Of LVGL v[\d.]+'s (\d+) widget examples, (\d+) are ported")
EXAMPLES_ROW = re.compile(r"^\| LVGL [^|]*\| (\d+) \| (\d+) \|", re.M)
FAIL_COUNT = re.compile(r"[Mm]ore than (\d+) \[?compile-fail tests")
BUILD_LINE = re.compile(r"^cmake --preset .*$", re.M)


def old_form_problems(text: str) -> list[str]:
    return [f"an old form: {why}" for pattern, why in OLD_FORMS if pattern.search(text)]


def c_name_problems(text: str) -> list[str]:
    found = sorted(set(C_NAME.findall(C_API_BLOCK.sub("", text))))
    return [f"a name of LVGL's C API outside a block marked <!-- c-api: shown -->: {name}" for name in found]


def link_problems(text: str, folder: pathlib.Path) -> list[str]:
    return [f"the link to {target} leads nowhere" for target in LINK.findall(text)
            if "://" not in target and not target.startswith("mailto:") and not (folder / target).exists()]


def gcc_problems(text: str, cmake: str) -> list[str]:
    minimum = CMAKE_GCC.search(cmake)
    if minimum is None:
        return ["CMakeLists.txt names no minimum version of GCC"]
    return [f"it asks for GCC {version} or newer, and CMakeLists.txt for {minimum[1]}"
            for version in GCC_NEEDED.findall(text) if version != minimum[1]]


def example_counts(examples: str) -> dict[str, tuple[int, int]]:
    """What examples/README.md says: the widget examples, and all of LVGL's examples."""
    counts = {}
    if widgets := EXAMPLES_WIDGETS.search(examples):
        counts["widget"] = (int(widgets[2]), int(widgets[1]))
    rows = [(int(ported), int(of)) for ported, of in EXAMPLES_ROW.findall(examples)]
    if rows:
        counts["all"] = (sum(r[0] for r in rows), sum(r[1] for r in rows))
    return counts


def count_problems(text: str, examples: str) -> list[str]:
    counts, problems = example_counts(examples), []
    for kind, pattern in (("widget", WIDGET_COUNT), ("all", ALL_COUNT)):
        for ported, of in pattern.findall(text):
            if (int(ported), int(of)) != counts.get(kind):
                problems.append(f"it counts {ported} of {of} {'widget examples' if kind == 'widget' else 'examples'}, "
                                f"and examples/README.md {counts.get(kind)}")
    return problems


def fail_count_problems(text: str, tests: int) -> list[str]:
    return [f"it says more than {n} compile-fail tests, and tests/fail/ holds {tests}"
            for n in FAIL_COUNT.findall(text) if tests <= int(n)]


def build_line_problems(text: str, claude: str) -> list[str]:
    section = claude.split("## Build and test", 1)[-1]
    wanted = BUILD_LINE.search(section)
    if wanted is None:
        return ["CLAUDE.md has no build line under \"Build and test\""]
    return [f"its build line differs from CLAUDE.md's: {line}" for line in BUILD_LINE.findall(text) if line != wanted[0]]


def problems(text: str, folder: pathlib.Path = ROOT) -> list[str]:
    return (old_form_problems(text) + c_name_problems(text) + link_problems(text, folder) +
            gcc_problems(text, (ROOT / "CMakeLists.txt").read_text()) +
            count_problems(text, (ROOT / "examples" / "README.md").read_text()) +
            fail_count_problems(text, len(list((ROOT / "tests" / "fail").glob("*.cpp")))) +
            build_line_problems(text, (ROOT / "CLAUDE.md").read_text()))


def main(argv: list[str]) -> int:
    path = pathlib.Path(argv[1]) if len(argv) > 1 else README
    found = problems(path.read_text(), path.resolve().parent)
    for problem in found:
        print(f"{path.name}: {problem}")
    print(f"{path.name}: {len(found)} problems")
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
