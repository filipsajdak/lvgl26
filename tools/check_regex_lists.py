#!/usr/bin/env python3
r"""Fails when a CTest regular expression is really a list, so that it checks less than it says.

    python3 tools/check_regex_lists.py <build dir> [ctest]

CTest splits PASS_REGULAR_EXPRESSION and FAIL_REGULAR_EXPRESSION at every unescaped `;` and passes the test
when ANY part matches: `a; b` is "a" or " b", not the text "a; b". A message with a `;` in it, put into
lvgl26_compile_fail, was checked by its first half only. Write `\\;` in the CMake string, which CTest reads
as a literal `;`. The build directory is configured; nothing is run. Uses the standard library only.
"""

import json
import subprocess
import sys

PROPERTIES = ("PASS_REGULAR_EXPRESSION", "FAIL_REGULAR_EXPRESSION")


def split_tests(tests):
    """[(test, property, parts)] for each regular expression of `ctest --show-only=json-v1` that has more than one part."""
    found = []
    for test in tests:
        for prop in test.get("properties", []):
            if prop["name"] in PROPERTIES and len(prop["value"]) > 1:
                found.append((test["name"], prop["name"], prop["value"]))
    return found


def report(found):
    lines = [f"{name}: {prop} is {len(parts)} alternatives, not one text: {parts}" for name, prop, parts in found]
    lines.append(r"CTest splits a regular expression at an unescaped ';' and accepts any part. Escape it as \\; in tests/CMakeLists.txt.")
    return "\n".join(lines)


def main(argv):
    if not argv:
        print(__doc__.strip().splitlines()[0], file=sys.stderr)
        return 2
    ctest = argv[1] if len(argv) > 1 else "ctest"
    out = subprocess.run([ctest, "--show-only=json-v1"], cwd=argv[0], capture_output=True, text=True, check=True).stdout
    found = split_tests(json.loads(out)["tests"])
    if found:
        print(report(found))
        return 1
    print("every regular expression of a test is one text")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
