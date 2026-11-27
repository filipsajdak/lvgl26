#!/usr/bin/env python3
"""Tests of tools/icons_names.py and of include/lv/icons/phosphor.names. Run: python3 tools/test_icons_names.py"""

import pathlib
import sys
import unittest

TOOLS = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))

import icons_names  # noqa: E402

CSS = """
.ph.ph-gear:before { content: "\\e270"; }
.ph.ph-arrow-left:before {
  content: "\\e058";
}
.ph-bold.ph-gear:before { content: "\\e270"; }
"""
NAMES = icons_names.OUT


class Reading(unittest.TestCase):
    def test_each_icon_of_the_style_sheet_is_found_once_with_its_code_point(self):
        self.assertEqual(icons_names.icons(CSS), {"gear": 0xE270, "arrow-left": 0xE058})

    def test_the_text_is_sorted_by_name_and_one_line_each(self):
        self.assertEqual(icons_names.render({"gear": 0xE270, "arrow-left": 0xE058}), "arrow-left e058\ngear e270\n")

    def test_a_style_sheet_without_icons_gives_nothing(self):
        self.assertEqual(icons_names.icons("body { color: red; }"), {})


class TheFile(unittest.TestCase):
    def lines(self):
        return NAMES.read_text().splitlines()

    def test_the_names_are_sorted_and_unique(self):
        names = [line.split(" ")[0] for line in self.lines()]
        self.assertEqual(names, sorted(set(names)))

    def test_the_search_of_the_header_compares_bytes_so_the_order_is_the_one_of_python(self):
        names = [line.split(" ")[0] for line in self.lines()]
        self.assertEqual(names, sorted(names, key=lambda n: n.encode()))

    def test_each_code_point_is_in_the_private_use_area(self):
        codes = [int(line.split(" ")[1], 16) for line in self.lines()]
        self.assertTrue(all(0xE000 <= c <= 0xF8FF for c in codes))

    def test_the_icons_of_the_coffee_machine_are_there(self):
        names = {line.split(" ")[0] for line in self.lines()}
        for icon in ("arrow-left", "check", "gear", "plus", "minus", "x", "arrow-right", "caret-down"):
            self.assertIn(icon, names)

    def test_the_file_ends_with_a_line_break(self):
        self.assertTrue(NAMES.read_text().endswith("\n"))


if __name__ == "__main__":
    unittest.main()
