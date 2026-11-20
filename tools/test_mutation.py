#!/usr/bin/env python3
"""Unit tests of the nightly parts of tools/mutation.py, tools/mutation_plan.py and
tools/mutation_summary.py: python3 tools/test_mutation.py"""

import json
import pathlib
import re
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
                                     "fallback": False, "logic": False, "part": ""})
        self.assertEqual(shards[1], {"name": "detail-b", "header": "include/lv/detail/b.hpp", "tests": ".",
                                     "targets": "", "fallback": True, "logic": False, "part": ""})

    def test_a_header_with_parts_is_one_shard_for_each_part_and_keeps_its_logic_flag(self):
        config = {"default": CONFIG["default"], "headers": {
            "include/lv/a.hpp": {"tests": "^a$", "targets": "a", "logic": True, "parts": 2}}}
        shards = plan.plan("HEAD~1", config, repository())["matrix"]["include"]
        self.assertEqual([(s["name"], s["part"], s["logic"]) for s in shards[:2]],
                         [("a-1of2", "1/2", True), ("a-2of2", "2/2", True)])
        self.assertEqual(len(shards), 3)  # and the unlisted detail/b.hpp

    def test_the_real_config_splits_the_slow_headers_and_runs_five_headers_logic_only(self):
        headers = json.loads(plan.CONFIG.read_text())["headers"]
        logic = sorted(h for h, c in headers.items() if c.get("logic"))
        self.assertEqual(logic, ["include/lv/look.hpp", "include/lv/one_of.hpp", "include/lv/style.hpp",
                                 "include/lv/tree.hpp", "include/lv/values.hpp"])
        self.assertEqual({h: c["parts"] for h, c in headers.items() if "parts" in c},
                         {"include/lv/look.hpp": 2, "include/lv/props.hpp": 2, "include/lv/tokens.hpp": 2})

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


def registered_tests():
    """(ctest names, build targets) that tests/CMakeLists.txt registers, read from its text.
    A compile-fail test is the ctest `fail_<name>` and has no target of its own to build."""
    text = (plan.ROOT / "tests/CMakeLists.txt").read_text()
    targets = set(re.findall(r"^\s*(?:lvgl26_test|add_executable)\((\w+)", text, re.M))
    names = set(targets) | set(re.findall(r"add_test\(NAME (\w+)", text))
    names |= {"fail_" + n for n in re.findall(r"^\s*lvgl26_compile_fail(?:_message)?\((\w+)", text, re.M)}
    return names, targets


class MappingMatchesTheTests(unittest.TestCase):
    """A pattern that matches no test, or a target that does not exist, makes a shard of the
    nightly run stop on its baseline or judge a header by nothing (owned.hpp, tokens.hpp)."""

    def test_every_alternative_of_every_pattern_matches_a_registered_test(self):
        names, _ = registered_tests()
        config = json.loads(plan.CONFIG.read_text())
        for header, chosen in config["headers"].items():
            for alternative in chosen["tests"].split("|"):
                self.assertTrue(any(re.search(alternative, n) for n in names), f"{header}: {alternative}")

    def test_every_target_exists(self):
        _, targets = registered_tests()
        config = json.loads(plan.CONFIG.read_text())
        for header, chosen in config["headers"].items():
            for target in chosen["targets"].split():
                self.assertIn(target, targets, header)

    def test_the_reader_finds_both_kinds_of_test(self):
        names, targets = registered_tests()
        self.assertTrue({"tokens", "fail_token_bad_color", "signal_tokens_check"} <= names)
        self.assertIn("tokens", targets)
        self.assertNotIn("fail_token_bad_color", targets)


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


class AcceptedFile(unittest.TestCase):
    def test_every_triage_names_a_header_a_mutant_and_a_reason(self):
        rows = json.loads((plan.TOOLS / "mutation_accepted.json").read_text())
        for row in rows:
            self.assertTrue((plan.ROOT / row["header"]).exists(), row)
            self.assertIn("  ->  ", row["mutant"])
            self.assertTrue(row["reason"].strip(), row)

    def test_the_triaged_mutant_of_runtime_hpp_is_matched_as_the_run_describes_it(self):
        accepted = mutation.accepted_for(str(plan.TOOLS / "mutation_accepted.json"), "include/lv/runtime.hpp")
        self.assertIn(mutation.mutant_text("line 507: }  ->  continue;"), accepted)


