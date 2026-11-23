#!/usr/bin/env python3
"""Unit tests of tools/compile_cost.py, cost_history.py and cost_page.py: python3 tools/test_compile_cost.py"""

import json
import pathlib
import subprocess
import sys
import tempfile
import unittest

TOOLS = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))

import compile_cost as cost  # noqa: E402
import cost_history as hist  # noqa: E402
import cost_page as page  # noqa: E402

CALLGRIND_STDERR = "==12== Collected : 12086576742\n==12== \n"
MEM_REPORT = "Allocations by kind\nTotal               226M       1\nTotal  0\n"
STATS_REPORT = ("Type hash: size 32749, 12835 elements, 0.767021 collisions\n"
                "decl_specializations: size 16381, 6738 elements, 1.242113 collisions\n"
                "type_specializations: size 8191, 3230 elements, 2.323700 collisions\n")
SU = "x.cpp:10:6:void set_value(lv::view<fixed>)\t1536\tstatic\nx.cpp:30:6:void other()\t64\tstatic\n"
LIMITS = {"warn_percent": 0.25, "block_percent": 1.0, "drift_percent": 2.0, "clear_percent": 1.0}
CONFIG = {**LIMITS, "probes": {"one_label": {"id": "one_label"}, "wide": {"id": "wide", "drift_percent": 10, "clear_percent": 5}}}


def row(probe, ir):
    return {"probe": probe, "kind": "compile", "ir": ir}


class Parsing(unittest.TestCase):
    def test_callgrind_total(self):
        self.assertEqual(cost.callgrind_total(CALLGRIND_STDERR), 12086576742)

    def test_callgrind_without_total_is_an_error(self):
        with self.assertRaises(cost.CostError):
            cost.callgrind_total("nothing")

    def test_allocated_total_reads_the_unit(self):
        self.assertEqual(cost.allocated_total(MEM_REPORT), 226 * 1024**2)

    def test_allocated_total_without_a_row_is_an_error(self):
        with self.assertRaises(cost.CostError):
            cost.allocated_total("no rows")

    def test_stats_counts_only_the_named_tables(self):
        got = cost.stats_counts(STATS_REPORT)
        self.assertEqual(got, {"decl_specializations_elements": 6738, "type_specializations_elements": 3230,
                               "type_hash_elements": 12835})

    def test_stack_row_finds_the_function(self):
        got = cost.stack_row(SU, {"id": "set_stack", "function": "set_value"})
        self.assertEqual(got, {"probe": "set_stack", "kind": "stack", "value": 1536})

    def test_stack_row_missing_function_is_an_error(self):
        with self.assertRaises(cost.CostError):
            cost.stack_row(SU, {"id": "x", "function": "nope"})

    def test_probes_file_names_every_probe_once_with_a_reason(self):
        probes = json.loads(cost.PROBES_FILE.read_text())["probes"]
        ids = [p["id"] for p in probes]
        self.assertEqual(len(ids), len(set(ids)))
        for p in probes:
            self.assertTrue(p["stands_for"])
            if p["kind"] == "compile":
                self.assertTrue((TOOLS.parent / p["file"]).is_file(), p["file"])


class Row(unittest.TestCase):
    """compile_row with the compiler and valgrind replaced by canned runs."""

    def test_row_carries_the_instruction_count_and_both_wall_times(self):
        probe = {"id": "p", "file": "f.cpp"}
        done = subprocess.CompletedProcess([], 0, "", "==1== Collected : 12345\n==1== mem 3M\nTotal 2k\n")
        calls = []

        def fake_run(command, directory):
            calls.append(command)
            return done

        saved = (cost.command_of, cost.run)
        cost.command_of, cost.run = (lambda build, probe: (["cc1plus"], "/d")), fake_run
        try:
            row = cost.compile_row("build", probe)
        finally:
            cost.command_of, cost.run = saved
        self.assertEqual((row["probe"], row["kind"], row["ir"]), ("p", "compile", 12345))
        self.assertIn("valgrind_wall_s", row)
        self.assertEqual(calls[-1][:2], ["valgrind", "--tool=callgrind"])


class RunRow(unittest.TestCase):
    def test_value_is_the_difference_per_second(self):
        counts = iter([1_000 + cost.RUN_SECONDS * 1_000, 1_000])
        saved = cost.run
        cost.run = lambda command, directory: subprocess.CompletedProcess(
            [], 0, "", f"Collected : {next(counts)}\n")
        try:
            row = cost.run_row("build", {"id": "v", "scenario": "hidden"})
        finally:
            cost.run = saved
        self.assertEqual(row, {"probe": "v", "kind": "run", "value": 1000})


