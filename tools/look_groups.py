#!/usr/bin/env python3
"""Reports the component groups of a token file that no source names in an lv::look.

    python3 tools/look_groups.py tokens.json src/ [more files or folders] [--ignore tile.usb]

A component group is a group of the token file whose entries are named for style properties
(`radius`, `bg_color`, ...): the groups that lv::look turns into a style. The token file holds
nothing of ours: it is a plain Design Tokens file that design tools open. Groups of primitive and
semantic tokens (`color`, `space`, `text`) have no entry named for a style property, and are
never reported. A group named like an LVGL state or part (`checked`, `knob`) is a layer of the group
above it, which lv::look takes along, so it is reported only through its parent.

A source uses a group when it names it in lv::look<"a.b">, in a list such as lv::look<"a a.b">,
or by lv::look_of<"a", E>, which uses every sibling group named `a-...` (`a-print`, `a-scan`): the
compiler picks it by the name of an enumerator, which this tool cannot read.

One file cannot see the whole program: a group that another program uses is listed with
--ignore. Exits with 1 and the unused groups if there are any. Uses the standard library only.
"""

import argparse
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
REFERENCE = ROOT / "docs" / "reference" / "lvgl-v9.6.0.json"
SOURCE_SUFFIXES = (".cpp", ".hpp", ".h", ".cc", ".cxx", ".ipp")
LOOK = re.compile(r'lv::look\s*<\s*"([^"]*)"')
LOOK_OF = re.compile(r'lv::look_of\s*<\s*"([^"]*)"')
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
    return styles, layers - {"any", "default"}


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


def component_groups(tokens: dict, styles: set[str], layers: set[str]) -> list[str]:
    """The paths of the component groups, not counting the layers of another group."""
    found = []
    for path, group in groups_of(tokens):
        if path.split(".")[-1] in layers:
            continue
        if has_style_entry(group, styles):
            found.append(path)
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


def uses_in(text: str) -> tuple[set[str], set[str]]:
    """The group paths named in `text`, and the prefixes named by lv::look_of."""
    used = {path for names in LOOK.findall(text) for path in names.split()}
    prefixes = set(LOOK_OF.findall(text))
    return used, prefixes


def is_variant_of(path: str, prefix: str) -> bool:
    """Whether `path` is a sibling group of `prefix` that lv::look_of<prefix, E> may name: prefix-name."""
    return path.startswith(prefix + "-") and "." not in path[len(prefix) + 1:]


def unused_groups(tokens: dict, files: list[pathlib.Path], styles: set[str], layers: set[str], ignore: set[str]) -> list[str]:
    used, prefixes = set(), set()
    for file in files:
        u, p = uses_in(file.read_text())
        used |= u
        prefixes |= p
    unused = []
    for path in component_groups(tokens, styles, layers):
        if path in used or path in ignore or any(is_variant_of(path, prefix) for prefix in prefixes):
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
