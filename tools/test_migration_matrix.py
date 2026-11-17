#!/usr/bin/env python3
"""Unit tests of tools/migration_matrix.py: python3 tools/test_migration_matrix.py"""

import copy
import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import migration_matrix as matrix  # noqa: E402

STATUSES = {"works": "Works", "partial": "Partial", "in-review": "In review", "planned": "Planned", "not-planned": "Not planned"}
GAP = {"id": "lists", "title": "Lists", "what": "A list.", "status": "planned", "unblocks": ["Qt Repeater"]}


def data():
    return {
        "updated": "2026-01-01",
        "statuses": STATUSES,
        "gaps": [copy.deepcopy(GAP)],
        "frameworks": [{"id": "qt", "name": "Qt Quick", "page": "docs/from-qt.md",
                        "rows": [{"concept": "Text", "status": "works", "answer": "lv::label"},
                                 {"concept": "Repeater", "status": "planned", "answer": "Not built", "gap": "lists"}]}],
    }


class Problems(unittest.TestCase):
    def test_good_data_has_none(self):
        self.assertEqual(matrix.problems(data()), [])

    def test_a_row_names_a_gap_that_does_not_exist(self):
        d = data()
        d["frameworks"][0]["rows"][1]["gap"] = "nothing"
        self.assertIn("qt: Repeater names the unknown gap nothing", matrix.problems(d))

    def test_a_row_that_does_not_work_names_a_gap(self):
        d = data()
        del d["frameworks"][0]["rows"][1]["gap"]
        self.assertIn("qt: Repeater does not work and names no gap", matrix.problems(d))

    def test_a_gap_in_review_names_its_pull_request(self):
        d = data()
        d["gaps"][0]["status"] = "in-review"
        self.assertIn("gap lists is in review and names no pull request", matrix.problems(d))

    def test_an_unknown_status_and_a_duplicate_gap(self):
        d = data()
        d["gaps"].append(copy.deepcopy(GAP))
        d["frameworks"][0]["rows"][0]["status"] = "mostly"
        found = matrix.problems(d)
        self.assertIn("gap lists is listed twice", found)
        self.assertIn("qt: Text has the unknown status mostly", found)

    def test_a_missing_page(self):
        d = data()
        d["frameworks"][0]["page"] = "docs/nowhere.md"
        self.assertIn("qt: the page docs/nowhere.md does not exist", matrix.problems(d))


class Writing(unittest.TestCase):
    def test_counts(self):
        self.assertEqual(matrix.counts(data()["frameworks"][0]), (1, 2))

    def test_code_is_set_in_code_and_the_rest_is_escaped(self):
        self.assertEqual(matrix.code("Use lv::bind and <=>"), "Use <code>lv::bind</code> and &lt;=&gt;")

    def test_the_section_has_the_anchors_the_site_links_to(self):
        text = matrix.section(data())
        self.assertIn('id="migration-qt"', text)
        self.assertIn('id="gap-lists"', text)
        self.assertIn("1 of 2 concepts work", text)

    def test_a_block_is_replaced_and_a_second_run_changes_nothing(self):
        page = 'a\n<!-- ==== -->\n\n<section class="band tint" id="limits">x</section>\n'
        once = matrix.replaced(page, matrix.section(data()))
        self.assertIn(matrix.START, once)
        self.assertEqual(matrix.replaced(once, matrix.section(data())), once)

    def test_the_real_data_is_sound_and_the_page_is_current(self):
        import json
        real = json.loads(matrix.DATA.read_text())
        self.assertEqual(matrix.problems(real), [])
        self.assertEqual(matrix.replaced(matrix.PAGE.read_text(), matrix.section(real)), matrix.PAGE.read_text())


if __name__ == "__main__":
    unittest.main()
