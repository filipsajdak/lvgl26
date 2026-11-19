#!/usr/bin/env python3
"""Keeps the repository's copy of Signal's design file in step with the design tool.

    python3 tools/signal_sync.py                 the check: fails when the copy, its record or the written files drift
    python3 tools/signal_sync.py --record        writes design/signal/source.json from the copy
    python3 tools/signal_sync.py --compare FILE  lists the differences between the copy and FILE, in both directions

Signal's source is a design system in Claude Design. design/signal/tokens.design.json is its copy, and
design/signal/source.json records which version of the design the copy holds and a SHA-256 of its content
(the JSON read back with sorted keys and no white space, so a change of layout does not count). The check fails when

- the version of the copy is not the recorded one, or its hash is not the recorded one: the copy was edited
  without `--record`, or `--record` was not run after a sync;
- a file that tools/signal_tokens.py writes is not what the tool writes from the copy.

What the check cannot see is a change made in the design tool: it reads no network and holds no login. Compare
with a copy of the design tool's tokens.json (`--compare`), made by whoever has access; skills/sync-signal says how.

Uses the standard library only. Exits with 1 and a list of problems if any.
"""

import argparse
import datetime
import hashlib
import json
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import signal_tokens  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parent.parent
DESIGN = signal_tokens.SOURCE
RECORD = ROOT / "design" / "signal" / "source.json"
SHOWN_VALUE = 60


def canonical(design: dict) -> str:
    """The design with sorted keys and no white space: the same content gives the same text."""
    return json.dumps(design, sort_keys=True, separators=(",", ":"), ensure_ascii=True)


def digest(design: dict) -> str:
    return hashlib.sha256(canonical(design).encode("ascii")).hexdigest()


def record_of(design: dict, read_on: str) -> dict:
    return {"version": design["version"], "sha256": digest(design), "read": read_on}


def record_problems(design: dict, record: dict) -> list[str]:
    problems = []
    if record.get("version") != design.get("version"):
        problems.append(f"the copy is version {design.get('version')} and the record says {record.get('version')}")
    if record.get("sha256") != digest(design):
        problems.append("the content of the copy is not the recorded one (edited without --record, or a sync was not recorded)")
    return problems


def written_problems() -> list[str]:
    """The files tools/signal_tokens.py writes that are not what it writes from the copy."""
    design = json.loads(DESIGN.read_text())
    stale = []
    for name, text in signal_tokens.documents(design).items():
        path = signal_tokens.OUT / name
        if not path.exists() or path.read_text() != text:
            stale.append(f"{path.name} is stale: run python3 tools/signal_tokens.py")
    return stale


def shown(value: object) -> str:
    text = json.dumps(value, sort_keys=True, ensure_ascii=True)
    return text if len(text) <= SHOWN_VALUE else text[:SHOWN_VALUE] + "..."


def name_of(item: object, index: int) -> str:
    """How a list entry is named in a path: by its "name" or "family" and "file", else by its place."""
    if isinstance(item, dict):
        for key in ("name", "file"):
            if key in item:
                return str(item[key])
    return str(index)


def keyed(items: list) -> dict:
    return {name_of(item, index): item for index, item in enumerate(items)}


def children(node: object) -> dict | None:
    if isinstance(node, dict):
        return node
    if isinstance(node, list):
        return keyed(node)
    return None


def differences(ours: object, theirs: object, path: str = "") -> list[str]:
    """Where `ours` (the copy) and `theirs` (the design tool) differ, each line says which side has it."""
    ours_children, theirs_children = children(ours), children(theirs)
    if ours_children is None or theirs_children is None or type(ours) is not type(theirs):
        return [] if ours == theirs else [f"{path or '/'}: copy {shown(ours)}, design tool {shown(theirs)}"]
    out = []
    for key in sorted(set(ours_children) | set(theirs_children)):
        here = f"{path}/{key}"
        if key not in theirs_children:
            out.append(f"{here}: only in the copy: {shown(ours_children[key])}")
        elif key not in ours_children:
            out.append(f"{here}: only in the design tool: {shown(theirs_children[key])}")
        else:
            out += differences(ours_children[key], theirs_children[key], here)
    return out


def check() -> list[str]:
    design = json.loads(DESIGN.read_text())
    if not RECORD.exists():
        return [f"{RECORD.relative_to(ROOT)} is missing: run python3 tools/signal_sync.py --record"]
    return record_problems(design, json.loads(RECORD.read_text())) + written_problems()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--record", action="store_true", help="write design/signal/source.json from the copy")
    parser.add_argument("--read-on", default=datetime.date.today().isoformat(), help="the date the design tool was read (with --record)")
    parser.add_argument("--compare", type=pathlib.Path, help="a copy of the design tool's tokens.json")
    args = parser.parse_args(argv)
    design = json.loads(DESIGN.read_text())
    if args.record:
        RECORD.write_text(json.dumps(record_of(design, args.read_on), indent=2) + "\n")
        print(f"wrote {RECORD.relative_to(ROOT)}: version {design['version']}")
        return 0
    if args.compare:
        found = differences(design, json.loads(args.compare.read_text()))
        print("\n".join(found) if found else "the copy and the design tool's file are the same")
        return 1 if found else 0
    problems = check()
    print("\n".join(problems) if problems else f"Signal version {design['version']}: the copy, its record and the written files agree")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
