#!/usr/bin/env python3
"""Unit tests of tools/check_tokens.py: python3 tools/test_check_tokens.py"""

import copy
import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import check_tokens as gate  # noqa: E402

RED = {"colorSpace": "srgb", "components": [1, 0, 0], "hex": "#ff0000"}
CONFORMING = {"color": {"$type": "color", "red": {"$value": RED}}, "space": {"$type": "dimension", "gap": {"$value": {"value": 8, "unit": "px"}}}}
LEGACY = {"color": {"$type": "color", "red": {"$value": "#ff0000"}}}
RESOLVER = {"version": "2025.10", "resolutionOrder": [{"$ref": "tokens.json"}]}


class Gate(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.checkers = gate.validators()

    def found(self, document):
        return gate.file_violations(self.checkers, copy.deepcopy(document))


class PushDown(unittest.TestCase):
    def test_a_token_takes_the_type_of_its_group(self):
        doc = {"a": {"$type": "color", "b": {"c": {"$value": "x"}}}}
        gate.push_types(doc)
        self.assertEqual(doc["a"]["b"]["c"]["$type"], "color")

    def test_a_token_keeps_its_own_type_and_the_nearest_group_wins(self):
        doc = {"$type": "color", "a": {"$type": "number", "b": {"$value": 1}, "c": {"$type": "dimension", "$value": 1}}}
        gate.push_types(doc)
        self.assertEqual((doc["a"]["b"]["$type"], doc["a"]["c"]["$type"]), ("number", "dimension"))

    def test_a_root_token_takes_the_type_of_its_group(self):
        doc = {"a": {"$type": "number", "$root": {"$value": 1}}}
        gate.push_types(doc)
        self.assertEqual(doc["a"]["$root"]["$type"], "number")

    def test_a_typeless_token_stays_typeless(self):
        doc = {"a": {"b": {"$value": 1}}}
        gate.push_types(doc)
        self.assertNotIn("$type", doc["a"]["b"])


class Schema(Gate):
    def test_a_conforming_file_passes(self):
        self.assertEqual(self.found(CONFORMING), [])

    def test_a_type_from_the_group_is_checked(self):
        found = self.found(LEGACY)
        self.assertEqual([(v.path, v.kind) for v in found], [("color.red.$value", "color $value: type")])

    def test_a_member_of_a_composite_is_named(self):
        doc = {"t": {"$type": "border", "b": {"$value": {"color": RED, "width": "1px", "style": "solid"}}}}
        self.assertEqual([v.path for v in self.found(doc)], ["t.b.$value.width"])

    def test_a_missing_type_is_a_violation(self):
        self.assertTrue(self.found({"a": {"b": {"$value": 1}}}))

    def test_a_bad_group_property_is_a_violation(self):
        doc = copy.deepcopy(CONFORMING)
        doc["color"]["$description"] = 5
        self.assertEqual([v.path for v in self.found(doc)], ["color.$description"])

    def test_a_resolver_document_uses_the_resolver_schema(self):
        self.assertEqual(self.found(RESOLVER), [])
        self.assertTrue(self.found({"resolutionOrder": 5}))


class Exceptions(Gate):
    def violations(self, name, document):
        return {name: self.found(document)}

    def test_a_listed_violation_passes(self):
        self.assertEqual(gate.differences(self.violations("a.json", LEGACY), {"a.json": {"color $value: type": 1}}), [])

    def test_an_unlisted_violation_fails_and_names_the_token(self):
        out = gate.differences(self.violations("a.json", LEGACY), {})
        self.assertIn("1 new violation", out[0])
        self.assertIn("color.red.$value", out[1])

    def test_a_stale_exception_fails(self):
        out = gate.differences({"a.json": []}, {"a.json": {"color $value: type": 1}})
        self.assertIn("stale exception", out[0])

    def test_a_count_that_dropped_fails(self):
        out = gate.differences(self.violations("a.json", LEGACY), {"a.json": {"color $value: type": 2}})
        self.assertIn("stale exception", out[0])

    def test_counts_group_by_file_and_kind(self):
        found = {"a.json": self.found(LEGACY) * 2}
        self.assertEqual(gate.counts(found), {"a.json": {"color $value: type": 2}})


class Repository(unittest.TestCase):
    def test_the_repository_passes_the_gate(self):
        self.assertEqual(gate.main([]), 0)


if __name__ == "__main__":
    unittest.main()
