#!/usr/bin/env python3
"""Checks the settings files of the repository against docs/reference/lvgl26-settings.schema.json,
and the keys that the setup skill names against docs/reference/settings.json.

    python3 tools/check_settings.py

Needs the jsonschema package. Both reference files are printed by tools/reference/settings.cpp.
"""
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
SCHEMA = ROOT / "docs/reference/lvgl26-settings.schema.json"
REFERENCE = ROOT / "docs/reference/settings.json"
SKILL = ROOT / "skills/settings/SKILL.md"
FILES = sorted([*ROOT.glob("examples/settings/presets/*.json"), *ROOT.glob("tests/settings/*.json")])
# A key as the skill writes it: allow.threads, limits.compile_time_image.
KEY = re.compile(r"`((?:allow|rules|limits)\.[a-z_]+)`")
# Mistakes the schema must refuse, as an editor would mark them.
REFUSED = [{"allow": {"thread": False}}, {"alow": {}}, {"limits": {"compile_time_image": {"value": 1, "unit": "pix"}}},
           {"allow": {"threads": "no"}}]


def validator():
    import jsonschema
    schema = json.loads(SCHEMA.read_text())
    jsonschema.Draft202012Validator.check_schema(schema)
    return jsonschema.Draft202012Validator(schema)


def file_problems(check) -> list[str]:
    problems = []
    for path in FILES:
        for error in check.iter_errors(json.loads(path.read_text())):
            problems.append(f"{path.relative_to(ROOT)}: {error.message}")
    for document in REFUSED:
        if check.is_valid(document):
            problems.append(f"the schema accepts {json.dumps(document)}")
    return problems


def known_keys() -> set[str]:
    reference = json.loads(REFERENCE.read_text())
    return {f"{section['key']}.{s['key']}" for section in reference["sections"] for s in section["settings"]}


def skill_problems() -> list[str]:
    if not SKILL.exists():
        return [f"{SKILL.relative_to(ROOT)} is missing"]
    named = set(KEY.findall(SKILL.read_text()))
    return [f"{SKILL.relative_to(ROOT)} names {key}, which docs/reference/settings.json does not have"
            for key in sorted(named - known_keys())]


def main() -> None:
    problems = file_problems(validator()) + skill_problems()
    for problem in problems:
        print(problem)
    if problems:
        sys.exit(1)
    print(f"{len(FILES)} settings files match the schema; the skill names only known keys")


if __name__ == "__main__":
    main()
