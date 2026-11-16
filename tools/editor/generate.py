#!/usr/bin/env python3
"""Writes the stub headers that let an editor read lvgl26 code.

    python3 tools/editor/generate.py --reference docs/reference/lvgl-v9.6.0.json --out DIR
        [--scan PROJECT_DIR] [--skip DIR] [--tokens FILE ...] [--real-include DIR]

lvgl26 is C++26 with reflection, which only GCC 16 compiles. clangd, the language
server behind most editors, cannot parse it. The stubs declare the same public API
as plain C++: clangd then completes the members of `lv::arc({.`, shows what each one
takes, and marks a wrong name in quotes. They are for editors only. GCC still builds
the program from the real headers.

What is written into DIR:

    lv/*.hpp               the hand-written stubs of tools/editor/stubs, copied
    lv/generated.hpp       properties, styles, events and enumerator names (from --reference)
    lv/token_files.hpp     a struct for each design token file
    lv/signal_tokens.hpp   the tokens of the Signal theme
    embed/*.txt            what the editor embeds in place of each token file
    fallback/contracts     the standard header <contracts>, for a library without it
    overlay.yaml           a Clang file overlay: the real headers and the token files
                           lead to the files above
    token_files.txt        the token files that were found, one for each line

Token files are the DTCG files under --scan (a JSON object with a "$value" in it; build
trees, hidden directories and the ones named with --skip are left out) and the ones
named with --tokens.

`cmake/editor.cmake` runs this for a project. Uses the standard library only.
"""

import argparse
import json
import os
import pathlib
import shutil
import sys

import reference_stubs
import token_stubs

HERE = pathlib.Path(__file__).resolve().parent
STUBS = HERE / "stubs"
FALLBACK = HERE / "fallback"
SIGNAL = HERE.parent.parent / "include" / "lv" / "signal"
LARGEST_TOKEN_FILE = 4 * 1024 * 1024
SKIPPED_DIRECTORIES = {"node_modules", "__pycache__"}


def is_skipped(directory: pathlib.Path, skip: set[pathlib.Path]) -> bool:
    """A hidden directory, a build tree, one that holds other people's packages, or one of `skip`."""
    name = directory.name
    if name.startswith(".") or name in SKIPPED_DIRECTORIES or directory.resolve() in skip:
        return True
    return (directory / "CMakeCache.txt").exists()


def json_files(root: pathlib.Path, skip: set[pathlib.Path]) -> list[pathlib.Path]:
    """Every *.json file under `root`, outside the directories that are skipped."""
    found = []
    for directory, subdirectories, names in os.walk(root):
        here = pathlib.Path(directory)
        subdirectories[:] = sorted(d for d in subdirectories if not is_skipped(here / d, skip))
        found += [here / name for name in sorted(names) if name.endswith(".json")]
    return found


def token_document(path: pathlib.Path) -> bytes | None:
    """The bytes of `path` if it is a design token file, or nothing."""
    try:
        if path.stat().st_size > LARGEST_TOKEN_FILE:
            return None
        data = path.read_bytes()
        return data if token_stubs.is_token_file(json.loads(data)) else None
    except (OSError, ValueError):
        return None


def candidates(scan: list[pathlib.Path], named: list[pathlib.Path], skip: list[pathlib.Path]) -> list[pathlib.Path]:
    """The files that may be token files: the named ones first, then every *.json file found."""
    skipped = {path.resolve() for path in skip}
    return list(named) + [path for root in scan for path in json_files(root, skipped)]


def find_token_files(scan: list[pathlib.Path], named: list[pathlib.Path],
                     skip: list[pathlib.Path] | None = None) -> dict[pathlib.Path, bytes]:
    """The token files of a project, by path: the named ones, and those found by scanning."""
    found = {}
    for path in candidates(scan, named, skip or []):
        data = token_document(path)
        if data is not None:
            found.setdefault(path.absolute(), data)
    for path in named:
        if path.absolute() not in found:
            sys.exit(f"editor stubs: {path} is not a design token file (a JSON object with a \"$value\" in it)")
    return found


def spellings(path: pathlib.Path) -> list[str]:
    """`path` as CMake wrote it, and with its links resolved: an editor may use either."""
    return list(dict.fromkeys([str(path), str(path.resolve())]))


