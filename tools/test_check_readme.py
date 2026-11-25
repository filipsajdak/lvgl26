#!/usr/bin/env python3
"""Tests of tools/check_readme.py. Run: python3 tools/test_check_readme.py"""

import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import check_readme as cr  # noqa: E402

CMAKE = "if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16.2)\n"
EXAMPLES = ("| LVGL `get_started` | 4 | 4 | Same pixels |\n"
            "| LVGL `widgets`: label | 48 | 52 | Same pixels |\n"
            "| Qt 6 examples | 1 | about 60 | Screenshots |\n"
            "Of LVGL v9.6.0's 163 widget examples, 75 are ported.\n")
CLAUDE = "# x\n\n## Build and test\n\n```sh\ncmake --preset host-v96 && ctest --preset host-v96\n```\n"


class OldForms(unittest.TestCase):
    def test_the_call_form_passes(self):
        self.assertEqual(cr.old_form_problems("lv::column({.size = s}, lv::label({.text = \"a\"}))"), [])

    def test_children_in_brackets_fail(self):
        self.assertEqual(len(cr.old_form_problems("lv::button({.on_clicked = f})[lv::label()]")), 1)
        self.assertEqual(len(cr.old_form_problems("constexpr auto ui = lv::column[lv::label()];")), 1)

    def test_a_state_of_a_plain_int_fails(self):
        self.assertEqual(len(cr.old_form_problems("lv::state<int> speed;")), 1)


class CNames(unittest.TestCase):
    def test_a_c_name_fails(self):
        self.assertEqual(cr.c_name_problems("call lv_obj_create and LV_ALIGN_CENTER"),
                         ["a name of LVGL's C API outside a block marked <!-- c-api: shown -->: LV_ALIGN_CENTER",
                          "a name of LVGL's C API outside a block marked <!-- c-api: shown -->: lv_obj_create"])

    def test_a_marked_block_may_name_them(self):
        self.assertEqual(cr.c_name_problems("<!-- c-api: shown -->\n```cpp\nlv_obj_create(p);\n```\n"), [])

    def test_names_of_lvgl26_are_no_c_names(self):
        self.assertEqual(cr.c_name_problems("lvgl26_editor_support() and lv::state"), [])


class Links(unittest.TestCase):
    def test_a_link_to_a_missing_file_fails(self):
        self.assertEqual(cr.link_problems("[a](docs/none.md) [b](docs/state.md) [c](https://x.org)", cr.ROOT),
                         ["the link to docs/none.md leads nowhere"])


class Facts(unittest.TestCase):
    def test_the_gcc_version_is_the_cmake_minimum(self):
        self.assertEqual(cr.gcc_problems("GCC 16.2 or newer", CMAKE), [])
        self.assertEqual(cr.gcc_problems("GCC 16.1 or newer", CMAKE),
                         ["it asks for GCC 16.1 or newer, and CMakeLists.txt for 16.2"])

    def test_a_version_that_is_no_requirement_is_left_alone(self):
        self.assertEqual(cr.gcc_problems("fixed in GCC 16.3", CMAKE), [])

    def test_the_counts_are_those_of_the_examples(self):
        self.assertEqual(cr.example_counts(EXAMPLES), {"widget": (75, 163), "all": (52, 56)})
        self.assertEqual(cr.count_problems("75 of LVGL's 163 widget examples, 52 of LVGL's 56 examples", EXAMPLES), [])
        self.assertEqual(len(cr.count_problems("74 of LVGL's 163 widget examples", EXAMPLES)), 1)
        self.assertEqual(len(cr.count_problems("79 of LVGL's 299 examples", EXAMPLES)), 1)

    def test_more_than_n_compile_fail_tests_must_hold(self):
        self.assertEqual(cr.fail_count_problems("More than 340 [compile-fail tests](tests/fail)", 344), [])
        self.assertEqual(cr.fail_count_problems("more than 340 compile-fail tests", 340),
                         ["it says more than 340 compile-fail tests, and tests/fail/ holds 340"])

    def test_the_build_line_is_that_of_claude_md(self):
        self.assertEqual(cr.build_line_problems("cmake --preset host-v96 && ctest --preset host-v96\n", CLAUDE), [])
        self.assertEqual(len(cr.build_line_problems("cmake --preset host-v10 && ctest --preset host-v10\n", CLAUDE)), 1)


class TheReadme(unittest.TestCase):
    def test_the_readme_of_the_repository_passes(self):
        self.assertEqual(cr.problems(cr.README.read_text()), [])


if __name__ == "__main__":
    unittest.main()
