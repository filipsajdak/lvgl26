#!/usr/bin/env python3
"""Reports the component groups of a token file that no source names in an lv::look.

    python3 tools/look_groups.py tokens.json src/ [more files or folders] [--ignore tile.usb]

A component group is a group of the token file whose entries are named for style properties
(`radius`, `bg_color`, ...): the groups that lv::look turns into a style. The token file holds
nothing of ours: it is a plain Design Tokens file that design tools open. Groups of primitive and
semantic tokens (`color`, `space`, `text`) have no entry named for a style property, and are
never reported. A group named like an LVGL state or part (`checked`, `knob`) is a layer of the look
above it, which lv::look takes along, so it is reported only through its parent. A group that sets a
style property only in its layers is a look too, and so is a group named like a part that no look holds.

A source uses a group when it names it in lv::look<"a.b">, in a list such as lv::look<"a a.b">,
or by lv::look_of<"a", E>, which uses every sibling group named `a-...` (`a-print`, `a-scan`): the
compiler picks it by the name of an enumerator, which this tool cannot read. Only code counts: a
look in a comment or inside another string literal uses nothing. A name is compared as the compiler
reads it, a C++ member name for each part of the path (`tile_print` for `tile-print`); the report
keeps the spelling of the token file.

One file cannot see the whole program: a group that another program uses is listed with
--ignore. Exits with 1 and the unused groups if there are any. Uses the standard library only.
"""

import argparse
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools" / "editor"))
from token_stubs import member_name  # noqa: E402  (the one copy of the compiler's naming rule)

REFERENCE = ROOT / "docs" / "reference" / "lvgl-v9.6.0.json"
SOURCE_SUFFIXES = (".cpp", ".hpp", ".h", ".cc", ".cxx", ".ipp")
# The code just before a look's string: lv::look<, lv::look_of< or lv::mode_look<, with any space.
LOOK_HEAD = re.compile(r"\blv\s*::\s*(look|look_of|mode_look)\s*<\s*$")
RAW_PREFIX = re.compile(r"(?:\b|^)(?:u8|u|U|L)?R$")
# How much code before a string is enough to see lv::look_of< in it, with some space.
CODE_KEPT = 64
LAYER_ENUMS = ("lv_part_t", "lv_state_t")


class Error(Exception):
    """The input is not usable; the message says what to fix."""


def reference_names(path: pathlib.Path) -> tuple[set[str], set[str]]:
    """The style property names and the names of the parts and states, from the reference data."""
    data = json.loads(path.read_text())
    styles = {entry["name"] for entry in data["style"]}
    layers = {name for enum in LAYER_ENUMS for name in data["enums"].get(enum, [])}
    if not layers:
        raise Error(f"{path} lists no parts or states: write it again with the lv_reference program")
    return styles, layers - {"any"}


def is_group(value) -> bool:
    return isinstance(value, dict) and "$value" not in value


def groups_of(node: dict, prefix: str = ""):
    """Every group below `node` as (dotted path, group)."""
    for key, value in node.items():
        if key.startswith("$") or not is_group(value):
            continue
        path = f"{prefix}{key}"
        yield path, value
        yield from groups_of(value, path + ".")


VENDOR_KEY = "io.github.filipsajdak.lvgl26"


def has_style_entry(group: dict, styles: set[str]) -> bool:
    """An entry named for a style property, or vendor data under $extensions (include/lv/tokens.hpp)."""
    if isinstance(group.get("$extensions"), dict) and VENDOR_KEY in group["$extensions"]:
        return True
    return any(key in styles and not is_group(value) for key, value in group.items() if not key.startswith("$"))


def is_layer_name(key: str, layers: set[str]) -> bool:
    """Whether a group of this name in a look is one of its layers. The compiler reads the name as
    a C++ member (`focus-key` as focus_key, `default` as default_), and so does this."""
    return member_name(key) in {member_name(layer) for layer in layers}


def layer_children(group: dict, layers: set[str]):
    return [value for key, value in group.items() if not key.startswith("$") and is_group(value) and is_layer_name(key, layers)]


def is_look(group: dict, styles: set[str], layers: set[str]) -> bool:
    """A group that sets a style property itself or in one of its layers (`checked` only, say)."""
    return has_style_entry(group, styles) or any(is_look(layer, styles, layers) for layer in layer_children(group, layers))


def component_groups(tokens: dict, styles: set[str], layers: set[str]) -> list[str]:
    """The paths of the component groups. A group named for a part or a state is a layer of the
    look above it, and lv::look takes it along; where no look holds it, it is a look of its own."""
    found = []

    def visit(node: dict, prefix: str, in_a_look: bool) -> None:
        for key, value in node.items():
            if key.startswith("$") or not is_group(value) or (in_a_look and is_layer_name(key, layers)):
                continue
            path = f"{prefix}{key}"
            look = is_look(value, styles, layers)
            if look:
                found.append(path)
            visit(value, path + ".", look)

    visit(tokens, "", False)
    return found


