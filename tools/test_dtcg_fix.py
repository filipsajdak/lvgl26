#!/usr/bin/env python3
"""Unit tests of tools/dtcg_fix.py: python3 tools/test_dtcg_fix.py"""

import json
import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import dtcg_fix as fixer  # noqa: E402

CHANNELS = 256
LEGACY = """{
  "color": {
    "$type": "color",
    "ink": { "$value": "#3366ff" },
    "glass": { "$value": "#3366ff80" },
    "short": { "$value": "#fa0" },
    "alias": { "$value": "{color.ink}" }
  },
  "space": { "$type": "dimension", "gap": { "$value": "8px" }, "big": { "$value": "1.5rem" }, "bare": { "$value": 4 } },
  "time": { "$type": "duration", "fast": { "$value": "200ms" }, "slow": { "$value": "0.5s" } },
  "shadow": { "$type": "shadow", "lift": { "$value": { "color": "#00000040", "offsetX": "0px", "offsetY": "2px", "blur": "4px", "spread": "0px" } } },
  "text": { "$type": "typography", "body": { "$value": { "fontFamily": "{font.body}", "fontSize": "16px", "letterSpacing": "0px", "lineHeight": 1.5 } } },
  "ramp": { "$type": "gradient", "sky": { "$value": [ { "color": "#ffffff", "position": 0 }, { "color": "#000000", "position": 1 } ] } },
  "own": { "wide": { "$type": "dimension", "$value": "2px" } },
  "weight": { "$type": "fontWeight", "bold": { "$value": 700 } }
}
"""


def loaded(text):
    return json.loads(text)


class Colors(unittest.TestCase):
    def test_a_color_has_components_and_a_hex_fallback(self):
        self.assertEqual(fixer.color_object("#3366ff"), {"hex": "#3366ff", "colorSpace": "srgb", "components": [0.2, 0.4, 1]})

    def test_an_alpha_digit_pair_becomes_alpha(self):
        self.assertEqual(fixer.color_object("#3366ff80")["alpha"], 0.502)
        self.assertEqual(fixer.color_object("#3366ff80")["hex"], "#3366ff")

    def test_three_digits_are_doubled(self):
        self.assertEqual(fixer.color_object("#fa0")["hex"], "#ffaa00")

    def test_every_channel_value_comes_back_after_the_reader_rounds_it(self):
        for channel in range(CHANNELS):
            digits = f"#{channel:02x}0000"
            red = fixer.color_object(digits)["components"][0]
            self.assertEqual(int(red * (CHANNELS - 1) + 0.5), channel, digits)

    def test_a_non_color_is_left(self):
        self.assertIsNone(fixer.color_object("{color.ink}"))
        self.assertIsNone(fixer.color_object("red"))


class Quantities(unittest.TestCase):
    def test_a_length(self):
        self.assertEqual(fixer.quantity_object("8px", fixer.LENGTH_UNITS), {"value": 8, "unit": "px"})
        self.assertEqual(fixer.quantity_object("1.5rem", fixer.LENGTH_UNITS), {"value": 1.5, "unit": "rem"})

    def test_a_percentage_has_no_form(self):
        self.assertIsNone(fixer.quantity_object("50%", fixer.LENGTH_UNITS))

    def test_a_time_unit_is_not_a_length(self):
        self.assertIsNone(fixer.quantity_object("200ms", fixer.LENGTH_UNITS))
        self.assertEqual(fixer.quantity_object("200ms", fixer.TIME_UNITS), {"value": 200, "unit": "ms"})


class Files(unittest.TestCase):
    def setUp(self):
        self.fixed = loaded(fixer.fix(LEGACY))

    def test_the_forms_of_each_type_are_rewritten(self):
        self.assertEqual(self.fixed["color"]["ink"]["$value"]["hex"], "#3366ff")
        self.assertEqual(self.fixed["color"]["glass"]["$value"]["alpha"], 0.502)
        self.assertEqual(self.fixed["space"]["gap"]["$value"], {"value": 8, "unit": "px"})
        self.assertEqual(self.fixed["time"]["slow"]["$value"], {"value": 0.5, "unit": "s"})

    def test_an_alias_a_number_and_an_untouched_type_stay(self):
        self.assertEqual(self.fixed["color"]["alias"]["$value"], "{color.ink}")
        self.assertEqual(self.fixed["weight"]["bold"]["$value"], 700)
        self.assertEqual(self.fixed["text"]["body"]["$value"]["lineHeight"], 1.5)
        self.assertEqual(self.fixed["text"]["body"]["$value"]["fontFamily"], "{font.body}")

    def test_a_type_of_the_token_itself_wins_over_its_group(self):
        self.assertEqual(self.fixed["own"]["wide"]["$value"], {"value": 2, "unit": "px"})

    def test_the_members_of_a_composite_are_rewritten(self):
        shadow = self.fixed["shadow"]["lift"]["$value"]
        self.assertEqual((shadow["color"]["alpha"], shadow["blur"]), (0.251, {"value": 4, "unit": "px"}))
        self.assertEqual(self.fixed["text"]["body"]["$value"]["fontSize"], {"value": 16, "unit": "px"})

    def test_the_stops_of_a_gradient_are_rewritten(self):
        self.assertEqual([stop["color"]["hex"] for stop in self.fixed["ramp"]["sky"]["$value"]], ["#ffffff", "#000000"])

    def test_a_bare_number_is_left_for_the_reader_to_refuse(self):
        self.assertEqual(self.fixed["space"]["bare"]["$value"], 4)

    def test_running_twice_changes_nothing(self):
        once = fixer.fix(LEGACY)
        self.assertEqual(fixer.fix(once), once)

    def test_the_layout_of_the_file_stays(self):
        old, new = LEGACY.splitlines(), fixer.fix(LEGACY).splitlines()
        self.assertEqual(len(old), len(new))
        self.assertEqual(old[0], new[0])
        self.assertEqual(old[-2], new[-2])

    def test_a_percentage_is_left(self):
        text = '{"w": {"$type": "dimension", "a": {"$value": "50%"}}}'
        self.assertEqual(fixer.fix(text), text)

    def test_a_string_with_an_escape_keeps_its_place(self):
        text = '{"d": "say \\"hi\\"", "c": {"$type": "color", "x": {"$value": "#000000"}}}'
        self.assertEqual(loaded(fixer.fix(text))["d"], 'say "hi"')
        self.assertIn("srgb", fixer.fix(text))


class Command(unittest.TestCase):
    def test_check_changes_nothing_and_fails_for_a_file_that_would_change(self):
        import tempfile

        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / "tokens.json"
            path.write_text(LEGACY)
            self.assertEqual(fixer.main(["--check", str(path)]), 1)
            self.assertEqual(path.read_text(), LEGACY)
            self.assertEqual(fixer.main([str(path)]), 0)
            self.assertEqual(fixer.main(["--check", str(path)]), 0)


if __name__ == "__main__":
    unittest.main()
