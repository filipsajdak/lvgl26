#!/usr/bin/env python3
"""Tests of tools/look_groups.py. Run: python3 tools/test_look_groups.py"""

import json
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import look_groups as tool  # noqa: E402

STYLES = {"bg_color", "radius", "text_color", "width"}
LAYERS = {"checked", "pressed", "knob", "indicator", "default", "focus_key"}
COLOR = {"$type": "color", "$value": "#ffffff"}
LENGTH = {"$type": "dimension", "$value": "4px"}

TOKENS = {
    "color": {"$type": "color", "white": {"$value": "#ffffff"}},
    "space": {"$type": "dimension", "gap": {"$value": "8px"}},
    "tile": {"radius": LENGTH, "pressed": {"bg_color": COLOR}},
    "tile-print": {"bg_color": COLOR},
    "tile-usb": {"bg_color": COLOR},
    "status": {"text_color": COLOR, "checked": {"text_color": COLOR}},
    "gauge": {"width": LENGTH, "knob": {"pressed": {"bg_color": COLOR}}},
}


def unused(source: str, tokens: dict = TOKENS, ignore: tuple[str, ...] = ()) -> list[str]:
    with tempfile.TemporaryDirectory() as folder:
        file = pathlib.Path(folder) / "ui.cpp"
        file.write_text(source)
        return tool.unused_groups(tokens, [file], STYLES, LAYERS, set(ignore))


class ComponentGroups(unittest.TestCase):
    def test_groups_of_values_are_not_components(self):
        found = tool.component_groups(TOKENS, STYLES, LAYERS)
        self.assertNotIn("color", found)
        self.assertNotIn("space", found)

    def test_a_group_named_for_style_properties_is_one(self):
        self.assertEqual(
            tool.component_groups(TOKENS, STYLES, LAYERS),
            ["tile", "tile-print", "tile-usb", "status", "gauge"],
        )

    def test_a_state_or_part_belongs_to_its_parent(self):
        found = tool.component_groups(TOKENS, STYLES, LAYERS)
        self.assertNotIn("tile.pressed", found)
        self.assertNotIn("status.checked", found)
        self.assertNotIn("gauge.knob", found)

    def test_a_look_with_entries_only_in_its_layers_is_one(self):
        tokens = {"status-loud": {"checked": {"text_color": COLOR}}, "deep": {"pressed": {"knob": {"radius": LENGTH}}}}
        self.assertEqual(tool.component_groups(tokens, STYLES, LAYERS), ["status-loud", "deep"])

    def test_a_group_named_like_a_part_is_a_look_where_no_look_holds_it(self):
        tokens = {"indicator": {"bg_color": COLOR}, "panel": {"knob": {"radius": LENGTH}}}
        self.assertEqual(tool.component_groups(tokens, STYLES, LAYERS), ["indicator", "panel"])

    def test_a_layer_is_named_as_the_compiler_reads_it(self):
        tokens = {"tile": {"radius": LENGTH, "default": {"radius": LENGTH}, "focus-key": {"radius": LENGTH}}}
        self.assertEqual(tool.component_groups(tokens, STYLES, LAYERS), ["tile"])


class VendorData(unittest.TestCase):
    def test_a_group_with_only_vendor_data_is_a_component(self):
        tokens = {"row": {"$extensions": {tool.VENDOR_KEY: {"flex_flow": "row"}}}}
        self.assertEqual(tool.component_groups(tokens, STYLES, LAYERS), ["row"])

    def test_the_vendor_data_of_another_vendor_is_not(self):
        tokens = {"row": {"$extensions": {"com.example": {"flex_flow": "row"}}}}
        self.assertEqual(tool.component_groups(tokens, STYLES, LAYERS), [])


