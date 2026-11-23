#!/usr/bin/env python3
"""Tests of tools/hooks/readme_reminder.py. Run: python3 tools/test_readme_reminder.py

The end-to-end tests make a git repository of their own in a temporary directory, so they can run
beside every other test."""

import json
import pathlib
import subprocess
import sys
import tempfile
import unittest

HOOK = pathlib.Path(__file__).resolve().parent / "hooks" / "readme_reminder.py"
sys.path.insert(0, str(HOOK.parent))
import readme_reminder as rr  # noqa: E402


def git(repo: pathlib.Path, *args: str) -> None:
    subprocess.run(["git", "-c", "user.name=t", "-c", "user.email=t@example.org", "-c", "commit.gpgsign=false",
                    *args], cwd=repo, check=True, capture_output=True)


def write(repo: pathlib.Path, name: str, text: str = "x\n") -> None:
    path = repo / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)


def run_hook(hook: dict | str) -> subprocess.CompletedProcess:
    text = hook if isinstance(hook, str) else json.dumps(hook)
    return subprocess.run([sys.executable, str(HOOK)], input=text, capture_output=True, text=True, check=False)


class Decision(unittest.TestCase):
    def test_a_change_of_the_library_or_an_example_without_the_readme_needs_it(self):
        self.assertTrue(rr.needs_reminder({"include/lv/tree.hpp"}))
        self.assertTrue(rr.needs_reminder({"examples/state/main.cpp"}))
        self.assertTrue(rr.needs_reminder({"docs/docs.json"}))

    def test_a_change_with_the_readme_or_elsewhere_does_not(self):
        self.assertFalse(rr.needs_reminder({"include/lv/tree.hpp", "README.md"}))
        self.assertFalse(rr.needs_reminder({"docs/state.md", "tools/docs.py"}))

    def test_only_a_commit_and_a_pull_request_are_read(self):
        self.assertEqual(rr.diffs_for("git commit -q -F msg.txt"), [["git", "diff", "--cached", "--name-only"]])
        self.assertEqual(len(rr.diffs_for("git commit -am 'fix'")), 2)
        self.assertEqual(len(rr.diffs_for("git commit --amend --no-edit")), 1)
        self.assertEqual(rr.diffs_for("gh pr create --draft"), [["git", "diff", "--name-only", "origin/main...HEAD"]])
        self.assertEqual(rr.diffs_for("git status && git log"), [])

    def test_a_leading_cd_names_the_directory(self):
        self.assertEqual(rr.directory_of("cd /tmp/x && git commit", "/here"), "/tmp/x")
        self.assertEqual(rr.directory_of("git commit", "/here"), "/here")


class EndToEnd(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.repo = pathlib.Path(self.temp.name)
        git(self.repo, "init", "-q")
        write(self.repo, "README.md")
        git(self.repo, "add", ".")
        git(self.repo, "commit", "-q", "-m", "start")

    def tearDown(self):
        self.temp.cleanup()

    def commit_hook(self, command: str = "git commit -m change") -> dict:
        return {"tool_name": "Bash", "tool_input": {"command": command}, "cwd": str(self.repo)}

    def test_a_staged_change_of_the_library_prints_the_reminder(self):
        write(self.repo, "include/lv/tree.hpp")
        git(self.repo, "add", ".")
        out = run_hook(self.commit_hook())
        self.assertEqual(out.returncode, 0)
        self.assertEqual(json.loads(out.stdout), {"hookSpecificOutput": {"hookEventName": "PreToolUse",
                                                                         "additionalContext": rr.REMINDER}})

    def test_the_readme_in_the_same_commit_prints_nothing(self):
        write(self.repo, "include/lv/tree.hpp")
        write(self.repo, "README.md", "changed\n")
        git(self.repo, "add", ".")
        self.assertEqual(run_hook(self.commit_hook()).stdout, "")

    def test_a_cd_before_the_commit_is_followed(self):
        write(self.repo, "examples/a.cpp")
        git(self.repo, "add", ".")
        hook = {"tool_name": "Bash", "tool_input": {"command": f"cd {self.repo} && git commit -m x"}, "cwd": "/"}
        self.assertIn(rr.REMINDER, run_hook(hook).stdout)

    def test_a_pull_request_reads_the_branch(self):
        git(self.repo, "update-ref", "refs/remotes/origin/main", "HEAD")
        write(self.repo, "docs/docs.json", "{}\n")
        git(self.repo, "add", ".")
        git(self.repo, "commit", "-q", "-m", "sidebar")
        self.assertIn(rr.REMINDER, run_hook(self.commit_hook("gh pr create --fill")).stdout)

    def test_it_never_blocks(self):
        for hook in ("not json", {"tool_name": "WebSearch"}, {"tool_name": "Bash", "tool_input": {"command": "ls"}},
                     {"tool_name": "Bash", "tool_input": {"command": "git commit"}, "cwd": "/no/such/dir"}):
            out = run_hook(hook)
            self.assertEqual((out.returncode, out.stdout), (0, ""))


if __name__ == "__main__":
    unittest.main()
