#!/usr/bin/env python3
"""Tests of tools/stack_budget.py. Run: python3 tools/test_stack_budget.py"""

import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import stack_budget as tool  # noqa: E402

LINE = "/src/examples/mcu/set_probe.cpp:6:24:void set_value(lv::view<lv::detail::fixed<4>{arc}>, int)\tBYTES\tstatic"
OTHER = "/src/include/lv/props.hpp:534:6:void lv::detail::write_back(lv_event_t*)\t16\tstatic"


class Budget(unittest.TestCase):
    def test_a_set_within_the_budget_passes(self):
        self.assertEqual(tool.over_budget(LINE.replace('BYTES', str(3200)) + "\n" + OTHER), [])

    def test_a_set_over_the_budget_fails(self):
        problems = tool.over_budget(LINE.replace('BYTES', str(45296)))
        self.assertEqual(len(problems), 1)
        self.assertIn("45296 bytes", problems[0])

    def test_the_budget_itself_passes(self):
        self.assertEqual(tool.over_budget(LINE.replace('BYTES', str(tool.SET_STACK_BUDGET))), [])

    def test_a_report_without_the_probe_fails(self):
        self.assertEqual(len(tool.over_budget(OTHER)), 1)

    def test_other_functions_do_not_count(self):
        self.assertEqual(tool.frames(OTHER)["/src/include/lv/props.hpp:534:6:void lv::detail::write_back(lv_event_t*)"], 16)
        self.assertFalse(tool.probed("void lv::detail::write_back(lv_event_t*)"))


if __name__ == "__main__":
    unittest.main()
