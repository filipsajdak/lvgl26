#!/usr/bin/env python3
"""Unit tests of tools/check_regex_lists.py: python3 tools/test_check_regex_lists.py"""

import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import check_regex_lists as lists  # noqa: E402


def test(name, **properties):
    return {"name": name, "properties": [{"name": k, "value": v} for k, v in properties.items()]}


class Split(unittest.TestCase):
    def test_one_part_is_one_text(self):
        self.assertEqual(lists.split_tests([test("a", PASS_REGULAR_EXPRESSION=["one; two"])]), [])

    def test_two_parts_are_found_with_their_test_and_property(self):
        found = lists.split_tests([test("b", PASS_REGULAR_EXPRESSION=["one", " two"])])
        self.assertEqual(found, [("b", "PASS_REGULAR_EXPRESSION", ["one", " two"])])

    def test_the_fail_expression_counts_too(self):
        self.assertEqual(len(lists.split_tests([test("c", FAIL_REGULAR_EXPRESSION=["x", "y"])])), 1)

    def test_other_properties_and_tests_without_any_are_ignored(self):
        self.assertEqual(lists.split_tests([test("d", LABELS=["x", "y"]), {"name": "e"}]), [])

    def test_the_report_names_the_fix(self):
        text = lists.report([("b", "PASS_REGULAR_EXPRESSION", ["one", " two"])])
        self.assertIn("b:", text)
        self.assertIn("Escape", text)


if __name__ == "__main__":
    unittest.main()