class ChildLocale(unittest.TestCase):
    def test_a_command_runs_in_the_c_locale_so_that_gcc_writes_plain_quotes(self):
        code, output = mutation.run("echo $LC_ALL", cwd=".")
        self.assertEqual((code, output.strip()), (0, "C"))


class Baseline(unittest.TestCase):
    def test_a_failed_baseline_shows_the_verdict_and_the_end_of_the_output(self):
        output = "\n".join(f"line {n}" for n in range(mutation.BASELINE_TAIL_LINES + 10))
        text = mutation.baseline_failure("invalid", output)
        self.assertIn("do not pass on the unchanged header", text)
        self.assertIn("(invalid)", text)
        self.assertIn(f"line {mutation.BASELINE_TAIL_LINES + 9}", text)
        self.assertNotIn("line 9\n", text)

    def test_judge_with_output_says_which_step_decided(self):
        steps = iter([(1, "error: expected ';'"), (0, "built"), (1, "1 test failed")])
        old, mutation.run = mutation.run, lambda command, cwd=None: next(steps)
        try:
            self.assertEqual(mutation.judge_with_output("p", "t", None, 1), ("invalid", "error: expected ';'"))
            self.assertEqual(mutation.judge_with_output("p", "t", None, 1), ("killed", "1 test failed"))
        finally:
            mutation.run = old


class TrackFiles(unittest.TestCase):
    def test_the_copy_lists_its_files_to_git(self):
        with tempfile.TemporaryDirectory() as directory:
            tree = pathlib.Path(directory)
            (tree / "tokens.json").write_text("{}")
            mutation.track_files(tree)
            listed = subprocess.run(["git", "ls-files", "*.json"], cwd=tree, capture_output=True, text=True, check=True)
            self.assertEqual(listed.stdout.split(), ["tokens.json"])

    def test_a_second_run_sees_a_file_that_is_gone(self):
        with tempfile.TemporaryDirectory() as directory:
            tree = pathlib.Path(directory)
            (tree / "a.json").write_text("{}")
            mutation.track_files(tree)
            (tree / "a.json").unlink()
            mutation.track_files(tree)
            listed = subprocess.run(["git", "ls-files"], cwd=tree, capture_output=True, text=True, check=True)
            self.assertEqual(listed.stdout.split(), [])


class Parts(unittest.TestCase):
    def test_the_parts_are_disjoint_and_cover_every_mutant(self):
        mutants = list(range(7))
        parts = [mutation.mutants_of_part(mutants, f"{k}/3") for k in (1, 2, 3)]
        self.assertEqual(sorted(m for part in parts for m in part), mutants)
        self.assertEqual(parts[0], [0, 3, 6])

    def test_a_part_out_of_range_stops_the_run(self):
        for bad in ("0/2", "3/2", "x/2", "1", "1/"):
            with self.assertRaises(SystemExit):
                mutation.mutants_of_part([1, 2], bad)


class CommentMutants(unittest.TestCase):
    ORIGINAL = "int a = 1;\n  // a note\nint b = 2;\n"

    def mutants(self, directory, texts):
        out = []
        for i, text in enumerate(texts):
            path = pathlib.Path(directory, f"m{i}.hpp")
            path.write_text(text)
            out.append(path)
        return out

    def test_a_mutant_of_a_comment_line_is_dropped_and_one_of_code_is_kept(self):
        with tempfile.TemporaryDirectory() as directory:
            comment = "int a = 1;\n  /*// a note*/\nint b = 2;\n"
            code = "int a = 1;\n  // a note\nint b = 3;\n"
            kept = mutation.without_comment_mutants(self.mutants(directory, [comment, code]), self.ORIGINAL)
            self.assertEqual([m.name for m in kept], ["m1.hpp"])

    def test_a_trailing_comment_does_not_make_a_line_a_comment(self):
        with tempfile.TemporaryDirectory() as directory:
            original = "int a = 1;  // one\n"
            kept = mutation.without_comment_mutants(self.mutants(directory, ["int a = 2;  // one\n"]), original)
            self.assertEqual(len(kept), 1)


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
