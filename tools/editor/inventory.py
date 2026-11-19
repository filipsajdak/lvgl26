#!/usr/bin/env python3
"""Checks that the editor stubs know every public name of lvgl26.

    lv_names | python3 tools/editor/inventory.py [--using FILE]

`lv_names` (tools/editor/names.cpp) prints the public names by reflection: every
member of namespace lv and of the namespaces in it, apart from lv::detail. For each
name this tool asks two things:

    is it declared in the hand-written stubs (tools/editor/stubs)?
    is it used in tests/editor/api_uses.cpp, which both compilers read?

A name that fails either is printed, and the tool exits with 1. So a new public name
cannot be added without its stub and a use that proves the stub has the same shape.

With --using FILE it also writes a C++ file that names each of them with a
using-declaration. Clang reads that file with the stubs (the test editor_names_declared):
a name that is only mentioned in the stubs, and not declared in its namespace, fails there.

The check here is by word, so it needs no Clang and runs wherever the tests run.
Uses the standard library only.
"""

import argparse
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
STUBS = ROOT / "tools" / "editor" / "stubs"
USES = ROOT / "tests" / "editor" / "api_uses.cpp"
COMMENT = re.compile(r"//[^\n]*")
HEADERS = ["lv/host.hpp", "lv/lv.hpp", "lv/mcp.hpp", "lv/one_of.hpp", "lv/png.hpp", "lv/pointer.hpp", "lv/signal.hpp", "lv/signal_type.hpp", "lv/ui_scheduler.hpp"]


def public_names(text: str) -> list[str]:
    """The names lv_names printed, each once. A name the compiler made up starts with two underscores."""
    names = [line.strip() for line in text.splitlines() if line.strip().startswith("lv::")]
    return sorted({name for name in names if not name.rsplit("::", 1)[1].startswith("__")})


def code_of(path: pathlib.Path) -> str:
    """The text of a source file without its comments: a name in a comment declares nothing."""
    return COMMENT.sub("", path.read_text())


def namespace_of(name: str) -> str:
    return name.rsplit("::", 1)[0]


def is_declared(name: str, stubs: dict[str, str]) -> bool:
    """Whether a stub that opens the name's namespace has the name as a word."""
    space, last = namespace_of(name), name.rsplit("::", 1)[1]
    inner = space.removeprefix("lv").removeprefix("::")
    opens = re.compile(rf"namespace (lv::)?{re.escape(inner)}\b" if inner else r"namespace lv\b")
    word = re.compile(rf"\b{re.escape(last)}\b")
    return any(opens.search(text) and word.search(text) for text in stubs.values())


def is_used(name: str, uses: str) -> bool:
    """Whether the uses file names it: in full, or after a using-directive for a unit."""
    full = re.compile(rf"\b{re.escape(name)}\b")
    short = name.replace("lv::units::", "lv::")
    return bool(full.search(uses) or re.search(rf"\b{re.escape(short)}\b", uses))


def using_file(names: list[str]) -> str:
    """A C++ file that names every public name with a using-declaration."""
    includes = "\n".join(f"#include <{header}>" for header in HEADERS)
    lines = "\n".join(f"using {name};" for name in names)
    return ("// Written by tools/editor/inventory.py: every public name of lvgl26, named once.\n"
            f"{includes}\n\nnamespace names_declared {{\n{lines}\n}}  // namespace names_declared\n")


def problems(names: list[str], stubs: dict[str, str], uses: str) -> list[str]:
    found = [f"{name}: not declared in tools/editor/stubs" for name in names if not is_declared(name, stubs)]
    found += [f"{name}: not used in tests/editor/api_uses.cpp" for name in names if not is_used(name, uses)]
    return found


def main(arguments: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Checks that the editor stubs know every public name of lvgl26.")
    parser.add_argument("--names", type=pathlib.Path, help="a file with the output of lv_names; default: stdin")
    parser.add_argument("--using", type=pathlib.Path, help="write a C++ file that names every public name")
    options = parser.parse_args(arguments)
    names = public_names(options.names.read_text() if options.names else sys.stdin.read())
    if not names:
        print("editor inventory: no names given; pipe the output of lv_names in")
        return 1
    if options.using:
        options.using.parent.mkdir(parents=True, exist_ok=True)
        options.using.write_text(using_file(names))
    stubs = {str(path): code_of(path) for path in sorted(STUBS.rglob("*.hpp"))}
    found = problems(names, stubs, code_of(USES))
    for problem in found:
        print(problem)
    print(f"{len(names)} public names, {len(found)} problems")
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
