#!/usr/bin/env python3
"""Puts the properties of every widget in the order the compiler wants: the id, the properties, the styles, the handlers.

    python3 tools/order_properties.py src/ main.cpp     rewrite the files and directories
    python3 tools/order_properties.py --check src/      list what would change; exit 1 if any

Designated initializers must follow the order of the members. lvgl26 puts them in four groups, each
in alphabetical order: `id`, then every other property (`size`, `text`, ...), then `style` and
`style_pressed`, ..., then the handlers (`on_clicked`, ...). The same rule is in include/lv/props.hpp
(`property_group`). A list that is out of order is a compile error, and this tool moves the properties.

The tool changes the order of the properties of a list and nothing else: it keeps the white space
between them, and each property keeps its own padding. A list that holds a comment between its
properties is left alone and reported: move it by hand. A list that is out of order for a test says "order on purpose" in a comment on its line, and is skipped.
A file may be C++ or Markdown with C++ blocks.
It reads: lv::<widget>({...}, lv::widget<"name">({...}), `.set({...})` and lv::props<"name"> p{...}.
Uses the standard library only.
"""

import argparse
import pathlib
import re
import sys

# The factories whose first argument is a props list: the widgets, and the boxes without a LVGL name.
FACTORIES = (
    "animimg arc arclabel bar box button buttonmatrix calendar canvas chart checkbox column dropdown group "
    "image imagebutton keyboard label led line list menu menu_cont menu_section menu_separator msgbox roller "
    "row scale slider spangroup spinbox spinner table tabview textarea tileview toggle widget win"
).split()
OPENING = re.compile(
    r"(?:\blv::(?:" + "|".join(FACTORIES) + r")(?:<\"[a-z_]+\">)?\(\s*"  # lv::label({
    r"|\.set(?:<\"[a-z_]+\">)?\(\s*"  # view.set({
    r"|\blv::props<\"[a-z_]+\">\s+\w+\s*)"  # lv::props<"arc"> p{
    r"\{"
)
DESIGNATOR = re.compile(r"\.([A-Za-z_]\w*)")
HANDLER_PREFIX = "on_"
ON_PURPOSE = "order on purpose"  # in a comment on the line of a list that is wrong for a test


def group_of(name: str) -> int:
    """0 for the id, 1 for properties, 2 for styles, 3 for handlers: property_group in include/lv/props.hpp."""
    if name == "id":
        return 0
    if name == "look" or name == "style" or name.startswith("style_"):
        return 2
    return 3 if name.startswith(HANDLER_PREFIX) else 1


def skip_literal(text: str, i: int) -> int:
    """The index after the string, character literal or comment that starts at i, or i if none does."""
    c = text[i]
    if c in "\"'":
        j = i + 1
        while j < len(text) and text[j] != c:
            j += 2 if text[j] == "\\" else 1
        return j + 1
    if text.startswith("//", i):
        j = text.find("\n", i)
        return len(text) if j < 0 else j
    if text.startswith("/*", i):
        j = text.find("*/", i)
        return len(text) if j < 0 else j + 2
    return i


def close_of(text: str, start: int) -> int:
    """The index of the brace that closes the one at `start`, or -1."""
    depth = 0
    i = start
    while i < len(text):
        j = skip_literal(text, i)
        if j != i:
            i = j
            continue
        if text[i] in "{([":
            depth += 1
        elif text[i] in "})]":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def split_top(inner: str) -> list[str]:
    """The pieces of a braced list between its commas at depth 0 (and the text after the last one)."""
    pieces, depth, begin, i = [], 0, 0, 0
    while i < len(inner):
        j = skip_literal(inner, i)
        if j != i:
            i = j
            continue
        if inner[i] in "{([":
            depth += 1
        elif inner[i] in "})]":
            depth -= 1
        elif inner[i] == "," and depth == 0:
            pieces.append(inner[begin:i])
            begin = i + 1
        i += 1
    pieces.append(inner[begin:])
    return pieces


def first_designator(piece: str) -> re.Match | None:
    """The `.name` that a piece starts with, after white space and comments."""
    i = 0
    while i < len(piece):
        if piece[i].isspace():
            i += 1
        elif piece.startswith(("//", "/*"), i):
            i = skip_literal(piece, i)
        else:
            break
    return DESIGNATOR.match(piece, i)