def overlay(out: pathlib.Path, real_include: pathlib.Path | None, token_files: dict[pathlib.Path, bytes]) -> str:
    """A Clang file overlay: the real headers and the token files lead to the stubs."""
    roots = []
    if real_include is not None:
        roots += [{"name": name, "type": "directory-remap", "external-contents": str(out / "lv")}
                  for name in spellings(real_include / "lv")]
    for path, data in token_files.items():
        marker = out / "embed" / f"{token_stubs.key_of(data):016x}.txt"
        roots += [{"name": name, "type": "file", "external-contents": str(marker)} for name in spellings(path)]
    return json.dumps({"version": 0, "case-sensitive": "false", "roots": roots}, indent=1) + "\n"


def signal_header(files: dict[str, bytes]) -> str:
    """lv/signal_tokens.hpp: the tokens of Signal, which lv/signal.hpp embeds in a real build."""
    lines = [f"/// The struct of the design tokens of Signal's {name} theme.\n"
             f"using signal_{name}_tokens = {token_stubs.struct_name(token_stubs.key_of(data))};"
             for name, data in sorted(files.items())]
    return ("#pragma once\n\n// Written by tools/editor/generate.py from include/lv/signal/*.json. Do not edit.\n\n"
            "#include <lv/token_files.hpp>\n\nnamespace lv::stub {\n\n" + "\n".join(lines) +
            "\n\n}  // namespace lv::stub\n")


def write_if_changed(path: pathlib.Path, text: str) -> None:
    """Writes `text`, unless the file holds it already: an editor reads a changed file again."""
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


def copy_stubs(out: pathlib.Path) -> None:
    for source in sorted(STUBS.rglob("*.hpp")):
        write_if_changed(out / source.relative_to(STUBS), source.read_text())
    for source in sorted(FALLBACK.iterdir()):
        write_if_changed(out / "fallback" / source.name, source.read_text())


def write_markers(out: pathlib.Path, token_files: dict[pathlib.Path, bytes]) -> None:
    embed = out / "embed"
    if embed.exists():
        shutil.rmtree(embed)
    for data in token_files.values():
        key = token_stubs.key_of(data)
        write_if_changed(embed / f"{key:016x}.txt", token_stubs.marker(key))


def generate(reference: pathlib.Path, out: pathlib.Path, token_files: dict[pathlib.Path, bytes],
             real_include: pathlib.Path | None) -> None:
    """Writes everything the docstring of this file lists into `out`."""
    signal = {path.stem: path.read_bytes() for path in sorted(SIGNAL.glob("*.json"))}
    said = [(str(path), data) for path, data in token_files.items()]
    said += [(f"Signal's {name} theme", data) for name, data in signal.items()]
    copy_stubs(out)
    write_if_changed(out / "lv" / "generated.hpp",
                     reference_stubs.header(json.loads(reference.read_text()), reference.name))
    write_if_changed(out / "lv" / "token_files.hpp", token_stubs.header(said))
    write_if_changed(out / "lv" / "signal_tokens.hpp", signal_header(signal))
    write_markers(out, token_files)
    write_if_changed(out / "overlay.yaml", overlay(out, real_include, token_files))
    write_if_changed(out / "token_files.txt", "".join(f"{path}\n" for path in token_files))


def main(arguments: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Writes the stub headers that let an editor read lvgl26 code.")
    parser.add_argument("--reference", type=pathlib.Path, required=True, help="docs/reference/lvgl-<version>.json")
    parser.add_argument("--out", type=pathlib.Path, required=True, help="the directory to write into")
    parser.add_argument("--scan", type=pathlib.Path, action="append", default=[],
                        help="a directory to search for design token files")
    parser.add_argument("--skip", type=pathlib.Path, action="append", default=[],
                        help="a directory that is not searched: the build directory, before it has a cache")
    parser.add_argument("--tokens", type=pathlib.Path, nargs="*", default=[], help="design token files")
    parser.add_argument("--real-include", type=pathlib.Path, help="lvgl26's include directory, for the overlay")
    options = parser.parse_args(arguments)
    out = options.out.absolute()
    token_files = find_token_files(options.scan, options.tokens, options.skip)
    generate(options.reference, out, token_files, options.real_include)
    print(f"editor stubs: {len(token_files)} token files, written to {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
