#!/usr/bin/env python3
"""Tests of tools/rules.py: the rules as data, and the page written from them. Run: python3 tools/test_rules.py"""

import copy
import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import rules  # noqa: E402

DATA = rules.load()


def with_rule(**changes) -> dict:
    """The data with its first rule changed."""
    data = copy.deepcopy(DATA)
    data["rules"][0].update(changes)
    return data


def first_problems(data: dict) -> list[str]:
    name = data["rules"][0].get("id", "?")
    return [p.split(": ", 1)[1] for p in rules.problems(data) if p.startswith(f"{name}: ")]


class Data(unittest.TestCase):
    def test_the_rules_of_the_repository_are_valid(self):
        self.assertEqual(rules.problems(DATA), [])

    def test_an_id_is_its_quality_and_a_number(self):
        self.assertIn("the id is <quality>-<n>, with a known quality", first_problems(with_rule(id="no-boilerplate-x")))
        self.assertIn("the id is <quality>-<n>, with a known quality", first_problems(with_rule(quality="fast-run")))

    def test_an_id_is_used_once(self):
        data = copy.deepcopy(DATA)
        data["rules"].append(copy.deepcopy(data["rules"][0]))
        self.assertIn(f"{data['rules'][0]['id']}: the id is used twice", rules.problems(data))

    def test_every_field_is_there_and_no_other(self):
        data = copy.deepcopy(DATA)
        del data["rules"][0]["why"]
        data["rules"][0]["colour"] = "red"
        self.assertEqual(first_problems(data), ["no why", "an unknown field colour"])

    def test_an_example_names_a_region_that_exists(self):
        self.assertTrue(any("needs exactly two lines `//! [nothing]`" in p
                            for p in first_problems(with_rule(example="examples/state/main.cpp nothing"))))
        self.assertTrue(any("does not exist" in p for p in first_problems(with_rule(example="examples/none.cpp"))))

    def test_no_example_says_why(self):
        self.assertIn("an example of null needs an example_note", first_problems(with_rule(example=None)))

    def test_a_guide_is_a_page_or_planned(self):
        self.assertIn("the guide nowhere.md is neither a page nor planned", first_problems(with_rule(guide="nowhere.md")))

    def test_a_planned_page_that_exists_leaves_the_plan(self):
        data = with_rule(guide="lists.md")
        data["pages_planned"]["lists.md"] = "Lists"
        self.assertIn("the planned page lists.md exists: remove it from pages_planned", first_problems(data))

    def test_evidence_has_a_file_a_date_and_a_finding(self):
        found = first_problems(with_rule(evidence={"source": "docs/none.md", "date": "today"}))
        self.assertEqual(found, ["the evidence names no file", "the evidence needs the date of the measurement",
                                 "the evidence needs what was found"])


class Checks(unittest.TestCase):
    pinned = {"plain": "a key names one row", "variable": "${message}"}

    def test_a_cmake_argument_is_read_as_cmake_reads_it(self):
        self.assertEqual(rules.cmake_text(r'a \\(b\\) \"c\"'), r'a \(b\) "c"')

    def test_the_pins_of_the_repository_are_read(self):
        self.assertEqual(rules.pins()["each_two_rows"], "lv::each takes one row template")

    def test_a_message_shows_what_its_test_pins(self):
        self.assertIsNone(rules.message_problem("plain", "lv: a key names one row, and more.", self.pinned))
        self.assertIn("differs", rules.message_problem("plain", "lv: a key is a row.", self.pinned))

    def test_a_pin_in_a_variable_cannot_be_compared(self):
        self.assertIn("CMake variable", rules.message_problem("variable", "anything", self.pinned))

    def test_a_check_names_a_test_that_exists(self):
        check = {"by": "compiler", "test": "no_such_test", "message": "x"}
        self.assertIn("the test no_such_test does not exist", first_problems(with_rule(check=check)))

    def test_a_message_pinned_in_a_file_is_in_it(self):
        check = {"by": "build", "test": "include_check_build", "pinned_in": "tests/include_check/run.cmake",
                 "message": "main.cpp:1: lv::nothing needs #include <lv/nothing.hpp>"}
        self.assertIn("tests/include_check/run.cmake does not hold the message", first_problems(with_rule(check=check)))

    def test_a_check_is_by_the_compiler_the_build_or_a_tool(self):
        check = {"by": "review", "message": "x"}
        self.assertTrue(any("a check is by one of" in p for p in first_problems(with_rule(check=check))))


class Page(unittest.TestCase):
    def test_the_page_lists_every_rule_with_its_anchor(self):
        text = rules.page(DATA)
        for rule in DATA["rules"]:
            self.assertIn(f"| [`{rule['id']}`](#{rule['id']}) |", text)
            self.assertIn(f"\n### {rule['id']}\n", text)

    def test_a_planned_guide_is_named_without_a_link(self):
        planned = next(iter(DATA["pages_planned"]))
        self.assertEqual(rules.guide_line(planned, DATA), f"Guide: {DATA['pages_planned'][planned]} (not written yet).")

    def test_a_written_guide_is_a_link(self):
        self.assertEqual(rules.guide_line("lists.md", DATA), "Guide: [Lists](lists.md).")

    def test_a_rule_without_a_check_says_so(self):
        self.assertEqual(rules.check_lines(None), ["Checked: no tool checks this rule yet."])

    def test_a_performance_rule_without_a_measure_says_so(self):
        rule = {"quality": "fast-run", "evidence": None}
        self.assertEqual(rules.evidence_lines(rule), ["Measured: not measured yet."])
        self.assertEqual(rules.evidence_lines({"quality": "checked", "evidence": None}), [])

    def test_a_guide_shows_a_rule_as_a_link_and_its_sentence(self):
        rule = DATA["rules"][0]
        self.assertEqual(rules.rule_list([rule["id"]], DATA), f"- [`{rule['id']}`](rules.md#{rule['id']}): {rule['rule']}\n")


if __name__ == "__main__":
    unittest.main()