def designators(pieces: list[str]) -> list[tuple[str, str, str, str]] | None:
    """(name, lead, body, trail) of each property of a list, or None if it is no list of designators.

    A piece that starts with no `.name` is the rest of the one before it: a comma in a template.
    """
    found: list[list[str]] = []
    for piece in pieces:
        stripped = piece.strip()
        if not stripped:
            continue
        if first_designator(stripped):
            found.append([piece])
        elif found:
            found[-1].append(piece)
        else:
            return None
    result = []
    for parts in found:
        text = ",".join(parts)
        body = text.strip()
        lead = text[: len(text) - len(text.lstrip())]
        trail = text[len(text.rstrip()):]
        name = first_designator(body).group(1)  # type: ignore[union-attr]
        result.append((name, lead, body, trail))
    return result


def has_comment(body: str) -> bool:
    i = 0
    while i < len(body):
        if body.startswith(("//", "/*"), i):
            return True
        j = skip_literal(body, i)
        i = j if j != i else i + 1
    return False


def reorder_list(inner: str) -> tuple[str, str]:
    """The reordered text of the inside of a braced list, and "same", "changed" or "comment"."""
    pieces = split_top(inner)
    items = designators(pieces)
    if not items or len(items) < 2:
        return inner, "same"
    wanted = sorted(items, key=lambda it: (group_of(it[0]), it[0]))
    if [it[0] for it in wanted] == [it[0] for it in items]:
        return inner, "same"
    if any(has_comment(it[2]) for it in items):
        return inner, "comment"
    names = [it[0] for it in items]
    if len(set(names)) != len(names):
        return inner, "same"
    trailing_comma = not pieces[-1].strip()
    rebuilt = [items[i][1] + wanted[i][2] + items[i][3] for i in range(len(items))]
    out = ",".join(rebuilt)
    if trailing_comma:
        out += "," + pieces[-1]
    return out, "changed"


def reorder_text(text: str) -> tuple[str, int, list[int]]:
    """The text with every props list in order, the number of lists changed, and the lines left for a hand."""
    out, last, changed, manual = [], 0, 0, []
    position = 0
    while True:
        found = OPENING.search(text, position)
        if not found:
            break
        brace = found.end() - 1
        end = close_of(text, brace)
        if end < 0:
            position = found.end()
            continue
        line_start = text.rfind("\n", 0, brace) + 1
        line_end = text.find("\n", end)
        if ON_PURPOSE in text[line_start : line_end if line_end >= 0 else len(text)]:
            position = found.end()
            continue
        inner = text[brace + 1 : end]
        # The nested lists come first: a style is no props list, but a child inside a handler may be.
        new_inner, state = reorder_list(inner)
        if state == "changed":
            out.append(text[last : brace + 1])
            out.append(new_inner)
            last = end
            changed += 1
        elif state == "comment":
            manual.append(text.count("\n", 0, brace) + 1)
        position = found.end()
    out.append(text[last:])
    return "".join(out), changed, manual


def files_of(paths: list[str]) -> list[pathlib.Path]:
    suffixes = {".cpp", ".hpp", ".h", ".md"}
    found: list[pathlib.Path] = []
    for p in map(pathlib.Path, paths):
        if p.is_dir():
            found += sorted(f for f in p.rglob("*") if f.suffix in suffixes and f.is_file())
        elif p.suffix in suffixes:
            found.append(p)
    return found


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("paths", nargs="+", help="files, or directories searched for .cpp, .hpp and .md")
    parser.add_argument("--check", action="store_true", help="change nothing; exit 1 if a file would change")
    args = parser.parse_args(argv)
    status = 0
    for path in files_of(args.paths):
        text = path.read_text()
        new, changed, manual = reorder_text(text)
        for line in manual:
            print(f"{path}:{line}: a comment between the properties: put them in order by hand", file=sys.stderr)
            status = 1
        if changed:
            print(f"{path}: {changed} list(s) {'to reorder' if args.check else 'reordered'}")
            status = 1
            if not args.check:
                path.write_text(new)
    return status if args.check or status else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
