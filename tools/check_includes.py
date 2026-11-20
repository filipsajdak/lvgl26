#!/usr/bin/env python3
"""Checks that a file includes the header of each name of lvgl26 it uses.

    python3 tools/check_includes.py main.cpp parts.hpp ...
    python3 tools/check_includes.py --exact examples/lists/main.cpp

<lv/lv.hpp> is the core. A name of an opt-in header (lv::list, lv::look, lv::theme, ...) needs
that header, or <lv/all.hpp>. Which header declares which name is docs/reference/headers.json,
printed by reflection (tools/reference/headers.cpp). A header counts when the file includes it,
when one of our headers that the file includes includes it, or when a header of the user's
own, included with quotes, does. Comments and strings are not read.

--build STAMP DEPFILE SOURCE is the form a build runs (cmake/check_includes.cmake): it checks
SOURCE and the headers of the program it includes with quotes, writes STAMP when all is well,
and writes DEPFILE, so that the build checks the file again only when one of them changes.

--page FILE writes the table of names and headers into FILE (docs/which-header.md) between its
markers; --check-page FILE fails when the table there is stale.

--exact also reports an opt-in header that the file includes and does not use, and
<lv/all.hpp>: what an example includes is what it shows.

Prints "file:line: lv::list needs #include <lv/list.hpp>" for each name, and exits with 1 if
there is any. Uses the standard library only.
"""

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
TABLE = ROOT / "docs" / "reference" / "headers.json"
HEADERS = ROOT / "include"
EVERYTHING = "lv/all.hpp"
CORE = "lv/lv.hpp"

INCLUDE = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]', re.M)
NAME = re.compile(r"\blv::([A-Za-z_]\w*)")
# lv::bind of a pointer to a member binds a field of a row: lv/list.hpp.
ROW_BIND = re.compile(r"\blv::bind\s*\(\s*&")
NOT_CODE = re.compile(r'//[^\n]*|/\*.*?\*/|R"(\w*)\((?:.|\n)*?\)\1"|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', re.S)


def blank(match: re.Match) -> str:
    """The text of a comment or a string, with every character but a line break as a space."""
    return re.sub(r"[^\n]", " ", match.group(0))


COMMENT = re.compile(r"//[^\n]*|/\*.*?\*/", re.S)


def without_comments(text: str) -> str:
    """The text without its comments, with the same lines: where #include lines are read."""
    return COMMENT.sub(blank, text)


def code_of(text: str) -> str:
    """The text without its comments and strings, with the same lines."""
    return NOT_CODE.sub(blank, text)


def our_includes(header: str) -> set[str]:
    """The headers of lvgl26 that `header` includes, directly or not."""
    seen: set[str] = set()
    todo = [header]
    while todo:
        path = HEADERS / todo.pop()
        if not path.exists():
            continue
        for _, name in INCLUDE.findall(path.read_text()):
            if name.startswith("lv/") and name not in seen:
                seen.add(name)
                todo.append(name)
    return seen


def load_table() -> tuple[dict[str, list[str]], set[str]]:
    """The opt-in names, each with the headers that declare it, and the names of the core."""
    by_header = json.loads(TABLE.read_text())
    core = our_includes(CORE) | {CORE}
    core_names = {name for header, names in by_header.items() if header in core for name in names}
    opt_in: dict[str, list[str]] = {}
    for header, names in by_header.items():
        for name in names:
            if header not in core and name not in core_names:
                opt_in.setdefault(name, []).append(header)
    return opt_in, core_names


def provided(path: pathlib.Path, seen: set[pathlib.Path] | None = None) -> set[str]:
    """Every header of lvgl26 that `path` makes available, through any include."""
    seen = set() if seen is None else seen
    if path in seen or not path.exists():
        return set()
    seen.add(path)
    found: set[str] = set()
    for kind, name in INCLUDE.findall(without_comments(path.read_text())):
        if kind == "<" and name.startswith("lv/"):
            found |= {name} | our_includes(name)
        elif kind == '"':
            found |= provided(path.parent / name, seen)
    if EVERYTHING in found:
        found |= our_includes(EVERYTHING)
    return found


def direct_includes(text: str) -> list[tuple[int, str]]:
    """The <lv/...> headers a file names itself, with their lines."""
    lines = []
    for match in INCLUDE.finditer(without_comments(text)):
        if match.group(1) == "<" and match.group(2).startswith("lv/"):
            lines.append((text.count("\n", 0, match.start()) + 1, match.group(2)))
    return lines


