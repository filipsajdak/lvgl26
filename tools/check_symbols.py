#!/usr/bin/env python3
"""Fails when an object file has a symbol with a name longer than a budget.

    python3 tools/check_symbols.py --budget 4096 build/tests/CMakeFiles/image_symbols.dir/image_symbols.cpp.o

A constant that the compiler makes can be named by its value: a template parameter object whose
bytes are part of its mangled name. Apple's linker refuses a name longer than LINKER_LIMIT, so such a
constant works on Linux and breaks on a Mac. The test runs everywhere, because `nm` does.

LINKER_LIMIT was found by linking assembler files that define one symbol of a given length with
ld 27106.1 (Xcode 27): a name of 1,048,575 characters links, and one of 1,048,577 stops with
"Assertion failed: (name.size() <= maxLength)".

Uses the standard library and `nm`. Exits with 1 and the longest names if one is over the budget.
"""

import argparse
import pathlib
import subprocess
import sys

LINKER_LIMIT = 1048575
SHOWN = 100  # characters of a long name that the message shows


def names(nm_output):
    """The symbol names out of `nm -P`: the first field of each line."""
    return [line.split(" ", 1)[0] for line in nm_output.splitlines() if line.strip()]


def too_long(symbols, budget):
    return sorted((s for s in symbols if len(s) > budget), key=len, reverse=True)


def symbols_of(path):
    out = subprocess.run(["nm", "-P", str(path)], check=True, capture_output=True, text=True)
    return names(out.stdout)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--budget", type=int, required=True, help="the longest name allowed, in characters")
    parser.add_argument("objects", nargs="+", type=pathlib.Path)
    args = parser.parse_args(argv)
    if args.budget > LINKER_LIMIT:
        parser.error(f"a budget over the linker's limit of {LINKER_LIMIT} would let a failing name pass")
    failed = False
    for path in args.objects:
        symbols = symbols_of(path)
        for name in too_long(symbols, args.budget)[:3]:
            failed = True
            print(f"{path}: a symbol of {len(name)} characters, over the budget of {args.budget}: {name[:SHOWN]}...")
        print(f"{path}: {len(symbols)} symbols, the longest of {max(map(len, symbols), default=0)} characters")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