class Judging(unittest.TestCase):
    def test_new_probe(self):
        self.assertEqual(hist.judge(100, None, None, LIMITS), ("new", 0.0))

    def test_a_rise_under_the_drift_is_same(self):
        self.assertEqual(hist.judge(101_900, 100_000, None, LIMITS)[0], "same")

    def test_a_rise_over_the_drift_is_a_regression(self):
        status, change = hist.judge(102_100, 100_000, None, LIMITS)
        self.assertEqual(status, "regression")
        self.assertAlmostEqual(change, 2.1)

    def test_a_regression_clears_only_under_the_clear_limit(self):
        self.assertEqual(hist.judge(101_500, 100_000, "regression", LIMITS)[0], "regression")
        self.assertEqual(hist.judge(100_900, 100_000, "regression", LIMITS)[0], "same")
        self.assertEqual(hist.judge(101_500, 100_000, "same", LIMITS)[0], "same")

    def test_cheaper_than_the_warn_limit_is_an_improvement(self):
        self.assertEqual(hist.judge(99_700, 100_000, None, LIMITS)[0], "improvement")
        self.assertEqual(hist.judge(99_900, 100_000, None, LIMITS)[0], "same")

    def test_the_last_night_says_what_the_verdict_was(self):
        nights = [{"verdicts": {"one_label": "same"}}, {"verdicts": {"one_label": "regression"}}]
        self.assertEqual(hist.previous_status(nights, "one_label"), "regression")
        self.assertIsNone(hist.previous_status([{"rows": []}], "one_label"))
        self.assertIsNone(hist.previous_status([], "one_label"))

    def test_a_probe_names_its_own_limits(self):
        found = hist.verdicts([row("one_label", 105_000), row("wide", 105_000)], {"one_label": {"value": 100_000},
                              "wide": {"value": 100_000}}, [], CONFIG)
        self.assertEqual([v["status"] for v in found], ["regression", "same"])
        self.assertEqual(hist.limits_of(CONFIG, "wide"), {**LIMITS, "drift_percent": 10, "clear_percent": 5})

    def test_the_file_has_the_four_limits_and_every_probe_resolves(self):
        config = json.loads(hist.PROBES_FILE.read_text())
        config["probes"] = {p["id"]: p for p in config["probes"]}
        self.assertEqual({k: config[k] for k in hist.LIMITS}, LIMITS)
        for probe in config["probes"]:
            got = hist.limits_of(config, probe)
            self.assertTrue(got["warn_percent"] <= got["block_percent"] and got["clear_percent"] <= got["drift_percent"])

    def test_baseline_moves_for_new_and_improvement_only(self):
        found = [{"probe": "a", "status": "new", "value": 5}, {"probe": "b", "status": "same", "value": 6},
                 {"probe": "c", "status": "regression", "value": 7}, {"probe": "d", "status": "improvement", "value": 8}]
        got = hist.moved_baseline({"b": {"value": 1}, "c": {"value": 2}}, found, "sha", "day")
        self.assertEqual(sorted(got), ["a", "b", "c", "d"])
        self.assertEqual((got["a"]["value"], got["b"]["value"], got["c"]["value"], got["d"]["value"]), (5, 1, 2, 8))

    def test_an_accepted_rise_is_not_a_regression_and_moves_the_baseline_with_its_reason(self):
        baseline = {"one_label": {"value": 100_000}, "wide": {"value": 100_000}}
        found = hist.verdicts([row("one_label", 103_000), row("wide", 100_100)], baseline, [], CONFIG, ["one_label"])
        self.assertEqual([v["status"] for v in found], ["accepted", "same"])
        got = hist.moved_baseline(baseline, found, "sha", "day", "call form, PR 189")
        self.assertEqual(got["one_label"], {"value": 103_000, "commit": "sha", "date": "day", "accepted": "call form, PR 189"})
        self.assertEqual(got["wide"], {"value": 100_000})

    def test_a_rise_of_a_probe_that_is_not_named_still_fails(self):
        found = hist.verdicts([row("one_label", 103_000)], {"one_label": {"value": 100_000}}, [], CONFIG, ["wide"])
        self.assertEqual(found[0]["status"], "regression")

    def test_summary_names_the_range_for_a_regression(self):
        found = [{"probe": "one_label", "status": "regression", "value": 101, "baseline": 100, "baseline_commit": "abc",
                  "change_percent": 3.0, "drift_percent": 2.0, "clear_percent": 1.0}]
        text = hist.summary(found, "https://run", "def")
        self.assertIn("git log --oneline abc..def", text)
        self.assertIn("https://run", text)