class Uses(unittest.TestCase):
    def test_a_look_names_its_group(self):
        self.assertNotIn("tile", unused('lv::look<"tile">'))

    def test_a_list_names_each_group(self):
        found = unused('lv::look<"tile tile-print">')
        self.assertNotIn("tile-print", found)
        self.assertIn("tile-usb", found)

    def test_a_look_over_several_lines(self):
        self.assertNotIn("gauge", unused('lv::look<\n    "gauge">'))

    def test_a_design_after_the_path_does_not_hide_it(self):
        self.assertNotIn("gauge", unused('lv::look<"gauge", Dark>'))

    def test_look_of_names_every_group_under_its_prefix(self):
        found = unused('lv::look<"tile"> | lv::look_of<"tile", Action>')
        self.assertNotIn("tile-print", found)
        self.assertNotIn("tile-usb", found)

    def test_look_of_reaches_the_siblings_of_its_prefix_only(self):
        tokens = {"a": {"radius": LENGTH}, "a-b": {"radius": LENGTH}, "a-b-c": {"radius": LENGTH},
                  "q": {"a-x": {"radius": LENGTH}}, "ab": {"radius": LENGTH}}
        self.assertEqual(unused('lv::look_of<"a", E>', tokens), ["a", "q.a-x", "ab"])

    def test_a_group_no_source_names_is_reported(self):
        self.assertEqual(unused('lv::look<"tile">'), ["tile-print", "tile-usb", "status", "gauge"])

    def test_another_program_may_use_a_group(self):
        self.assertNotIn("gauge", unused("", ignore=("gauge",)))

    def test_a_text_that_only_looks_like_a_look_is_not_one(self):
        self.assertIn("gauge", unused('// a look<"gauge"> in a comment, not lv::look'))

    def test_a_look_in_a_comment_is_no_use(self):
        self.assertIn("gauge", unused('// lv::look<"gauge">\n'))
        self.assertIn("gauge", unused('/* lv::look<"gauge"> */'))
        self.assertIn("tile-print", unused('lv::look<"tile"> // lv::look_of<"tile", E>'))

    def test_a_look_in_a_string_is_no_use(self):
        self.assertIn("gauge", unused('auto s = "lv::look<\\"gauge\\">";'))
        self.assertIn("gauge", unused('auto s = R"(lv::look<"gauge">)";'))

    def test_a_comment_sign_in_a_string_hides_nothing(self):
        self.assertNotIn("gauge", unused('auto s = "//"; auto g = lv::look<"gauge">;'))
        self.assertNotIn("gauge", unused("auto c = '\"'; auto g = lv::look<\"gauge\">;"))
        self.assertNotIn("gauge", unused("auto n = 1'000; auto g = lv::look<\"gauge\">;"))

    def test_a_path_may_use_the_member_spelling(self):
        found = unused('lv::look<"tile_print"> | lv::look<"gauge">', ignore=("tile", "status"))
        self.assertEqual(found, ["tile-usb"])

    def test_look_of_may_use_the_member_spelling(self):
        tokens = {"ink-tile": {"radius": LENGTH}, "ink-tile-print": {"radius": LENGTH}, "inkx": {"radius": LENGTH}}
        self.assertEqual(unused('lv::look<"ink_tile"> | lv::look_of<"ink_tile", E>', tokens), ["inkx"])

    def test_a_report_keeps_the_spelling_of_the_token_file(self):
        self.assertIn("tile-usb", unused('lv::look<"tile">'))


class Reference(unittest.TestCase):
    def test_the_committed_reference_names_parts_and_states(self):
        styles, layers = tool.reference_names(tool.REFERENCE)
        self.assertIn("radius", styles)
        self.assertTrue({"checked", "pressed", "knob", "indicator"} <= layers)
        self.assertNotIn("any", layers)

    def test_a_reference_without_parts_is_refused(self):
        with tempfile.TemporaryDirectory() as folder:
            path = pathlib.Path(folder) / "ref.json"
            path.write_text(json.dumps({"style": [], "enums": {}}))
            with self.assertRaises(tool.Error):
                tool.reference_names(path)


class Command(unittest.TestCase):
    def run_tool(self, tokens: dict, source: str, *extra: str) -> int:
        with tempfile.TemporaryDirectory() as folder:
            (pathlib.Path(folder) / "tokens.json").write_text(json.dumps(tokens))
            (pathlib.Path(folder) / "ui.cpp").write_text(source)
            return tool.main([f"{folder}/tokens.json", folder, *extra])

    def test_exit_status_is_one_for_an_unused_group(self):
        self.assertEqual(self.run_tool(TOKENS, 'lv::look<"tile">'), 1)

    def test_exit_status_is_zero_when_every_group_is_used(self):
        tokens = {"tile": {"radius": LENGTH}}
        self.assertEqual(self.run_tool(tokens, 'lv::look<"tile">'), 0)

    def test_a_missing_source_is_a_usage_error(self):
        self.assertEqual(tool.main(["tokens.json", "no/such/folder"]), 2)


if __name__ == "__main__":
    unittest.main()
