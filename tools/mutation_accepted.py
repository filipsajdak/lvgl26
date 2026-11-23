#!/usr/bin/env python3
"""The format of tools/mutation_accepted.json: one entry on each line, sorted by header and mutant.

Two branches that each add entries then merge without a conflict, unless they add the same entry.

    python3 tools/mutation_accepted.py            writes the file in that form
    python3 tools/mutation_accepted.py --check    fails when the file is not in that form
"""
import json
import pathlib
import sys

PATH = pathlib.Path(__file__).with_name("mutation_accepted.json")


def canonical(rows: list[dict]) -> str:
    """The text of the file for these entries."""
    ordered = sorted(rows, key=lambda row: (row["header"], row["mutant"]))
    lines = [json.dumps(row, ensure_ascii=False) for row in ordered]
    return "[\n" + ",\n".join("  " + line for line in lines) + "\n]\n"


def main(argv: list[str]) -> int:
    rows = json.loads(PATH.read_text())
    text = canonical(rows)
    if "--check" in argv:
        if PATH.read_text() != text:
            print(f"{PATH.name} is not one sorted entry per line: run python3 tools/mutation_accepted.py")
            return 1
        return 0
    PATH.write_text(text)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
