#!/usr/bin/env python3
"""Unit tests of the nightly parts of tools/mutation.py, tools/mutation_plan.py and
tools/mutation_summary.py: python3 tools/test_mutation.py"""

import json
import pathlib
import subprocess
import sys
import tempfile
import unittest

TOOLS = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))

import mutation  # noqa: E402
import mutation_plan as plan  # noqa: E402
import mutation_summary as summary  # noqa: E402

CONFIG = {"default": {"tests": ".", "targets": None},
          "headers": {"include/lv/a.hpp": {"tests": "^a$", "targets": "a"}}}


def git(root, *args):
    subprocess.run(["git", "-c", "user.name=t", "-c", "user.email=t@t", *args], cwd=root, check=True,
                   capture_output=True)


def repository():
    """A repository with two commits: the second changes a.hpp, adds detail/b.hpp, deletes c.hpp."""
    root = pathlib.Path(tempfile.mkdtemp())
    (root / "include/lv/detail").mkdir(parents=True)
    (root / "include/lv/a.hpp").write_text("1\n")
    (root / "include/lv/c.hpp").write_text("1\n")
    (root / "include/lv/data.json").write_text("1\n")
    git(root, "init", "-q")
    git(root, "add", ".")
    git(root, "commit", "-qm", "base")
    (root / "include/lv/a.hpp").write_text("2\n")
    (root / "include/lv/detail/b.hpp").write_text("1\n")
    (root / "include/lv/data.json").write_text("2\n")
    (root / "include/lv/c.hpp").unlink()
    git(root, "add", "-A")
    git(root, "commit", "-qm", "change")
    return root


class Plan(unittest.TestCase):
    def test_lists_changed_headers_that_still_exist(self):
        root = repository()
        self.assertEqual(plan.changed_headers("HEAD~1", root), ["include/lv/a.hpp", "include/lv/detail/b.hpp"])

    def test_a_header_whose_name_could_be_read_as_a_command_ends_the_plan(self):
        root = repository()
        (root / "include/lv/x$(id).hpp").write_text("1\n")
        git(root, "add", "-A")
        git(root, "commit", "-qm", "odd name")
        with self.assertRaises(SystemExit) as stopped:
            plan.changed_headers("HEAD~1", root)
        self.assertIn("x$(id).hpp", str(stopped.exception))

    def test_nothing_changed_is_no_work(self):
        root = repository()
        result = plan.plan("HEAD", CONFIG, root)
        self.assertFalse(result["any"])
        self.assertEqual(result["matrix"], {"include": []})

    def test_a_listed_header_gets_its_tests_and_an_unlisted_one_the_default(self):
        root = repository()
        shards = plan.plan("HEAD~1", CONFIG, root)["matrix"]["include"]
        self.assertEqual(shards[0], {"name": "a", "header": "include/lv/a.hpp", "tests": "^a$", "targets": "a",
                                     "fallback": False})
        self.assertEqual(shards[1], {"name": "detail-b", "header": "include/lv/detail/b.hpp", "tests": ".",
                                     "targets": "", "fallback": True})

    def test_github_output_lines(self):
        root = repository()
        out = pathlib.Path(tempfile.mkdtemp()) / "out"
        plan.write_github_output(out, plan.plan("HEAD~1", CONFIG, root))
        lines = out.read_text().splitlines()
        self.assertEqual(lines[0], "any=true")
        self.assertEqual(lines[1], "since=HEAD~1")
        self.assertEqual(len(json.loads(lines[2].removeprefix("matrix="))["include"]), 2)

    def test_the_real_config_names_a_test_for_every_listed_header(self):
        config = json.loads(plan.CONFIG.read_text())
        for header, chosen in config["headers"].items():
            self.assertTrue((plan.ROOT / header).exists(), header)
            self.assertTrue(chosen["tests"] and chosen["targets"], header)


class Accepted(unittest.TestCase):
    def test_a_mutant_is_matched_without_its_line_number(self):
        self.assertEqual(mutation.mutant_text("line 12: a == b  ->  a != b"), "a == b  ->  a != b")
        self.assertEqual(mutation.mutant_text("(a line was removed or added)"), "(a line was removed or added)")

    def test_only_the_accepted_rows_of_the_header_count(self):
        path = pathlib.Path(tempfile.mkdtemp()) / "accepted.json"
        path.write_text(json.dumps([{"header": "h1", "mutant": "x  ->  y", "reason": "equivalent"},
                                    {"header": "h2", "mutant": "p  ->  q", "reason": "arid"}]))
        self.assertEqual(mutation.accepted_for(str(path), "h1"), {"x  ->  y": "equivalent"})
        self.assertEqual(mutation.accepted_for(None, "h1"), {})
        self.assertEqual(mutation.accepted_for(str(path.with_name("none.json")), "h1"), {})


class Result(unittest.TestCase):
    def row(self, **kw):
        base = {"header": "include/lv/a.hpp", "since": "abc", "mutants": 10, "judged": 10, "complete": True,
                "killed": 9, "survived": 0, "accepted": 1, "invalid": 0, "survivors": []}
        return {**base, **kw}

    def test_exit_codes(self):
        self.assertEqual(mutation.exit_code(self.row()), 0)
        self.assertEqual(mutation.exit_code(self.row(survivors=["line 1: a  ->  b"])), 1)
        self.assertEqual(mutation.exit_code(self.row(complete=False, judged=4)), 2)
        self.assertEqual(mutation.exit_code(self.row(complete=False, survivors=["s"])), 1)

    def test_result_row_counts_accepted_survivors_apart(self):
        class Args:
            header = "h"
            since = "s"
        row = mutation.result_row(Args, {"killed": 5, "survived": 3, "invalid": 1}, ["one"], ["a", "b"], 9, 12)
        self.assertEqual((row["survived"], row["accepted"], row["complete"]), (1, 2, False))

    def test_summary_names_survivors_and_what_was_not_covered(self):
        text = summary.summary(self.row(complete=False, judged=4, survived=1, survivors=["line 3: a  ->  b"]))
        self.assertIn("1 survived", text)
        self.assertIn("4 of 10 mutants were judged; the rest were not reached", text)
        self.assertNotIn("no entry in tools/mutation_tests.json", text)
        self.assertIn("a.hpp has no entry in tools/mutation_tests.json",
                      summary.summary(self.row(), fallback=True))
        self.assertIn("line 3: a  ->  b", text)
        self.assertIn("mutation_accepted.json", text)
        self.assertNotIn("```", summary.summary(self.row()))

    def test_judge_all_stops_at_the_time_budget_and_puts_the_header_back(self):
        tree = pathlib.Path(tempfile.mkdtemp())
        (tree / "h.hpp").write_text("mutant")
        mutant = tree / "m.hpp"
        mutant.write_text("mutant")

        class Args:
            header = "h.hpp"
            time_budget = -1  # spent before the first mutant
            preset = "p"
            tests = "t"
            targets = None
            test_timeout = 1
        old, mutation.TREE = mutation.TREE, tree
        try:
            result = mutation.judge_all(Args, "original", [mutant], {})
        finally:
            mutation.TREE = old
        self.assertEqual(result, ({"killed": 0, "survived": 0, "invalid": 0}, [], []))
        self.assertEqual((tree / "h.hpp").read_text(), "original")


if __name__ == "__main__":
    unittest.main()
