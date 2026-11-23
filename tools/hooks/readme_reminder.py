#!/usr/bin/env python3
"""Reminds an agent to check README.md before a commit or a pull request that changes syntax or features.

Claude Code runs it before each Bash command (`.claude/settings.json`, PreToolUse) and gives it the
hook's JSON on stdin. For `git commit` it reads the staged files, and with `-a` the changed ones too.
For `gh pr create` it reads the files the branch changes against `origin/main`. When they touch
`include/lv/`, `examples/` or `docs/docs.json` and not `README.md`, it prints the reminder as context
for the agent, in the same form as the WebSearch hook.

It never blocks: any other command, and any error, prints nothing and exits 0.

It runs git only inside the project, `$CLAUDE_PROJECT_DIR`. A command that starts with `cd <dir> &&` is
read in that directory, and otherwise in the hook's `cwd`; a directory whose real path lies outside the
project, or no project at all, makes the hook print nothing without running git. A repository's
configuration can run programs (`core.fsmonitor`), so git runs with that and the hooks turned off, no
optional locks, no external diff or text conversion, and no system configuration. Uses the standard
library only.
"""

import json
import os
import re
import subprocess
import sys

REMINDER = "lvgl26: syntax or feature change - check README.md and site/index.html"
WATCHED = ("include/lv/", "examples/", "docs/docs.json")
# git, with what its configuration could make it run turned off.
GIT = ("git", "-c", "core.fsmonitor=false", "-c", "core.hooksPath=/dev/null", "--no-optional-locks")
DIFF = (*GIT, "diff", "--no-ext-diff", "--no-textconv", "--name-only")
COMMIT = re.compile(r"\bgit\s+(?:-C\s+\S+\s+)?commit\b")
ALL_CHANGES = re.compile(r"(?:^|\s)(?:-a\w*|--all)(?=\s|$)")
PR_CREATE = re.compile(r"\bgh\s+pr\s+create\b")
LEADING_CD = re.compile(r"^\s*cd\s+(\S+)\s*&&")


def needs_reminder(files: set[str]) -> bool:
    return any(f.startswith(WATCHED) for f in files) and "README.md" not in files


def diffs_for(command: str) -> list[list[str]]:
    """The `git diff` commands that list what this command commits or proposes; none for any other."""
    if COMMIT.search(command):
        diffs = [[*DIFF, "--cached"]]
        return diffs + [[*DIFF, "HEAD"]] if ALL_CHANGES.search(command) else diffs
    if PR_CREATE.search(command):
        return [[*DIFF, "origin/main...HEAD"]]
    return []


def inside(path: str, root: str) -> bool:
    return path == root or path.startswith(root.rstrip(os.sep) + os.sep)


def directory_of(command: str, cwd: str | None, project: str) -> str | None:
    """The real path the command runs in, when it lies inside the project; None otherwise."""
    moved = LEADING_CD.match(command)
    base = cwd or project
    target = os.path.expanduser(moved[1].strip("'\"")) if moved else base
    real = os.path.realpath(os.path.join(base, target))
    return real if inside(real, os.path.realpath(project)) else None


def changed_files(diffs: list[list[str]], cwd: str) -> set[str]:
    files: set[str] = set()
    environment = dict(os.environ, GIT_CONFIG_NOSYSTEM="1")
    for diff in diffs:
        out = subprocess.run(diff, cwd=cwd, env=environment, capture_output=True, text=True, timeout=10, check=False)
        files |= set(out.stdout.split())
    return files


def reminder_for(hook: dict) -> str | None:
    project = os.environ.get("CLAUDE_PROJECT_DIR")
    if hook.get("tool_name") != "Bash" or not project:
        return None
    command = hook.get("tool_input", {}).get("command", "")
    diffs = diffs_for(command)
    directory = directory_of(command, hook.get("cwd"), project) if diffs else None
    if directory is None or not needs_reminder(changed_files(diffs, directory)):
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
