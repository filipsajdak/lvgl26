#!/usr/bin/env python3
"""Tests of tools/call_form.py. Run: python3 tools/test_call_form.py"""

import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import call_form as tool  # noqa: E402


def moved(text: str) -> str:
    return tool.children_into_call(text)[0]


def commas(text: str) -> str:
    return tool.trailing_commas(text)[0]


class ChildrenIntoCall(unittest.TestCase):
    def test_children_after_properties_move_into_the_call(self):
        self.assertEqual(moved('lv::group({.id = "a"})[lv::label({.text = "x"}), lv::label({.text = "y"})]'),
                         'lv::group({.id = "a"}, lv::label({.text = "x"}), lv::label({.text = "y"}))')

    def test_a_node_with_no_properties(self):
        self.assertEqual(moved("lv::column[lv::label({.text = \"x\"})]"), 'lv::column(lv::label({.text = "x"}))')
        self.assertEqual(moved("lv::row()[lv::label({})]"), "lv::row(lv::label({}))")

    def test_a_tree_inside_a_tree(self):
        self.assertEqual(moved("lv::column({})[lv::row[lv::button({})[lv::label({})]]]"),
                         "lv::column({}, lv::row(lv::button({}, lv::label({}))))")

    def test_the_widget_by_name(self):
        self.assertEqual(moved('lv::widget<"table">({.id = "t"})[lv::label({})]'), 'lv::widget<"table">({.id = "t"}, lv::label({}))')

    def test_each_shown_and_when(self):
        self.assertEqual(moved("lv::each(tracks, &track::id, {.selected = s})[row]"),
                         "lv::each(tracks, &track::id, {.selected = s}, row)")
        self.assertEqual(moved("lv::shown(page)[lv::is<p::a>(a), lv::otherwise()]"),
                         "lv::shown(page, lv::is<p::a>(a), lv::otherwise())")
        self.assertEqual(moved("lv::when(online)[banner]"), "lv::when(online, banner)")

    def test_when_with_options_has_no_call_form_and_is_left(self):
        text = "lv::when(online, {.enter = f})[banner]"
        self.assertEqual(tool.children_into_call(text), (text, 0, [1]))

    def test_a_call_with_no_children_and_other_subscripts_are_left_alone(self):
        text = 'lv::label({.text = "x"}); auto c = things[2]; lv::state s{0}; x = lv::each(a, &r::id, row);'
        self.assertEqual(moved(text), text)


class TrailingCommas(unittest.TestCase):
    def test_three_properties_get_a_comma(self):
        self.assertEqual(commas('lv::label({.id = "a", .text = "x", .size = {1, 2}})'),
                         'lv::label({.id = "a", .text = "x", .size = {1, 2},})')

    def test_two_short_properties_do_not(self):
        text = 'lv::label({.id = "a", .text = "x"})'
        self.assertEqual(commas(text), text)

    def test_a_property_that_holds_a_list_does(self):
        self.assertEqual(commas("lv::label({.style = {.radius = 1}, .id = a})"), "lv::label({.style = {.radius = 1}, .id = a,})")

    def test_a_list_that_ends_with_a_comma_is_left_alone(self):
        text = 'lv::label({.id = "a", .text = "x", .size = 1,})'
        self.assertEqual(commas(text), text)

    def test_the_comma_goes_before_the_white_space_that_closes_the_list(self):
        self.assertEqual(commas("lv::label({\n  .a = 1,\n  .b = 2,\n  .c = 3\n})"), "lv::label({\n  .a = 1,\n  .b = 2,\n  .c = 3,\n})")

    def test_a_list_of_values_is_not_a_list_of_properties(self):
        text = "auto a = {1, 2, 3, 4}; lv::label({.size = {1, 2, 3}})"
        self.assertEqual(commas(text), text)

    def test_a_comment_inside_leaves_the_list_for_a_hand(self):
        text = "lv::label({\n  .a = 1,\n  .b = 2,  // note\n  .c = 3\n})"
        self.assertEqual(tool.trailing_commas(text), (text, 0, [1]))

    def test_the_nested_lists_get_theirs_too(self):
        self.assertEqual(commas("x({.a = 1, .b = {.c = 1, .d = 2, .e = 3}})"), "x({.a = 1, .b = {.c = 1, .d = 2, .e = 3,},})")

    def test_converting_twice_changes_nothing(self):
        once = tool.convert('lv::group({.id = "g", .size = s, .style = {.pad_all = 1}})[lv::label({.text = "x"})]')[0]
        self.assertEqual(tool.convert(once)[0], once)


if __name__ == "__main__":
    unittest.main()