def source_files(paths: list[str]) -> list[pathlib.Path]:
    files = []
    for name in paths:
        path = pathlib.Path(name)
        if path.is_dir():
            files += sorted(p for p in path.rglob("*") if p.suffix in SOURCE_SUFFIXES)
        elif path.is_file():
            files.append(path)
        else:
            raise Error(f"{name} is neither a file nor a folder")
    return files


def string_end(text: str, start: int) -> tuple[int, str]:
    """The end of the literal opened by the quote at `start`, and its contents. A raw string
    (R"x(...)x") ends at its own delimiter; any other ends at an unescaped quote."""
    quote = text[start]
    opening = text.find("(", start)
    if quote == '"' and opening > 0 and RAW_PREFIX.search(text[max(0, start - 3):start]):
        delimiter = text[start + 1:opening]
        close = text.find(")" + delimiter + '"', start)
        close = len(text) if close < 0 else close
        return close + len(delimiter) + 2, text[start + len(delimiter) + 2:close]
    i = start + 1
    while i < len(text) and text[i] != quote:
        i += 2 if text[i] == "\\" else 1
    return i + 1, text[start + 1:i]


def comment_end(text: str, start: int) -> int:
    if text.startswith("//", start):
        end = text.find("\n", start)
        return len(text) if end < 0 else end
    end = text.find("*/", start + 2)
    return len(text) if end < 0 else end + 2


def opens_literal(text: str, i: int) -> bool:
    """A quote that starts a string or a character; the ' of 1'000 is part of a number."""
    return text[i] == '"' or (text[i] == "'" and not (i and text[i - 1].isalnum()))


def look_strings(text: str):
    """(look or look_of, the text of its string) for each string literal that follows lv::look< or
    lv::look_of< in code: not in a comment, and not inside another string."""
    code = ""
    i = 0
    while i < len(text):
        if text.startswith(("//", "/*"), i):
            i = comment_end(text, i)
            code += " "
        elif opens_literal(text, i):
            i, contents = string_end(text, i)
            head = LOOK_HEAD.search(code) if text[i - 1] == '"' else None
            if head:
                yield head.group(1), contents
            code = " "
        else:
            code = (code + text[i])[-CODE_KEPT:]
            i += 1


def canonical(path: str) -> str:
    """A dotted path as the compiler follows it: each name as a C++ member (`tile_print` for `tile-print`)."""
    return ".".join(member_name(segment) for segment in path.split("."))


def uses_in(text: str) -> tuple[set[str], set[str]]:
    """The group paths named in `text`, and the prefixes named by lv::look_of, both canonical."""
    used, prefixes = set(), set()
    for kind, contents in look_strings(text):
        if kind in ("look", "mode_look"):
            used |= {canonical(path) for path in contents.split()}
        else:
            prefixes.add(contents)
    return used, prefixes


def is_variant_of(path: str, prefix: str) -> bool:
    """Whether `path` is a sibling group of `prefix` that lv::look_of<prefix, E> may name: prefix-name.
    The compiler follows prefix + "-" + the enumerator's name, so both are compared as it reads them."""
    *parent, last = path.split(".")
    *prefix_parent, prefix_last = prefix.split(".")
    head = member_name(prefix_last + "-")
    return canonical(".".join(parent)) == canonical(".".join(prefix_parent)) and \
        len(parent) == len(prefix_parent) and member_name(last).startswith(head) and len(member_name(last)) > len(head)


def unused_groups(tokens: dict, files: list[pathlib.Path], styles: set[str], layers: set[str], ignore: set[str]) -> list[str]:
    """The component groups that no file names, in the spelling of the token file."""
    used, prefixes = set(), set()
    for file in files:
        u, p = uses_in(file.read_text())
        used |= u
        prefixes |= p
    named = used | {canonical(path) for path in ignore}
    unused = []
    for path in component_groups(tokens, styles, layers):
        if canonical(path) in named or any(is_variant_of(path, prefix) for prefix in prefixes):
            continue
        unused.append(path)
    return unused


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Reports the component groups that no source names in an lv::look.")
    parser.add_argument("tokens", help="the token file")
    parser.add_argument("sources", nargs="+", help="source files or folders")
    parser.add_argument("--reference", default=str(REFERENCE), help="reference data of the LVGL in use")
    parser.add_argument("--ignore", nargs="*", default=[], help="groups another program uses")
    args = parser.parse_args(argv)
    try:
        styles, layers = reference_names(pathlib.Path(args.reference))
        tokens = json.loads(pathlib.Path(args.tokens).read_text())
        unused = unused_groups(tokens, source_files(args.sources), styles, layers, set(args.ignore))
    except (Error, OSError, json.JSONDecodeError) as error:
        print(f"look_groups: {error}", file=sys.stderr)
        return 2
    for path in unused:
        print(f"{args.tokens}: the group '{path}' is named by no lv::look")
    print(f"{len(component_groups(tokens, styles, layers))} component groups, {len(unused)} unused")
    return 1 if unused else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
