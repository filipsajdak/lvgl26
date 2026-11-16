#!/usr/bin/env python3
"""Writes the token files of the built-in Signal theme from the design system.

    python3 tools/signal_tokens.py          writes include/lv/signal/dark.json and day.json
    python3 tools/signal_tokens.py --check  fails when a written file is stale

design/signal/tokens.design.json is the source. include/lv/signal.hpp embeds the
written files, which are in the W3C Design Tokens format that lv::tokens reads. A
token name loses its group prefix: `space-4` is `space.4`, `stroke-track` is
`stroke.track`. A value that is not a length in pixels (`radius-face`, 50%) is left out.
"""

import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
SOURCE = ROOT / "design" / "signal" / "tokens.design.json"
OUT = ROOT / "include" / "lv" / "signal"
LENGTHS = {"spacing": "space", "radius": "radius", "stroke": "stroke", "size": "size"}


def lengths(design: dict, section: str, group: str) -> dict:
    """One group of dimension tokens, without the group's name in each token's name."""
    out = {"$type": "dimension"}
    for token in design[section]["tokens"]:
        if token["value"].endswith("px"):
            out[token["name"].removeprefix(group + "-")] = {"$value": token["value"]}
    return out


def document(design: dict, theme: str) -> str:
    colors = {"$type": "color"}
    for token in design["color"]["tokens"]:
        colors[token["name"]] = {"$value": token["value"][theme].lower()}
    doc = {"$description": f"Signal, {theme} theme. Written by tools/signal_tokens.py from "
                           "design/signal/tokens.design.json: do not edit.",
           "color": colors}
    for section, group in LENGTHS.items():
        doc[group] = lengths(design, section, group)
    return json.dumps(doc, indent=2) + "\n"


def main() -> int:
    design = json.loads(SOURCE.read_text())
    stale = []
    for theme in (t["id"] for t in design["color"]["themes"]):
        path, text = OUT / f"{theme}.json", document(design, theme)
        if "--check" in sys.argv[1:]:
            if not path.exists() or path.read_text() != text:
                stale.append(path.relative_to(ROOT))
        else:
            path.write_text(text)
    for path in stale:
        print(f"{path} is stale: run python3 tools/signal_tokens.py")
    return 1 if stale else 0


if __name__ == "__main__":
    sys.exit(main())
