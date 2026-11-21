#!/usr/bin/env python3
"""Tests of tools/order_properties.py. Run: python3 tools/test_order_properties.py"""

import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import order_properties as tool  # noqa: E402


def ordered(text: str) -> str:
    return tool.reorder_text(text)[0]


class OrderProperties(unittest.TestCase):
    def test_the_groups_by_name(self):
        self.assertEqual([tool.group_of(n) for n in ("id", "text", "style", "style_pressed", "on_clicked", "one_line")],
                         [0, 1, 2, 2, 3, 1])

    def test_a_style_comes_after_the_properties_and_a_handler_last(self):
        self.assertEqual(ordered('lv::button({.on_clicked = &f, .style = {.radius = r}, .size = s, .id = "b"})'),
                         'lv::button({.id = "b", .size = s, .style = {.radius = r}, .on_clicked = &f})')

    def test_a_list_in_order_is_left_alone(self):
        text = 'lv::label({.id = "a", .text = "x", .style = {}, .on_clicked = &f})'
        self.assertEqual(ordered(text), text)

    def test_the_white_space_stays_where_it_is(self):
        before = 'lv::label({\n    .style = {.x = 1},\n    .text = "t",\n    .id = "a",\n})'
        after = 'lv::label({\n    .id = "a",\n    .text = "t",\n    .style = {.x = 1},\n})'
        self.assertEqual(ordered(before), after)

    def test_a_comma_inside_a_value_does_not_split_it(self):
        before = 'lv::label({.style = {.a = f<int, int>(1, 2)}, .text = std::pair<int, int>{1, 2}.first})'
        after = 'lv::label({.text = std::pair<int, int>{1, 2}.first, .style = {.a = f<int, int>(1, 2)}})'
        self.assertEqual(ordered(before), after)

    def test_a_string_with_a_brace_or_a_comma(self):
        before = 'lv::label({.style = {}, .text = "a, }{"})'
        self.assertEqual(ordered(before), 'lv::label({.text = "a, }{", .style = {}})')

    def test_set_and_props_are_lists_of_properties(self):
        self.assertEqual(ordered('view.set({.on_clicked = &f, .text = "x"});'), 'view.set({.text = "x", .on_clicked = &f});')
        self.assertEqual(ordered('lv::props<"arc"> p{.value = 1, .id = "a"};'), 'lv::props<"arc"> p{.id = "a", .value = 1};')

    def test_the_widget_form_and_the_children(self):
        before = 'lv::widget<"table">({.style = {}, .id = "t"}, lv::label({.style = {}, .text = "x"}))'
        after = 'lv::widget<"table">({.id = "t", .style = {}}, lv::label({.text = "x", .style = {}}))'
        self.assertEqual(ordered(before), after)

    def test_a_comment_between_the_properties_is_left_for_a_hand(self):
        text = 'lv::label({\n  .style = {},\n  // the text\n  .text = "x",\n})'
        new, changed, manual = tool.reorder_text(text)
        self.assertEqual((new, changed, manual), (text, 0, [1]))

    def test_other_lists_are_not_touched(self):
        text = 'lv::each(tracks, &track::id, {.selected = s, .on_row = &f}, row); lv::theme({.screen = {}, .arc = {}})'
        self.assertEqual(ordered(text), text)

    def test_the_count_of_lists_changed(self):
        _, changed, _ = tool.reorder_text('lv::label({.style = {}, .text = "x"}); lv::label({.text = "y", .style = {}})')
        self.assertEqual(changed, 1)


if __name__ == "__main__":
    unittest.main()
