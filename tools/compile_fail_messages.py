#!/usr/bin/env python3
"""The messages that the compile-fail tests pin, as data.

    python3 tools/compile_fail_messages.py <build dir> [ctest]    prints {test: {"form": ..., "message": ...}} as JSON

A test written with lvgl26_compile_fail_message keeps the whole text of the message it checks in its CTest
properties. This reads them back, for a check that a page of the documentation quotes a message as its test
has it. The build directory is configured; nothing is run. Uses the standard library only.
"""

import json
import subprocess
import sys

MESSAGE = "LVGL26_MESSAGE"
FORM = "LVGL26_MESSAGE_FORM"


def messages_of(tests):
    """{test: {"form": form, "message": text}} for the tests of `ctest --show-only=json-v1` that pin a message."""
    found = {}
    for test in tests:
        props = {p["name"]: p["value"] for p in test.get("properties", [])}
        if MESSAGE in props:
            found[test["name"]] = {"form": props[FORM], "message": props[MESSAGE]}
    return found


def main(argv):
    if not argv:
        print(__doc__.strip().splitlines()[0], file=sys.stderr)
        return 2
    out = subprocess.run([argv[1] if len(argv) > 1 else "ctest", "--show-only=json-v1"], cwd=argv[0],
                         capture_output=True, text=True, check=True).stdout
    print(json.dumps(messages_of(json.loads(out)["tests"]), indent=1, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
