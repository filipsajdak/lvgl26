#!/usr/bin/env python3
"""Formats the UI code of lvgl26: clang-format with the .clang-format of the repository, and a pass after it.

    python3 tools/format_ui.py examples/lists/main.cpp     rewrite the file
    python3 tools/format_ui.py --check examples            list the files that would change; exit 1 if any

A tree is written with a trailing comma in every property list that is to be laid out one property per
line (`lv::label({.id = "a", .text = "x",})`), and without it where the list stays on one line. clang-format
does the layout from that comma. It does not document this: tests/test_format_ui.py pins what it does, so
that a new clang-format shows a change. After clang-format, a pass joins `.style =` and the `{` that it
puts on the next line into `.style = {`, and moves the block left; another gives such a line one space
before the `=` and its body one step in; a third writes `b * px` back where clang-format reads it as a
pointer (`b* px`). They change white space only, and the tool refuses a result
whose text differs from the file's apart from white space.

Needs clang-format 21 to 23 (CLANG_FORMAT names the program). Uses the standard library only.
"""

from __future__ import annotations

import argparse
import os
import pathlib
import re
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
STYLE = ROOT / ".clang-format"
SUFFIXES = {".cpp", ".hpp"}
# The versions whose output the tests pin.
VERSIONS = (21, 22, 23)


def clang_format_program() -> str | None:
    """The clang-format to run, or None: $CLANG_FORMAT, else clang-format-21 to 23, else clang-format."""
    named = os.environ.get("CLANG_FORMAT")
    for name in ([named] if named else []) + [f"clang-format-{v}" for v in VERSIONS] + ["clang-format"]:
        found = shutil.which(name)
        if found:
            return found
    return None


def major_version(program: str) -> int:
    text = subprocess.run([program, "--version"], capture_output=True, text=True, check=True).stdout
    return int(re.search(r"version (\d+)", text).group(1))  # type: ignore[union-attr]


def run_clang_format(text: str, name: str, program: str) -> str:
    done = subprocess.run([program, f"--style=file:{STYLE}", f"--assume-filename={name}"], input=text,
                          capture_output=True, text=True)
    if done.returncode != 0:
        raise RuntimeError(f"clang-format failed on {name}: {done.stderr.strip()}")
    return done.stdout


def indent_of(line: str) -> int:
    return len(line) - len(line.lstrip(" "))


def block_end(lines: list[str], start: int) -> int:
    """The index of the line where the brace that opens on `lines[start]` is closed."""
    depth = 0
    for k in range(start, len(lines)):
        # Strings and characters hold no braces that count.
        for ch in re.sub(r'"(\\.|[^"\\])*"|\'(\\.|[^\'\\])*\'', '""', lines[k]):
            depth += ch == "{"
            depth -= ch == "}"
        if depth == 0:
            return k
    return len(lines) - 1


def join_blocks(text: str) -> str:
    """`.style =` and the line with only `{` after it become `.style = {`; the block moves left."""
    lines = text.split("\n")
    i = 0
    while i < len(lines) - 1:
        cur, nxt = lines[i], lines[i + 1]
        if cur.rstrip().endswith("=") and nxt.strip() == "{":
            shift = indent_of(nxt) - indent_of(cur)
            end = block_end(lines, i + 1)
            lines[i] = cur.rstrip() + " {"
            body = [line[shift:] if indent_of(line) >= shift else line for line in lines[i + 2 : end + 1]]
            lines[i + 1 : end + 1] = body
        i += 1
    return "\n".join(lines)


OPENER = re.compile(r"^(\s*)(\.\w+)\s+= \{$")


def tighten_blocks(text: str) -> str:
    """A property that opens a block is `.name = {` with one space, and its body is one step in.

    clang-format pads `.style  =` to the `=` of the lines above it and then puts the body three columns in.
    """
    lines = text.split("\n")
    for i in range(len(lines) - 1):
        found = OPENER.match(lines[i])
        if not found:
            continue
        lines[i] = f"{found.group(1)}{found.group(2)} = {{"
        delta = indent_of(lines[i]) + 2 - indent_of(lines[i + 1])
        if delta:
            end = block_end(lines, i)
            lines[i + 1 : end] = [(" " * delta + line if delta > 0 else line[-delta:]) if line.strip() else line
                                  for line in lines[i + 1 : end]]
            lines[end] = " " * indent_of(lines[i]) + lines[end].lstrip()
    return "\n".join(lines)


# The units of lv::units. clang-format reads `b * px` after a comma as a declaration of a pointer
# and writes `b* px`; it is a product, and the pass writes the spaces back.
UNITS = r"(?:px|pct|ms|sec|min|deg)"
POINTER_LOOK = re.compile(r"(?<=[\w)\]])\* (" + UNITS + r")\b")


def restore_products(text: str) -> str:
    """`height* px` becomes `height * px`. It changes white space only."""
    return POINTER_LOOK.sub(r" * \1", text)


def squeezed(text: str) -> str:
    return re.sub(r"\s+", "", text)


def format_text(text: str, name: str, program: str) -> str:
    """The text as the repository writes it. Raises if more than white space would change."""
    formatted = restore_products(tighten_blocks(join_blocks(run_clang_format(text, name, program))))
    if squeezed(formatted) != squeezed(text):
        raise RuntimeError(f"{name}: the format would change more than white space")
    return formatted


def files_of(paths: list[str]) -> list[pathlib.Path]:
    found: list[pathlib.Path] = []
    for p in map(pathlib.Path, paths):
        found += sorted(f for f in p.rglob("*") if f.suffix in SUFFIXES) if p.is_dir() else [p]
    return found


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("paths", nargs="+", help="files, or directories searched for .cpp and .hpp")
    parser.add_argument("--check", action="store_true", help="change nothing; exit 1 if a file would change")
    args = parser.parse_args(argv)
    program = clang_format_program()
    if not program:
        print("clang-format 21 to 23 is not on the path (CLANG_FORMAT names it)", file=sys.stderr)
        return 2
    if major_version(program) not in VERSIONS:
        print(f"{program} is version {major_version(program)}; the format is pinned for {VERSIONS}", file=sys.stderr)
        return 2
    status = 0
    for path in files_of(args.paths):
        text = path.read_text()
        formatted = format_text(text, str(path), program)
        if formatted != text:
            print(f"{path}: {'would change' if args.check else 'formatted'}")
            status = 1
            if not args.check:
                path.write_text(formatted)
    return status if args.check else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
