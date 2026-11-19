#!/usr/bin/env python3
"""Unit tests of tools/signal_sync.py: python3 tools/test_signal_sync.py"""

import copy
import json
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import signal_sync as sync  # noqa: E402

DESIGN = {
    "name": "Signal",
    "version": 5,
    "color": {"tokens": [{"name": "ground", "value": {"dark": "#0A0A0C"}, "usage": "Screens."},
                         {"name": "lamp-green", "value": {"dark": "#3DDB84"}, "usage": "Lamps."}]},
    "type": {"fonts": [{"family": "Barlow", "file": "fonts/Barlow-Medium.ttf", "weight": "500"}]},
}


class Record(unittest.TestCase):
    def test_a_change_of_layout_or_key_order_keeps_the_hash(self):
        shuffled = json.loads(json.dumps(DESIGN, indent=4, sort_keys=True))
        self.assertEqual(sync.digest(shuffled), sync.digest(DESIGN))

    def test_a_change_of_a_value_changes_the_hash(self):
        other = copy.deepcopy(DESIGN)
        other["color"]["tokens"][0]["value"]["dark"] = "#0A0A0D"
        self.assertNotEqual(sync.digest(other), sync.digest(DESIGN))

    def test_a_record_of_the_copy_has_no_problem(self):
        self.assertEqual(sync.record_problems(DESIGN, sync.record_of(DESIGN, "2026-11-19")), [])

    def test_an_edit_without_recording_is_found(self):
        record = sync.record_of(DESIGN, "2026-11-19")
        other = copy.deepcopy(DESIGN)
        other["color"]["tokens"].pop()
        problems = sync.record_problems(other, record)
        self.assertEqual(len(problems), 1)
        self.assertIn("not the recorded one", problems[0])

    def test_a_new_version_without_recording_names_both_versions(self):
        record = sync.record_of(DESIGN, "2026-11-19")
        other = copy.deepcopy(DESIGN)
        other["version"] = 6
        problems = sync.record_problems(other, record)
        self.assertIn("version 6 and the record says 5", problems[0])
        self.assertEqual(len(problems), 2)


class Differences(unittest.TestCase):
    def test_equal_files_have_none(self):
        self.assertEqual(sync.differences(DESIGN, copy.deepcopy(DESIGN)), [])

    def test_a_token_only_in_the_copy_and_one_only_in_the_design_tool(self):
        tool = copy.deepcopy(DESIGN)
        tool["color"]["tokens"].pop()
        tool["color"]["tokens"].append({"name": "lamp-blue", "value": {"dark": "#4D8DFF"}, "usage": "Beam."})
        found = sync.differences(DESIGN, tool)
        self.assertEqual(len(found), 2)
        self.assertTrue(any("/color/tokens/lamp-blue: only in the design tool" in line for line in found))
        self.assertTrue(any("/color/tokens/lamp-green: only in the copy" in line for line in found))

    def test_a_value_that_differs_names_both_sides(self):
        tool = copy.deepcopy(DESIGN)
        tool["color"]["tokens"][0]["value"]["dark"] = "#111111"
        found = sync.differences(DESIGN, tool)
        self.assertEqual(found, ['/color/tokens/ground/value/dark: copy "#0A0A0C", design tool "#111111"'])

    def test_a_font_is_named_by_its_file(self):
        tool = copy.deepcopy(DESIGN)
        tool["type"]["fonts"] = []
        self.assertIn("/type/fonts/fonts/Barlow-Medium.ttf: only in the copy", sync.differences(DESIGN, tool)[0])

    def test_a_long_value_is_cut(self):
        tool = copy.deepcopy(DESIGN)
        tool["color"]["tokens"][0]["usage"] = "x" * 200
        self.assertLess(len(sync.differences(DESIGN, tool)[0]), 200)


class Command(unittest.TestCase):
    def test_compare_exits_with_1_for_a_difference_and_0_for_none(self):
        with tempfile.TemporaryDirectory() as directory:
            same = pathlib.Path(directory) / "same.json"
            same.write_text(sync.DESIGN.read_text())
            self.assertEqual(sync.main(["--compare", str(same)]), 0)
            other = json.loads(sync.DESIGN.read_text())
            other["version"] += 1
            different = pathlib.Path(directory) / "different.json"
            different.write_text(json.dumps(other))
            self.assertEqual(sync.main(["--compare", str(different)]), 1)


class WrittenFiles(unittest.TestCase):
    def test_a_written_file_that_differs_or_is_missing_is_found(self):
        with tempfile.TemporaryDirectory() as directory:
            original = sync.signal_tokens.OUT
            sync.signal_tokens.OUT = pathlib.Path(directory)
            try:
                self.assertEqual(len(sync.written_problems()), len(sync.signal_tokens.documents(json.loads(sync.DESIGN.read_text()))))
                sync.signal_tokens.OUT = original
                self.assertEqual(sync.written_problems(), [])
            finally:
                sync.signal_tokens.OUT = original


class Repository(unittest.TestCase):
    def test_the_repository_copy_its_record_and_the_written_files_agree(self):
        self.assertEqual(sync.check(), [])

    def test_the_recorded_hash_is_the_hash_of_the_copy(self):
        recorded = json.loads(sync.RECORD.read_text())
        self.assertEqual(recorded["sha256"], sync.digest(json.loads(sync.DESIGN.read_text())))


if __name__ == "__main__":
    unittest.main()