class Command(unittest.TestCase):
    def run_judge(self, directory, ir, *more):
        rows = pathlib.Path(directory, "rows.json")
        rows.write_text(json.dumps([row("one_label", ir)]))
        return subprocess.run([sys.executable, str(TOOLS / "cost_history.py"), "judge", "--rows", str(rows),
                               "--history", str(pathlib.Path(directory, "h.jsonl")),
                               "--baseline", str(pathlib.Path(directory, "b.json")),
                               "--commit", "c" * 40, "--date", "2026-11-20", *more],
                              capture_output=True, text=True)

    def test_first_night_is_new_then_a_rise_fails(self):
        with tempfile.TemporaryDirectory() as d:
            first = self.run_judge(d, 100_000)
            self.assertEqual(first.returncode, 0)
            self.assertEqual(json.loads(first.stdout)[0]["status"], "new")
            second = self.run_judge(d, 105_000)
            self.assertEqual(second.returncode, 1)
            self.assertEqual(len(hist.read_history(pathlib.Path(d, "h.jsonl"))), 2)
            self.assertEqual(json.loads(pathlib.Path(d, "b.json").read_text())["one_label"]["value"], 100_000)


    def test_accept_records_the_reason_and_needs_one(self):
        with tempfile.TemporaryDirectory() as d:
            self.run_judge(d, 100_000)
            refused = self.run_judge(d, 105_000, "--accept", "one_label")
            self.assertEqual(refused.returncode, 2)
            self.assertIn("needs a --reason", refused.stderr)
            self.assertEqual(len(hist.read_history(pathlib.Path(d, "h.jsonl"))), 1)
            accepted = self.run_judge(d, 105_000, "--accept", "one_label", "--reason", "call form, PR 189")
            self.assertEqual(accepted.returncode, 0)
            self.assertEqual(json.loads(accepted.stdout)[0]["status"], "accepted")
            nights = hist.read_history(pathlib.Path(d, "h.jsonl"))
            self.assertEqual(nights[-1]["accepted"], {"probes": ["one_label"], "reason": "call form, PR 189"})
            self.assertEqual(json.loads(pathlib.Path(d, "b.json").read_text())["one_label"]["accepted"], "call form, PR 189")
            self.assertEqual(self.run_judge(d, 105_100).returncode, 0)

    def test_accept_names_a_probe_that_exists(self):
        with tempfile.TemporaryDirectory() as d:
            self.assertEqual(self.run_judge(d, 100_000, "--accept", "nothing", "--reason", "x").returncode, 2)


    def test_the_night_records_its_verdicts_and_a_regression_clears_with_the_hysteresis(self):
        with tempfile.TemporaryDirectory() as d:
            self.run_judge(d, 100_000)
            self.assertEqual(self.run_judge(d, 103_000).returncode, 1)
            self.assertEqual(hist.read_history(pathlib.Path(d, "h.jsonl"))[-1]["verdicts"], {"one_label": "regression"})
            self.assertEqual(self.run_judge(d, 101_500).returncode, 1)
            self.assertEqual(self.run_judge(d, 100_900).returncode, 0)
            self.assertEqual(json.loads(pathlib.Path(d, "b.json").read_text())["one_label"]["value"], 100_000)


class PullRequest(unittest.TestCase):
    CONFIG = CONFIG

    def verdicts(self, change_percent):
        base = {"one_label": row("one_label", 100_000)}
        head = {"one_label": row("one_label", round(100_000 * (1 + change_percent / 100)))}
        return hist.judge_pr(base, head, self.CONFIG)[0]["status"]

    def test_up_to_the_warn_limit_merges_with_no_comment(self):
        self.assertEqual(self.verdicts(0.25), "ok")
        self.assertEqual(self.verdicts(-3), "ok")

    def test_between_warn_and_block_needs_the_table_and_a_reason(self):
        self.assertEqual(self.verdicts(0.3), "table")
        self.assertEqual(self.verdicts(1.0), "table")

    def test_above_block_waits(self):
        self.assertEqual(self.verdicts(1.1), "block")

    def test_the_command_reads_rows_from_files_and_fails_for_a_block(self):
        with tempfile.TemporaryDirectory() as d:
            base, head = pathlib.Path(d, "base"), pathlib.Path(d, "head")
            base.mkdir()
            head.mkdir()
            (base / "row-one_label.json").write_text(json.dumps(row("one_label", 100_000)))
            (head / "row-one_label.json").write_text(json.dumps(row("one_label", 102_000)))
            done = subprocess.run([sys.executable, str(TOOLS / "cost_history.py"), "judge-pr", "--base", str(base),
                                   "--head", str(head)], capture_output=True, text=True)
            self.assertEqual(done.returncode, 1)
            self.assertIn("| one_label |", done.stdout)
            self.assertIn("waits until it is fixed", done.stdout)


class Page(unittest.TestCase):
    NIGHTS = [{"date": f"2026-11-{10 + i}", "commit": "a" * 40, "rows": [row("one_label", 1000 + i)]} for i in range(4)]

    def test_series_in_order(self):
        self.assertEqual([v for _, _, v in page.series(self.NIGHTS, "one_label")], [1000, 1001, 1002, 1003])

    def test_sparkline_needs_two_points(self):
        self.assertEqual(page.sparkline([1]), "")
        self.assertIn("<polyline", page.sparkline([1, 2, 3]))

    def test_render_has_the_probe_and_a_commit_link(self):
        probes = [{"id": "one_label", "kind": "compile", "stands_for": "a small screen"}]
        text = page.render(self.NIGHTS, probes)
        self.assertIn('id="one_label"', text)
        self.assertIn("/commit/" + "a" * 40, text)

    def test_render_without_history_says_so(self):
        self.assertIn("No night has been measured", page.render([], []))


if __name__ == "__main__":
    unittest.main()
