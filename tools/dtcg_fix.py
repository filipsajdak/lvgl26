#!/usr/bin/env python3
"""Rewrites design token files from the forms before the Design Tokens Format 2025.10 to its object forms.

    python3 tools/dtcg_fix.py FILE...          rewrite the files in place
    python3 tools/dtcg_fix.py --check FILE...  exit 1 for a file that would change, and change nothing

    "#3366ff"  ->  {"colorSpace": "srgb", "components": [0.2, 0.4, 1], "hex": "#3366ff"}
    "#3366ff80" ->  the same with "alpha": 0.502
    "8px"      ->  {"value": 8, "unit": "px"}          (a dimension)
    "200ms"    ->  {"value": 200, "unit": "ms"}        (a duration)

The forms are rewritten for the tokens of type color, dimension and duration, and for the members of
border, shadow, typography, transition and gradient tokens. An alias ("{color.ink}"), a value in the new
form, a bare number (the reader refuses it) and a type the tool does not know stay as they are. A dimension in percent ("50%") has no form in
the standard: the tool leaves it, and the check of tools/check_tokens.py names it.

Only the text of each old value changes: the rest of the file keeps its layout, so a diff shows the values
and nothing else. Running the tool twice changes nothing.
Uses the standard library only.
"""

import argparse
import json
import pathlib
import re
import sys

CHANNELS = 255
COMPONENT_DIGITS = 4
SRGB = "srgb"
HEX = re.compile(r"^#([0-9a-fA-F]{3}|[0-9a-fA-F]{6}|[0-9a-fA-F]{8})$")
QUANTITY = re.compile(r"^(-?[0-9]+(?:\.[0-9]+)?)(px|rem|ms|s)$")
LENGTH_UNITS = ("px", "rem")
TIME_UNITS = ("ms", "s")
# The members of a composite token that hold a value of a type the tool rewrites.
COMPOSITE_MEMBERS = {
    "border": {"color": "color", "width": "dimension"},
    "shadow": {"color": "color", "offsetX": "dimension", "offsetY": "dimension", "blur": "dimension", "spread": "dimension"},
    "typography": {"fontSize": "dimension", "letterSpacing": "dimension"},
    "transition": {"duration": "duration", "delay": "duration"},
    "gradient": {"color": "color"},
}


class Text(str):
    """A string of a document that knows where it stands in the text."""

    start = 0
    end = 0


def number(value):
    """A number as JSON writes it: 8 and not 8.0."""
    value = float(value)
    return int(value) if value.is_integer() else value


def component(channel):
    return number(round(channel / CHANNELS, COMPONENT_DIGITS))


def color_object(text):
    """The object form of "#rgb", "#rrggbb" or "#rrggbbaa"; None for anything else."""
    found = HEX.match(text)
    if not found:
        return None
    digits = found.group(1)
    if len(digits) == 3:
        digits = "".join(c * 2 for c in digits)
    channels = [int(digits[i:i + 2], 16) for i in (0, 2, 4)]
    out = {"hex": "#" + digits[:6].lower(), "colorSpace": SRGB, "components": [component(c) for c in channels]}
    if len(digits) == 8:
        out["alpha"] = component(int(digits[6:], 16))
    return out


def quantity_object(text, units):
    """{"value", "unit"} for "8px" or "200ms" with one of `units`; None for anything else."""
    found = QUANTITY.match(text)
    if not found or found.group(2) not in units:
        return None
    return {"value": number(found.group(1)), "unit": found.group(2)}


def rewritten(kind, text):
    """The object form of a string of the given token type, or None when it has none."""
    if kind == "color":
        return color_object(text)
    if kind == "dimension":
        return quantity_object(text, LENGTH_UNITS)
    if kind == "duration":
        return quantity_object(text, TIME_UNITS)
    return None


# ---- a JSON parser that keeps the position of each string ---------------------------------------

class Parser:
    def __init__(self, text):
        self.text = text
        self.at = 0

    def skip(self):
        while self.text[self.at].isspace():
            self.at += 1

    def value(self):
        self.skip()
        char = self.text[self.at]
        if char == "{":
            return self.collection("}", self.member)
        if char == "[":
            return self.collection("]", lambda: (None, self.value()))
        if char == '"':
            return self.string()
        return self.scalar()

    def string(self):
        start = self.at
        self.at += 1
        while self.text[self.at] != '"':
            self.at += 2 if self.text[self.at] == "\\" else 1
        self.at += 1
        out = Text(json.loads(self.text[start:self.at]))
        out.start, out.end = start, self.at
        return out

    def scalar(self):
        found = re.compile(r"-?[0-9.eE+-]+|true|false|null").match(self.text, self.at)
        self.at = found.end()
        return json.loads(found.group())

    def member(self):
        self.skip()
        key = self.string()
        self.skip()
        self.at += 1  # the colon
        return key, self.value()

    def collection(self, closing, item):
        """A JSON object (a dict) or array (a list) up to its closing bracket."""
        self.at += 1
        items = []
        while True:
            self.skip()
            if self.text[self.at] == closing:
                break
            items.append(item())
            self.skip()
            self.at += 1 if self.text[self.at] == "," else 0
        self.at += 1
        return dict(items) if closing == "}" else [value for _, value in items]


# ---- finding what to rewrite ---------------------------------------------------------------------

def is_token(node):
    return isinstance(node, dict) and "$value" in node


def replacement(kind, value):
    """(start, end, text) for a Text value that has an object form, else None."""
    if not isinstance(value, Text):
        return None
    new = rewritten(kind, value)
    return (value.start, value.end, json.dumps(new)) if new else None


def member_edits(kind, value):
    """The edits of the members of a composite token; a shadow or a gradient may be a list of them."""
    if isinstance(value, list):
        return [edit for item in value for edit in member_edits(kind, item)]
    if not isinstance(value, dict):
        return []
    edits = (replacement(sub, value.get(name)) for name, sub in COMPOSITE_MEMBERS[kind].items())
    return [edit for edit in edits if edit]


def token_edits(kind, token):
    value = token["$value"]
    if kind in COMPOSITE_MEMBERS:
        return member_edits(kind, value)
    edit = replacement(kind, value)
    return [edit] if edit else []


def edits_of(node, inherited=None):
    """All edits of a document, with the type of each token taken from itself or its nearest group."""
    kind = node.get("$type", inherited)
    out = []
    for key, child in node.items():
        if not isinstance(child, dict) or (key.startswith("$") and key != "$root"):
            continue
        out += token_edits(child.get("$type", kind), child) if is_token(child) else edits_of(child, kind)
    return out


def fix(text):
    """The text of a token file with every old value rewritten."""
    edits = edits_of(Parser(text).value())
    for start, end, new in sorted(edits, reverse=True):
        text = text[:start] + new + text[end:]
    return text


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("files", nargs="+", type=pathlib.Path)
    parser.add_argument("--check", action="store_true", help="change nothing; exit 1 for a file that would change")
    args = parser.parse_args(argv)
    stale = []
    for path in args.files:
        old = path.read_text()
        new = fix(old)
        if new != old:
            stale.append(path)
            if not args.check:
                path.write_text(new)
    for path in stale:
        print(("would rewrite " if args.check else "rewrote ") + str(path))
    return 1 if stale and args.check else 0


if __name__ == "__main__":
    sys.exit(main())
