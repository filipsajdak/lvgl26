#!/usr/bin/env python3
"""Checks the Claude Code plugin of this repository.

    python3 tools/check_plugin.py

- `.claude-plugin/plugin.json` and `marketplace.json` parse and name the plugin.
- Every skill has a `SKILL.md` with `name` and `description` in its front matter.
- Every relative link in a skill resolves to a file. A skill points to the
  repository's files; it does not copy them, so a moved file must fail here.

Uses the standard library only. Exits with 1 and a list of problems if any.
"""

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
LINK = re.compile(r"\]\(([^)#\s]+)(?:#[^)]*)?\)")


def manifest_problems() -> list[str]:
    problems = []
    for name in ("plugin.json", "marketplace.json"):
        path = ROOT / ".claude-plugin" / name
        try:
            data = json.loads(path.read_text())
        except (OSError, ValueError) as error:
            problems.append(f"{path.relative_to(ROOT)}: {error}")
            continue
        if not data.get("name"):
            problems.append(f"{path.relative_to(ROOT)}: no name")
    return problems


def front_matter(text: str) -> dict[str, str]:
    """The `key: value` lines between the first two `---` lines."""
    if not text.startswith("---\n") or "\n---\n" not in text[4:]:
        return {}
    block = text[4:text.index("\n---\n", 4)]
    pairs = (line.split(":", 1) for line in block.splitlines() if ":" in line)
    return {key.strip(): value.strip() for key, value in pairs}


def link_problems(skill: pathlib.Path, text: str) -> list[str]:
    targets = (t for t in LINK.findall(text) if "://" not in t)
    return [f"{skill.relative_to(ROOT)}: link to {t} does not resolve"
            for t in targets if not (skill.parent / t).exists()]


def one_skill_problems(folder: pathlib.Path) -> list[str]:
    skill = folder / "SKILL.md"
    if not skill.exists():
        return [f"skills/{folder.name}: no SKILL.md"]
    text = skill.read_text()
    fields = front_matter(text)
    missing = [f"skills/{folder.name}/SKILL.md: no {key} in the front matter"
               for key in ("name", "description") if not fields.get(key)]
    return missing + link_problems(skill, text)


def skill_problems() -> list[str]:
    folders = [f for f in sorted((ROOT / "skills").iterdir()) if f.is_dir()]
    if not folders:
        return ["skills/: no skill"]
    return [problem for folder in folders for problem in one_skill_problems(folder)]


def main() -> None:
    problems = manifest_problems() + skill_problems()
    for problem in problems:
        print(problem)
    skills = len(list((ROOT / "skills").glob("*/SKILL.md")))
    print(f"{skills} skill(s), {len(problems)} problem(s)")
    sys.exit(1 if problems else 0)


if __name__ == "__main__":
    main()
