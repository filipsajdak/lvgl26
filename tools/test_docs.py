#!/usr/bin/env python3
"""Tests of tools/docs.py: the outline of a guide, its messages and its rules. Run: python3 tools/test_docs.py"""

import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import docs  # noqa: E402
import rules  # noqa: E402

PAGE = docs.DOCS / "example-guide.md"  # a name only: no page is read from it
FAIL = docs.source_of("tests/fail/each_plain_key.cpp", None)
MESSAGE = ("error: static assertion failed: lv: a key names one row, and is compared as a state compares its value. "
           "Give it a named type, `struct track_id { int value; };`, or an enum. To allow plain integers, "
           "set rules.raw_integer_state in the settings.\n")
COSTS = "\n".join(["| Cost | What we know | Where it comes from |", "|---|---|---|"] +
                  [f"| {c} | not measured yet | not measured yet |" for c in docs.COSTS]) + "\n"


def guide(**replace: str) -> str:
    """A page in the outline, with one section's text replaced."""
    body = {
        "The smallest example": "It shows a list.\n",
        "How it works": "Rows follow the list.\n",
        "What you can do": "Change rows.\n",
        "What the compiler checks": f"<!-- snippet: tests/fail/each_plain_key.cpp -->\n```cpp\n{FAIL}```\n\n"
                                    f"<!-- message: each_plain_key -->\n```text\n{MESSAGE}```\n",
        "What it costs": COSTS,
        "Rules": docs.rules_block("no-boilerplate-4"),
        "Limits": "See the [status page](https://example.org/status.html).\n",
        "Names": "See the [reference](https://example.org/docs/reference/lvgl-v9.6.0.json).\n",
    }
    body.update({k.replace("_", " "): v for k, v in replace.items()})
    opening = "# A guide\n\nOne sentence of what it is for.\n\nHeader: `<lv/list.hpp>`\n\n"
    return opening + "".join(f"## {title}\n\n{text}\n" for title, text in body.items())


def problems(text: str) -> list[str]:
    return [p.split(": ", 1)[1] for p in docs.outline_problems(PAGE, text) + docs.message_problems(PAGE, text)]


class Outline(unittest.TestCase):
    def test_a_page_in_the_outline_passes(self):
        self.assertEqual(problems(guide()), [])

    def test_a_missing_section_fails(self):
        text = guide().replace("## Limits\n", "## Other\n")
        self.assertTrue(any("missing ['Limits']" in p for p in problems(text)))

    def test_sections_out_of_order_fail(self):
        text = guide().replace("## How it works", "## SWAP").replace("## What you can do", "## How it works")
        text = text.replace("## SWAP", "## What you can do")
        self.assertTrue(any("out of order" in p for p in problems(text)))

    def test_a_heading_inside_code_is_no_section(self):
        found = docs.sections("## A\n\n```sh\n## not a heading\n```\n\n## B\ntext\n")
        self.assertEqual(list(found), ["A", "B"])

    def test_an_empty_section_says_why(self):
        self.assertTrue(any("How it works: the section is empty" in p for p in problems(guide(How_it_works=""))))

    def test_the_line_header_is_third(self):
        text = guide().replace("Header: `<lv/list.hpp>`\n\n", "")
        self.assertTrue(any("`Header:`" in p for p in problems(text)))

    def test_the_line_under_the_title_is_one_sentence(self):
        text = guide().replace("One sentence of what it is for.", "Two sentences. Here.")
        self.assertTrue(any("one sentence" in p for p in problems(text)))

    def test_an_example_that_needs_an_opt_in_header_names_it(self):
        snippet = "<!-- snippet: examples/x.cpp a -->\n```cpp\nconstinit lv::list<int, 2> rows;\n```\n"
        text = guide(What_you_can_do=snippet).replace("`<lv/list.hpp>`", "`<lv/lv.hpp>`")
        self.assertIn("the line `Header:` lacks <lv/list.hpp>, which the examples need", problems(text))

    def test_every_cpp_block_is_a_snippet(self):
        text = guide(What_you_can_do="```cpp\nint x;\n```\n")
        self.assertIn("1 C++ block(s) without a `<!-- snippet: -->` line: show a part of a built file", problems(text))
        self.assertEqual(problems(guide(What_you_can_do="```sh\ncmake --build build\n```\n")), [])

    def test_a_header_that_does_not_exist_fails(self):
        text = guide().replace("`<lv/list.hpp>`", "`<lv/lists.hpp>`")
        self.assertIn("the header <lv/lists.hpp> does not exist", problems(text))


