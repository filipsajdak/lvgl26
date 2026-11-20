#!/usr/bin/env python3
"""Unit tests of tools/compile_fail_messages.py: python3 tools/test_compile_fail_messages.py"""

import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import compile_fail_messages as messages  # noqa: E402


def test(name, **properties):
    return {"name": name, "properties": [{"name": k, "value": v} for k, v in properties.items()]}


class Messages(unittest.TestCase):
    def test_a_test_that_pins_a_message_gives_its_form_and_text(self):
        tests = [test("fail_a", LVGL26_MESSAGE="lv: it; or not.", LVGL26_MESSAGE_FORM="constexpr")]
        self.assertEqual(messages.messages_of(tests), {"fail_a": {"form": "constexpr", "message": "lv: it; or not."}})

    def test_other_tests_are_left_out(self):
        self.assertEqual(messages.messages_of([test("fail_b", LABELS="compile-fail"), {"name": "c"}]), {})


if __name__ == "__main__":
    unittest.main()
