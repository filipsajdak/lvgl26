#!/usr/bin/env python3
"""Moves the children of a tree into the call, and writes a trailing comma where a property list is laid out one per line.

    python3 tools/call_form.py src/ main.cpp          rewrite the files and directories
    python3 tools/call_form.py --check src/           list what would change; exit 1 if any

A tree can be written two ways: `lv::group({...})[child, child]` and `lv::group({...}, child, child)`. The call
form lets clang-format lay a tree out as nested blocks (tools/format_ui.py), and this tool turns the first into
the second: `X(props)[kids]` becomes `X(props, kids)`, `X[kids]` becomes `X(kids)`, and the same for `lv::each`,
`lv::shown` and `lv::when`. A `lv::when(flag, {options})[tree]` has no call form and is reported.

It then puts a trailing comma into every designated-initializer list that has three properties or more, or a
property that is itself a list of properties (`.style = {...}`): clang-format lays those out one property per
line. A shorter list keeps its line. A list with a comment inside is reported, not changed. Run
tools/format_ui.py after it. Uses the standard library only.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from order_properties import DESIGNATOR, FACTORIES, close_of, has_comment, split_top  # noqa: E402

CALLEES = sorted(set(FACTORIES) | {"each", "shown", "when"}, key=len, reverse=True)
CALLEE = re.compile(r"\blv::(" + "|".join(CALLEES) + r")(<\"[a-z_]+\">)?(?=[(\[])")
SUFFIXES = {".cpp", ".hpp", ".md"}


def skip_space(text: str, i: int) -> int:
    while i < len(text) and text[i].isspace():
        i += 1
    return i


def children_into_call(text: str) -> tuple[str, int, list[int]]:
    """The text with every `X(...)[kids]` and `X[kids]` of a tree as `X(..., kids)`; the number changed and the lines left."""
    count, left, position = 0, [], 0
    while True:
        found = CALLEE.search(text, position)
        if not found:
            return text, count, left
        head = found.group(0)
        at = found.end()
        if text[at] == "(":
            close = close_of(text, at)
            bracket = skip_space(text, close + 1) if close >= 0 else -1
            if close < 0 or bracket >= len(text) or text[bracket] != "[":
                position = found.end()
                continue
            args = text[at + 1 : close]
            if found.group(1) == "when" and len(split_top(args)) > 1:
                left.append(text.count("\n", 0, found.start()) + 1)
                position = found.end()
                continue
        else:
            bracket, args = at, ""
        end = close_of(text, bracket)
        if end < 0:
            position = found.end()
            continue
        kids = text[bracket + 1 : end].strip()
        inner = (args.rstrip() + ", " if args.strip() else "") + kids
        text = text[: found.start()] + head + "(" + inner + ")" + text[end + 1 :]
        count += 1
        position = found.start()


OPENER = re.compile(r"([(=,{]\s*)\{(?=\s*\.[A-Za-z_]\w*\s*=)")


def wants_comma(items: list[str]) -> bool:
    """Whether a list of properties is laid out one per line: three or more, or one that holds a list of properties."""
    if len(items) >= 3:
        return True
    return any(re.match(r"\s*\.\w+\s*=\s*\{\s*\.\w+", item) for item in items)


def trailing_commas(text: str) -> tuple[str, int, list[int]]:
    """A trailing comma in every list of properties that wants one; the number changed and the lines left."""
    count, left, position = 0, [], 0
    while True:
        found = OPENER.search(text, position)
        if not found:
            return text, count, left
        brace = found.end() - 1
        end = close_of(text, brace)
        position = found.end()
        if end < 0:
            continue
        inner = text[brace + 1 : end]
        pieces = split_top(inner)
        items = [p for p in pieces if p.strip()]
        if not items or not DESIGNATOR.match(items[0].strip()) or not wants_comma(items):
            continue
        if not pieces[-1].strip():  # it ends with a comma already
            continue
        if has_comment(inner):
            left.append(text.count("\n", 0, brace) + 1)
            continue
        body_end = brace + 1 + len(inner.rstrip())
        text = text[:body_end] + "," + text[body_end:]
        count += 1


def convert(text: str) -> tuple[str, int, list[int]]:
    text, moved, left_a = children_into_call(text)
    text, commas, left_b = trailing_commas(text)
    return text, moved + commas, sorted(set(left_a + left_b))


def files_of(paths: list[str]) -> list[pathlib.Path]:
    found: list[pathlib.Path] = []
    for p in map(pathlib.Path, paths):
        found += sorted(f for f in p.rglob("*") if f.suffix in SUFFIXES and f.is_file()) if p.is_dir() else [p]
    return found


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("paths", nargs="+", help="files, or directories searched for .cpp, .hpp and .md")
    parser.add_argument("--check", action="store_true", help="change nothing; exit 1 if a file would change")
    args = parser.parse_args(argv)
    status = 0
    for path in files_of(args.paths):
        text = path.read_text()
        new, changed, left = convert(text)
        for line in left:
            print(f"{path}:{line}: left for a hand (a comment inside, or a call form that does not exist)", file=sys.stderr)
        if changed:
            print(f"{path}: {changed} change(s)")
            status = 1
            if not args.check:
                path.write_text(new)
    return status if args.check else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