class CompilerChecks(unittest.TestCase):
    def test_a_file_of_tests_fail_is_a_snippet(self):
        self.assertIn("lv::each(tracks, &track::id)", FAIL)

    def test_a_mistake_needs_its_message(self):
        section = f"<!-- snippet: tests/fail/each_plain_key.cpp -->\n```cpp\n{FAIL}```\n"
        self.assertTrue(any("shown without `<!-- message: each_plain_key -->`" in p
                            for p in problems(guide(What_the_compiler_checks=section))))

    def test_a_message_must_show_what_its_test_pins(self):
        text = guide().replace("a key names one row", "a key is one row")
        self.assertTrue(any("differs from what the test each_plain_key pins" in p for p in problems(text)))

    def test_a_message_names_a_test_that_exists(self):
        text = guide().replace("message: each_plain_key", "message: no_such_test")
        self.assertTrue(any("no compile-fail test is called no_such_test" in p for p in problems(text)))

    def test_other_code_there_is_refused(self):
        section = "```cpp\nint x;\n```\n"
        self.assertTrue(any("a file of tests/fail/" in p for p in problems(guide(What_the_compiler_checks=section))))

    def test_a_section_that_does_not_apply_says_so(self):
        self.assertEqual(problems(guide(What_the_compiler_checks="Nothing here can be wrong.\n")), [])


class Costs(unittest.TestCase):
    def test_every_cost_has_a_row(self):
        text = guide(What_it_costs=COSTS.replace("| Stack |", "| Heap |"))
        self.assertTrue(any("the table of costs has the rows" in p for p in problems(text)))

    def test_a_number_names_its_source(self):
        text = guide(What_it_costs=COSTS.replace("| RAM | not measured yet | not measured yet |", "| RAM | 960 bytes | |"))
        self.assertTrue(any("RAM links its source" in p for p in problems(text)))

    def test_a_claim_without_a_source_is_refused(self):
        row = "| RAM | 960 bytes | not measured yet |"
        text = guide(What_it_costs=COSTS.replace("| RAM | not measured yet | not measured yet |", row))
        self.assertTrue(any("RAM has no source" in p for p in problems(text)))

    def test_a_linked_source_passes(self):
        row = "| RAM | 960 bytes a row | [lists.md](https://example.org/lists.md), 2026-11-17 |"
        text = guide(What_it_costs=COSTS.replace("| RAM | not measured yet | not measured yet |", row))
        self.assertEqual(problems(text), [])


class Rules(unittest.TestCase):
    def test_the_block_lists_each_rule_with_its_sentence(self):
        block = docs.rules_block("no-boilerplate-4")
        sentence = next(r["rule"] for r in rules.load()["rules"] if r["id"] == "no-boilerplate-4")
        self.assertIn(f"- [`no-boilerplate-4`](rules.md#no-boilerplate-4): {sentence}\n", block)

    def test_a_stale_block_fails_and_sync_writes_it(self):
        stale = "<!-- rules: no-boilerplate-4 -->\n- old text\n<!-- rules: end -->\n"
        self.assertTrue(any("are stale" in p for p in docs.rules_problems(PAGE, stale)))
        self.assertEqual(docs.synced(stale), docs.rules_block("no-boilerplate-4"))

    def test_an_unknown_rule_fails(self):
        text = "<!-- rules: no-such-1 -->\n<!-- rules: end -->\n"
        self.assertTrue(any("no rule is called no-such-1" in p for p in docs.rules_problems(PAGE, text)))

    def test_the_rules_section_holds_a_block(self):
        self.assertTrue(any("Rules: the section holds a block" in p for p in problems(guide(Rules="None.\n"))))

    def test_a_guide_lists_the_rules_that_name_it(self):
        page = docs.DOCS / "lists.md"
        missing = docs.guide_rule_problems(page, docs.rules_block("no-boilerplate-4"))
        self.assertTrue(any("fast-run-2" in p for p in missing))


class OutlineList(unittest.TestCase):
    def test_a_name_that_is_no_page_fails(self):
        self.assertEqual(docs.outline_name_problems({"lists.md", "list.md"}),
                         ["docs/docs.json: the outline names list.md, which is no page"])


class Links(unittest.TestCase):
    def test_limits_link_the_status_page(self):
        self.assertTrue(any("status page" in p for p in problems(guide(Limits="Nothing yet.\n"))))

    def test_names_link_the_reference_data(self):
        self.assertTrue(any("reference data" in p for p in problems(guide(Names="None.\n"))))


if __name__ == "__main__":
    unittest.main()
