#!/usr/bin/env python3
"""Tests of tools/format_ui.py. Run: python3 tools/test_format_ui.py

The tests that run clang-format pin what the format of the repository relies on: a property list that ends with
a comma is laid out one property per line, and one without stays on one line while it fits. clang-format does not
document this. If a new clang-format changes it, the test says so, and the format is looked at again.
"""

import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import format_ui as tool  # noqa: E402

PROGRAM = tool.clang_format_program()
needs_clang_format = unittest.skipUnless(PROGRAM, "clang-format is not installed")

ONE_LINE = 'constexpr auto a = lv::label({.id = "a", .text = "x", .size = {1, 2}});\n'
WITH_COMMA = 'constexpr auto a = lv::label({.id = "a", .text = "x", .size = {1, 2},});\n'
ONE_PER_LINE = '''constexpr auto a = lv::label({
  .id   = "a",
  .text = "x",
  .size = {1, 2},
});
'''
NESTED = '''constexpr auto c = lv::column({.size = {1, 2}, .style = {.pad_all = 1, .radius = 2,},},
                              lv::label({.text = "x"}), lv::label({.text = "y",}));
'''
NESTED_FORMATTED = '''constexpr auto c = lv::column(
  {
    .size = {1, 2},
    .style = {
      .pad_all = 1,
      .radius  = 2,
    },
  },
  lv::label({.text = "x"}),
  lv::label({
    .text = "y",
  })
);
'''


class JoinBlocks(unittest.TestCase):
    def test_a_block_after_an_equals_sign_moves_up_and_left(self):
        before = "  .style =\n    {\n      .a = 1,\n    },\n  .b = 2,\n"
        self.assertEqual(tool.join_blocks(before), "  .style = {\n    .a = 1,\n  },\n  .b = 2,\n")

    def test_a_nested_block_moves_with_it(self):
        before = "x =\n  {\n    .a =\n      {\n        .b = 1,\n      },\n  };\n"
        self.assertEqual(tool.join_blocks(before), "x = {\n  .a = {\n    .b = 1,\n  },\n};\n")

    def test_a_brace_in_a_string_does_not_end_the_block(self):
        before = 'x =\n  {\n    .a = "}",\n  };\ny = 1;\n'
        self.assertEqual(tool.join_blocks(before), 'x = {\n  .a = "}",\n};\ny = 1;\n')

    def test_other_lines_are_left_alone(self):
        text = "int a = 1;\nauto f = [] {\n  return 2;\n};\n"
        self.assertEqual(tool.join_blocks(text), text)

    def test_the_pass_is_idempotent(self):
        once = tool.join_blocks("  .style =\n    {\n      .a = 1,\n    },\n")
        self.assertEqual(tool.join_blocks(once), once)


@needs_clang_format
class Lever(unittest.TestCase):
    """What clang-format does with a trailing comma, in the versions the repository is pinned for."""

    def test_the_version_is_one_that_is_pinned(self):
        self.assertIn(tool.major_version(PROGRAM), tool.VERSIONS)

    def test_without_a_comma_a_list_stays_on_one_line(self):
        self.assertEqual(tool.format_text(ONE_LINE, "a.cpp", PROGRAM), ONE_LINE)

    def test_a_trailing_comma_makes_a_list_one_property_per_line(self):
        self.assertEqual(tool.format_text(WITH_COMMA, "a.cpp", PROGRAM), ONE_PER_LINE)

    def test_the_lever_works_in_every_level_of_a_tree(self):
        self.assertEqual(tool.format_text(NESTED, "a.cpp", PROGRAM), NESTED_FORMATTED)

    def test_the_equals_signs_of_a_list_are_aligned(self):
        out = tool.format_text(WITH_COMMA, "a.cpp", PROGRAM).split("\n")
        self.assertEqual({line.index("=") for line in out[1:4]}, {8})

    def test_formatting_twice_changes_nothing(self):
        once = tool.format_text(NESTED, "a.cpp", PROGRAM)
        self.assertEqual(tool.format_text(once, "a.cpp", PROGRAM), once)

    def test_only_white_space_changes(self):
        self.assertEqual(tool.squeezed(tool.format_text(NESTED, "a.cpp", PROGRAM)), tool.squeezed(NESTED))

    def test_a_result_that_changes_more_than_white_space_is_refused(self):
        original = tool.run_clang_format
        tool.run_clang_format = lambda text, name, program: text.replace("1", "2")
        try:
            with self.assertRaises(RuntimeError):
                tool.format_text(ONE_LINE, "a.cpp", PROGRAM)
        finally:
            tool.run_clang_format = original


if __name__ == "__main__":
    unittest.main()
