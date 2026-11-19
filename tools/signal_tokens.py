#!/usr/bin/env python3
"""Writes the token files of the built-in Signal theme from the design system.

    python3 tools/signal_tokens.py          writes include/lv/signal/dark.json, day.json and type.json
    python3 tools/signal_tokens.py --check  fails when a written file is stale

design/signal/tokens.design.json is the source. The written files are in the W3C Design
Tokens format that lv::tokens reads. A token name loses its group prefix: `space-4` is
`space.4`, `stroke-track` is `stroke.track`. A value that is not a length in pixels
(`radius-face`, 50%) is left out.

dark.json and day.json hold the colors and lengths of a theme: include/lv/signal.hpp
embeds them. type.json holds the typography, which is the same in every theme:
include/lv/signal_type.hpp embeds it. It is a file of its own because every token costs
compile time in the header that reads it, and lv/signal.hpp makes no font. `font` holds
the two families, each as the first name of its stack. `text` holds a typography token
for every style, with its line height as a multiple of the font size, as the format
writes it.
"""

import json
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import dtcg_fix  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parent.parent
SOURCE = ROOT / "design" / "signal" / "tokens.design.json"
OUT = ROOT / "include" / "lv" / "signal"
LENGTHS = {"spacing": "space", "radius": "radius", "stroke": "stroke", "size": "size"}
SCHEMA = "https://www.designtokens.org/schemas/2025.10/format.json"
WRITTEN = "Written by tools/signal_tokens.py from design/signal/tokens.design.json: do not edit."


def render(node: dict, level: int = 0) -> str:
    """The document with two spaces of indent for groups; a token is one line, so that its text stays short."""
    if "$value" in node:
        return "{" + ", ".join(
            f"{json.dumps(key)}: {json.dumps(value, separators=(',', ':'))}" for key, value in node.items()) + "}"
    pad = "  " * (level + 1)
    members = [f"{pad}{json.dumps(key)}: {render(value, level + 1) if isinstance(value, dict) else json.dumps(value)}"
               for key, value in node.items()]
    return "{\n" + ",\n".join(members) + "\n" + "  " * level + "}"


def lengths(design: dict, section: str, group: str) -> dict:
    """One group of dimension tokens, without the group's name in each token's name."""
    out = {"$type": "dimension"}
    for token in design[section]["tokens"]:
        if token["value"].endswith("px"):
            out[token["name"].removeprefix(group + "-")] = {"$value": dtcg_fix.quantity_object(token["value"], dtcg_fix.LENGTH_UNITS)}
    return out


def document(design: dict, theme: str) -> str:
    colors = {"$type": "color"}
    for token in design["color"]["tokens"]:
        colors[token["name"]] = {"$value": dtcg_fix.color_object(token["value"][theme].lower())}
    doc = {"$schema": SCHEMA, "$description": f"Signal, {theme} theme. {WRITTEN}", "color": colors}
    for section, group in LENGTHS.items():
        doc[group] = lengths(design, section, group)
    return render(doc) + "\n"


def families(design: dict) -> dict:
    """The font families: the first name of each stack, the one whose file a device holds."""
    out = {"$type": "fontFamily"}
    for name, stack in design["type"]["families"].items():
        out[name] = {"$value": stack.split(",")[0].strip().strip('"')}
    return out


def line_height(style: dict) -> float | int:
    """The line height of a style as a multiple of its font size, to four decimal places."""
    size, height = (int(style[key].removesuffix("px")) for key in ("fontSize", "lineHeight"))
    ratio = round(height / size, 4)
    if round(size * ratio) != height:
        sys.exit(f"signal_tokens: four decimal places lose the line height of {style['name']}")
    return int(ratio) if ratio == int(ratio) else ratio


def styles(design: dict) -> dict:
    """A typography token for every style of the design, in the order of the design file."""
    out = {"$type": "typography"}
    for group in design["type"]["groups"]:
        for style in group["styles"]:
            out[style["name"]] = {"$value": {"fontFamily": "{font." + group["family"] + "}",
                                             "fontSize": dtcg_fix.quantity_object(style["fontSize"], dtcg_fix.LENGTH_UNITS),
                                             "fontWeight": style["fontWeight"],
                                             "letterSpacing": dtcg_fix.quantity_object(style.get("letterSpacing", "0px"), dtcg_fix.LENGTH_UNITS),
                                             "lineHeight": line_height(style)}}
    return out


def typography(design: dict) -> str:
    doc = {"$schema": SCHEMA, "$description": f"Signal, typography. {WRITTEN}", "font": families(design), "text": styles(design)}
    return render(doc) + "\n"


def documents(design: dict) -> dict:
    """Every written file: its name and its text."""
    out = {t["id"] + ".json": document(design, t["id"]) for t in design["color"]["themes"]}
    out["type.json"] = typography(design)
    return out


def main() -> int:
    stale = []
    for name, text in documents(json.loads(SOURCE.read_text())).items():
        path = OUT / name
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
