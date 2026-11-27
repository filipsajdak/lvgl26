#!/usr/bin/env python3
"""Writes include/lv/icons/phosphor.names, the names of Phosphor's icons and their code points.

    python3 tools/icons_names.py path/to/phosphor-web/src/regular/style.css

The file has a line for each icon, sorted by name: the name, a space, the code point in hexadecimal.
Phosphor Icons (MIT) 2.1.1 gives each icon a class `.ph-<name>:before` with its code point in the
private use area; the Regular, Bold and other weights share the names and the code points.
"""
import pathlib
import re
import sys

OUT = pathlib.Path(__file__).resolve().parent.parent / "include" / "lv" / "icons" / "phosphor.names"
RULE = re.compile(r'\.ph\.ph-([a-z0-9-]+):before\s*\{\s*content:\s*"\\([0-9a-f]+)"')


def icons(css: str) -> dict[str, int]:
    """The code point of each icon in the style sheet, by name."""
    return {name: int(code, 16) for name, code in RULE.findall(css)}


def render(table: dict[str, int]) -> str:
    """The text of the names file for `table`."""
    return "".join(f"{name} {code:x}\n" for name, code in sorted(table.items()))


def main(argv: list[str]) -> int:
    if len(argv) != 1:
        print(__doc__)
        return 2
    table = icons(pathlib.Path(argv[0]).read_text())
    if not table:
        print("no icon found in that file")
        return 1
    OUT.write_text(render(table))
    print(f"{len(table)} icons")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
