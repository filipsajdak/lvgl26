#!/usr/bin/env python3
"""Tests of tools/hooks/readme_reminder.py. Run: python3 tools/test_readme_reminder.py

The end-to-end tests make a git repository of their own in a temporary directory, so they can run
beside every other test."""

import json
import os
import pathlib
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

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


def run_hook(hook: dict | str, project: pathlib.Path | None) -> subprocess.CompletedProcess:
    text = hook if isinstance(hook, str) else json.dumps(hook)
    environment = {k: v for k, v in os.environ.items() if k != "CLAUDE_PROJECT_DIR"}
    if project is not None:
        environment["CLAUDE_PROJECT_DIR"] = str(project)
    return subprocess.run([sys.executable, str(HOOK)], input=text, env=environment, capture_output=True, text=True,
                          check=False)


class Decision(unittest.TestCase):
    def test_a_change_of_the_library_or_an_example_without_the_readme_needs_it(self):
        self.assertTrue(rr.needs_reminder({"include/lv/tree.hpp"}))
        self.assertTrue(rr.needs_reminder({"examples/state/main.cpp"}))
        self.assertTrue(rr.needs_reminder({"docs/docs.json"}))

    def test_a_change_with_the_readme_or_elsewhere_does_not(self):
        self.assertFalse(rr.needs_reminder({"include/lv/tree.hpp", "README.md"}))
        self.assertFalse(rr.needs_reminder({"docs/state.md", "tools/docs.py"}))

    def test_only_a_commit_and_a_pull_request_are_read(self):
        self.assertEqual(rr.diffs_for("git commit -q -F msg.txt"), [[*rr.DIFF, "--cached"]])
        self.assertEqual(len(rr.diffs_for("git commit -am 'fix'")), 2)
        self.assertEqual(len(rr.diffs_for("git commit --amend --no-edit")), 1)
        self.assertEqual(rr.diffs_for("gh pr create --draft"), [[*rr.DIFF, "origin/main...HEAD"]])
        self.assertEqual(rr.diffs_for("git status && git log"), [])

    def test_git_runs_with_what_its_configuration_could_run_turned_off(self):
        for diff in rr.diffs_for("git commit -a"):
            self.assertEqual(diff[:6], ["git", "-c", "core.fsmonitor=false", "-c", "core.hooksPath=/dev/null",
                                        "--no-optional-locks"])
            self.assertIn("--no-ext-diff", diff)
            self.assertIn("--no-textconv", diff)

    def test_a_directory_inside_the_project_is_read_and_one_outside_is_not(self):
        with tempfile.TemporaryDirectory() as root:
            project = os.path.realpath(root)
            os.mkdir(os.path.join(project, "sub"))
            self.assertEqual(rr.directory_of(f"cd {project}/sub && git commit", project, project), f"{project}/sub")
            self.assertEqual(rr.directory_of("cd sub && git commit", project, project), f"{project}/sub")
            self.assertEqual(rr.directory_of("git commit", f"{project}/sub", project), f"{project}/sub")
            self.assertIsNone(rr.directory_of("cd /tmp && git commit", project, project))
            self.assertIsNone(rr.directory_of("cd .. && git commit", project, project))
            self.assertIsNone(rr.directory_of("git commit", "/", project))


class NoGitOutside(unittest.TestCase):
    """In the process, with git replaced: what the hook would run, and where."""

    def reminder(self, command: str, project: str | None, cwd: str) -> tuple[str | None, mock.MagicMock]:
        found = subprocess.CompletedProcess([], 0, stdout="include/lv/tree.hpp\n", stderr="")
        environment = {"CLAUDE_PROJECT_DIR": project} if project else {}
        with mock.patch.dict(os.environ, environment, clear=False), \
             mock.patch.object(rr.subprocess, "run", return_value=found) as run:
            if project is None:
                os.environ.pop("CLAUDE_PROJECT_DIR", None)
            return rr.reminder_for({"tool_name": "Bash", "tool_input": {"command": command}, "cwd": cwd}), run

    def test_a_cd_outside_the_project_runs_no_git(self):
        with tempfile.TemporaryDirectory() as project, tempfile.TemporaryDirectory() as other:
            found, run = self.reminder(f"cd {other} && git commit -m x", project, project)
            self.assertIsNone(found)
            run.assert_not_called()

    def test_without_a_project_no_git_runs(self):
        with tempfile.TemporaryDirectory() as here:
            found, run = self.reminder("git commit -m x", None, here)
            self.assertIsNone(found)
            run.assert_not_called()

    def test_inside_the_project_git_runs_there_without_the_system_configuration(self):
        with tempfile.TemporaryDirectory() as project:
            found, run = self.reminder("git commit -m x", project, project)
            self.assertIn(rr.REMINDER, found)
            arguments, options = run.call_args
            self.assertEqual(arguments[0][:6], list(rr.GIT))
            self.assertEqual(options["cwd"], os.path.realpath(project))
            self.assertEqual(options["env"]["GIT_CONFIG_NOSYSTEM"], "1")


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
        out = run_hook(self.commit_hook(), self.repo)
        self.assertEqual(out.returncode, 0)
        self.assertEqual(json.loads(out.stdout), {"hookSpecificOutput": {"hookEventName": "PreToolUse",
                                                                         "additionalContext": rr.REMINDER}})

    def test_the_readme_in_the_same_commit_prints_nothing(self):
        write(self.repo, "include/lv/tree.hpp")
        write(self.repo, "README.md", "changed\n")
        git(self.repo, "add", ".")
        self.assertEqual(run_hook(self.commit_hook(), self.repo).stdout, "")

    def test_a_cd_before_the_commit_is_followed(self):
        write(self.repo, "examples/a.cpp")
        git(self.repo, "add", ".")
        hook = {"tool_name": "Bash", "tool_input": {"command": f"cd {self.repo} && git commit -m x"}, "cwd": "/"}
        self.assertIn(rr.REMINDER, run_hook(hook, self.repo).stdout)
        self.assertEqual(run_hook(hook, self.repo / "include").stdout, "")  # the repository is outside that project

    def test_a_pull_request_reads_the_branch(self):
        git(self.repo, "update-ref", "refs/remotes/origin/main", "HEAD")
        write(self.repo, "docs/docs.json", "{}\n")
        git(self.repo, "add", ".")
        git(self.repo, "commit", "-q", "-m", "sidebar")
        self.assertIn(rr.REMINDER, run_hook(self.commit_hook("gh pr create --fill"), self.repo).stdout)

    def test_it_never_blocks(self):
        for hook in ("not json", {"tool_name": "WebSearch"}, {"tool_name": "Bash", "tool_input": {"command": "ls"}},
                     {"tool_name": "Bash", "tool_input": {"command": "git commit"}, "cwd": "/no/such/dir"}):
            out = run_hook(hook, self.repo)
            self.assertEqual((out.returncode, out.stdout), (0, ""))
        write(self.repo, "include/lv/tree.hpp")
        git(self.repo, "add", ".")
        out = run_hook(self.commit_hook(), None)  # no project: no git
        self.assertEqual((out.returncode, out.stdout), (0, ""))


if __name__ == "__main__":
    unittest.main()
