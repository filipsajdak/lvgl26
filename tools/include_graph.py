#!/usr/bin/env python3
"""The files that a program includes: one resolution of `#include` for the checks of a build.

    tools/check_includes.py, tools/check_fonts.py (and the layout check that will follow)

A program reaches its own headers in two ways: beside the file that names them
(`#include "design.hpp"`), and through an include directory of its target
(`#include "design/design.hpp"` or `<design/design.hpp>` with `target_include_directories`).
A check that follows only the first way never sees what the second reaches, and a check that
looks for something in the files it saw (a font, a header) then passes where it should not. So
this module follows both, and says which includes it could not resolve:

    include_graph.files_of(sources, include_dirs)  ->  (files, unresolved)

`unresolved` holds `(file, line, name)` for each quoted include that no directory has. A file
of lvgl26 (`<lv/...>`), of LVGL and of the standard library is not the program's and is left
out without a word; an angle include is followed only where an include directory has it.
`warn(unresolved)` prints them, one line each, so that a check never reads a missing file as an
empty one. Uses the standard library only.
"""

import pathlib
import re
import sys

INCLUDE = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]', re.M)
COMMENT = re.compile(r"//[^\n]*|/\*.*?\*/", re.S)
# Headers that are never the program's own.
NOT_THE_PROGRAMS = ("lv/", "lvgl/")


def without_comments(text: str) -> str:
    """The text without its comments, with the same lines."""
    return COMMENT.sub(lambda m: re.sub(r"[^\n]", " ", m.group(0)), text)


def includes_of(text: str) -> list[tuple[int, str, str]]:
    """Each include of `text` outside comments: its line, `<` or `"`, and its name."""
    code = without_comments(text)
    return [(code.count("\n", 0, m.start()) + 1, m.group(1), m.group(2)) for m in INCLUDE.finditer(code)]


def resolve(name: str, kind: str, from_file: pathlib.Path, include_dirs: list[pathlib.Path]) -> pathlib.Path | None:
    """The file that `#include kind name` names in `from_file`, or None."""
    candidates = [from_file.parent] if kind == '"' else []
    for directory in candidates + list(include_dirs):
        found = directory / name
        if found.is_file():
            return found.resolve()
    return None


def files_of(sources: list[pathlib.Path], include_dirs: list[pathlib.Path]) -> tuple[list[pathlib.Path], list[tuple[pathlib.Path, int, str]]]:
    """`sources` and every file of the program they include, each once, and the includes not resolved."""
    seen: dict[pathlib.Path, None] = {}
    unresolved: list[tuple[pathlib.Path, int, str]] = []
    todo = [s.resolve() for s in sources]
    while todo:
        path = todo.pop(0)
        if path in seen or not path.is_file():
            continue
        seen[path] = None
        for line, kind, name in includes_of(path.read_text(errors="replace")):
            if name.startswith(NOT_THE_PROGRAMS):
                continue
            found = resolve(name, kind, path, include_dirs)
            if found is not None:
                todo.append(found)
            elif kind == '"':
                unresolved.append((path, line, name))
    return list(seen), unresolved


def warn(unresolved: list[tuple[pathlib.Path, int, str]], tool: str) -> None:
    """One line on stderr for each include that no directory resolved."""
    for path, line, name in unresolved:
        print(
            f'{path}:{line}: warning: {tool} could not find the include "{name}" beside the file or in an include '
            "directory of the target, so what it holds is not checked",
            file=sys.stderr,
        )


def split_include_dirs(args: list[str]) -> tuple[list[pathlib.Path], list[str]]:
    """The `-I dir` and `-Idir` arguments of a command line, and the rest."""
    dirs: list[pathlib.Path] = []
    rest: list[str] = []
    it = iter(args)
    for arg in it:
        if arg == "-I":
            dirs.append(pathlib.Path(next(it)))
        elif arg.startswith("-I"):
            dirs.append(pathlib.Path(arg[2:]))
        else:
            rest.append(arg)
    return dirs, rest
