#!/usr/bin/env python3
"""Counts the words of a snippet of code, and checks the counts a docs page states.

    python3 tools/code_words.py examples/looks/screen.hpp tree     prints the count
    python3 tools/code_words.py --check docs/looks.md              fails when a stated count is stale

A word is an identifier, a number or a string literal. Punctuation and comments are not
counted, so a line break or a brace costs nothing. A page states a count in a comment:

    <!-- words: examples/looks/screen.hpp tree 61 -->

A snippet is the code between `//! [name]` lines, as in docs/README.md. Uses the standard
library only.
"""

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
WORD = re.compile(r'"(?:[^"\\]|\\.)*"|[A-Za-z_][A-Za-z_0-9]*|\d[\w.]*')
STATED = re.compile(r"<!-- words: (\S+) (\S+) (\d+) -->")
MARK = "//! ["


def snippet(path: pathlib.Path, name: str) -> str:
    """The lines between `//! [name]` markers; an error names the file when there are none."""
    lines, inside = [], False
    for line in path.read_text().splitlines():
        if line.strip() == f"{MARK}{name}]":
            inside = not inside
        elif inside:
            lines.append(line)
    if not lines:
        raise SystemExit(f"{path}: no snippet called {name}")
    return "\n".join(lines)


def count(code: str) -> int:
    """Words of `code`, without comments."""
    plain = re.sub(r"//[^\n]*", "", code)
    return len(WORD.findall(plain))


def check(page: pathlib.Path) -> int:
    stale = 0
    for match in STATED.finditer(page.read_text()):
        file, name, stated = match.group(1), match.group(2), int(match.group(3))
        found = count(snippet(ROOT / file, name))
        if found != stated:
            print(f"{page}: {file} {name} has {found} words, the page says {stated}")
            stale += 1
    return 1 if stale else 0


def main(argv: list[str]) -> int:
    if len(argv) == 2 and argv[0] == "--check":
        return check(pathlib.Path(argv[1]))
    if len(argv) == 2:
        print(count(snippet(ROOT / argv[0], argv[1])))
        return 0
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
