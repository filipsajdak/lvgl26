#!/usr/bin/env python3
"""Reminds an agent to check README.md before a commit or a pull request that changes syntax or features.

Claude Code runs it before each Bash command (`.claude/settings.json`, PreToolUse) and gives it the
hook's JSON on stdin. For `git commit` it reads the staged files, and with `-a` the changed ones too.
For `gh pr create` it reads the files the branch changes against `origin/main`. When they touch
`include/lv/`, `examples/` or `docs/docs.json` and not `README.md`, it prints the reminder as context
for the agent, in the same form as the WebSearch hook.

It never blocks: any other command, and any error, prints nothing and exits 0. A command that starts
with `cd <dir> &&` is read in that directory; otherwise in the hook's `cwd`. Uses the standard library only.
"""

import json
import os
import re
import subprocess
import sys

REMINDER = "lvgl26: syntax or feature change - check README.md and site/index.html"
WATCHED = ("include/lv/", "examples/", "docs/docs.json")
COMMIT = re.compile(r"\bgit\s+(?:-C\s+\S+\s+)?commit\b")
ALL_CHANGES = re.compile(r"(?:^|\s)(?:-a\w*|--all)(?=\s|$)")
PR_CREATE = re.compile(r"\bgh\s+pr\s+create\b")
LEADING_CD = re.compile(r"^\s*cd\s+(\S+)\s*&&")


def needs_reminder(files: set[str]) -> bool:
    return any(f.startswith(WATCHED) for f in files) and "README.md" not in files


def diffs_for(command: str) -> list[list[str]]:
    """The `git diff` commands that list what this command commits or proposes; none for any other."""
    if COMMIT.search(command):
        diffs = [["git", "diff", "--cached", "--name-only"]]
        return diffs + [["git", "diff", "--name-only", "HEAD"]] if ALL_CHANGES.search(command) else diffs
    if PR_CREATE.search(command):
        return [["git", "diff", "--name-only", "origin/main...HEAD"]]
    return []


def directory_of(command: str, cwd: str) -> str:
    moved = LEADING_CD.match(command)
    return os.path.expanduser(moved[1].strip("'\"")) if moved else cwd


def changed_files(diffs: list[list[str]], cwd: str) -> set[str]:
    files: set[str] = set()
    for diff in diffs:
        out = subprocess.run(diff, cwd=cwd, capture_output=True, text=True, timeout=10, check=False)
        files |= set(out.stdout.split())
    return files


def reminder_for(hook: dict) -> str | None:
    if hook.get("tool_name") != "Bash":
        return None
    command = hook.get("tool_input", {}).get("command", "")
    diffs = diffs_for(command)
    if not diffs or not needs_reminder(changed_files(diffs, directory_of(command, hook.get("cwd") or "."))):
        return None
    return json.dumps({"hookSpecificOutput": {"hookEventName": "PreToolUse", "additionalContext": REMINDER}})


def main() -> int:
    try:
        found = reminder_for(json.load(sys.stdin))
    except Exception:  # a reminder must never stop a command
        return 0
    if found:
        print(found)
    return 0


if __name__ == "__main__":
    sys.exit(main())