def problems(path: pathlib.Path, table: dict[str, list[str]], exact: bool = False) -> list[str]:
    text = path.read_text()
    code = code_of(text)
    have = provided(path)
    needed: dict[str, int] = {}
    found = []

    def need(header_choices: list[str], name: str, line: int) -> None:
        for header in header_choices:
            needed.setdefault(header, line)
        if not any(h in have for h in header_choices):
            found.append(f"{path}:{line}: lv::{name} needs #include <{header_choices[0]}>")

    for match in NAME.finditer(code):
        line = code.count("\n", 0, match.start()) + 1
        if match.group(1) in table:
            need(table[match.group(1)], match.group(1), line)
    for match in ROW_BIND.finditer(code):
        need(["lv/list.hpp"], "bind(&row::field)", code.count("\n", 0, match.start()) + 1)
    if exact:
        for line, header in direct_includes(text):
            if header == EVERYTHING:
                found.append(f"{path}:{line}: includes <{EVERYTHING}>; an example includes the headers it uses")
            elif header in {h for hs in table.values() for h in hs} and header not in needed:
                found.append(f"{path}:{line}: <{header}> is included and not used")
    return found


BEGIN = "<!-- headers: begin -->"
END = "<!-- headers: end -->"


def page_table(table: dict[str, list[str]]) -> str:
    """The table of the page: each opt-in header and the names it declares, in alphabetical order."""
    by_header: dict[str, list[str]] = {}
    for name, headers in table.items():
        for header in headers:
            by_header.setdefault(header, []).append(name)
    rows = ["| Header | Names |", "|---|---|"]
    for header in sorted(by_header):
        names = ", ".join(f"`lv::{n}`" for n in sorted(by_header[header]))
        rows.append(f"| `<{header}>` | {names} |")
    rows.append("| `<lv/list.hpp>` | `lv::bind(&row::field)`: a field of a row |")
    return "\n".join(rows)


def with_table(text: str, table: dict[str, list[str]]) -> str:
    start, end = text.index(BEGIN) + len(BEGIN), text.index(END)
    return text[:start] + "\n" + page_table(table) + "\n" + text[end:]


def page(argv: list[str]) -> int:
    path = pathlib.Path(argv[1])
    table, _ = load_table()
    text = path.read_text()
    if argv[0] == "--page":
        path.write_text(with_table(text, table))
        return 0
    if with_table(text, table) != text:
        print(f"{path}: the table is stale. Write it: python3 tools/check_includes.py --page {path}")
        return 1
    return 0


def followed(path: pathlib.Path) -> list[pathlib.Path]:
    """`path` and every file of the program it includes with quotes, each once."""
    seen: list[pathlib.Path] = []
    todo = [path.resolve()]
    while todo:
        current = todo.pop()
        if current in seen or not current.exists():
            continue
        seen.append(current)
        for kind, name in INCLUDE.findall(without_comments(current.read_text())):
            if kind == '"':
                todo.append((current.parent / name).resolve())
    return seen


def escaped(path: pathlib.Path) -> str:
    """A path as a depfile writes it."""
    return str(path).replace(" ", "\\ ")


def build(argv: list[str]) -> int:
    stamp, depfile, source = (pathlib.Path(a) for a in argv[:3])
    table, _ = load_table()
    files = followed(source)
    found = [problem for path in files for problem in problems(path, table)]
    inputs = files + [TABLE, pathlib.Path(__file__).resolve()]
    depfile.parent.mkdir(parents=True, exist_ok=True)
    depfile.write_text(f"{escaped(stamp)}: " + " ".join(escaped(f) for f in inputs) + "\n")
    for problem in found:
        print(problem)
    if found:
        stamp.unlink(missing_ok=True)
        return 1
    stamp.write_text("")
    return 0


def main(argv: list[str]) -> int:
    if argv and argv[0] in ("--page", "--check-page"):
        return page(argv)
    if argv and argv[0] == "--build":
        return build(argv[1:])
    exact = "--exact" in argv
    files = [pathlib.Path(a) for a in argv if a != "--exact"]
    table, _ = load_table()
    found = [problem for path in files for problem in problems(path, table, exact)]
    for problem in found:
        print(problem)
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
