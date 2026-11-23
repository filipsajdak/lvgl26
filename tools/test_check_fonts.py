#!/usr/bin/env python3
"""Tests of tools/check_fonts.py: ctest check_fonts_tool."""

import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import check_fonts as tool  # noqa: E402


def program(files: dict[str, str], *, defined=()) -> list[str]:
    with tempfile.TemporaryDirectory() as d:
        paths = []
        for name, text in files.items():
            p = pathlib.Path(d) / name
            p.write_text(text)
            paths.append(p)
        return tool.missing_fonts(tool.program_files(paths), set(defined))


USE = 'constexpr auto f = lv::font_of<"text.title">;\n'
DEFINE = 'template <> const lv::drawn_font lv::font_object<"text.title"> = lv::draw<"text.title">(type);\n'


class Fonts(unittest.TestCase):
    def test_a_font_with_its_file_is_fine(self):
        self.assertEqual(program({"main.cpp": USE, "font_text_title.cpp": DEFINE}), [])

    def test_a_font_with_no_file_is_named_with_the_file_to_add(self):
        (message,) = program({"main.cpp": "\n" + USE})
        self.assertIn("main.cpp:2: the font 'text.title' has no file that draws it", message)
        self.assertIn("Add font_text_title.cpp", message)
        self.assertIn('lv::font_object<"text.title"> = lv::draw<"text.title">(type);', message)

    def test_a_use_in_an_included_header_is_found(self):
        self.assertEqual(len(program({"main.cpp": '#include "screens.hpp"\n', "screens.hpp": USE})), 1)

    def test_a_definition_in_an_included_file_counts(self):
        self.assertEqual(program({"main.cpp": '#include "fonts.hpp"\n' + USE, "fonts.hpp": DEFINE}), [])

    def test_a_font_that_a_library_draws_is_defined(self):
        self.assertEqual(program({"main.cpp": USE}, defined={"text.title"}), [])

    def test_a_name_in_a_comment_or_a_text_is_no_use(self):
        text = '// lv::font_of<"text.title">\n/* lv::font_of<"text.title"> */\nconst char* s = "font_of<\\"text.title\\">";\n'
        self.assertEqual(program({"main.cpp": text}), [])

    def test_each_missing_font_is_listed_once(self):
        text = USE + USE + 'auto g = lv::font_of<"text.body">;\n'
        self.assertEqual(len(program({"main.cpp": text})), 2)

    def test_the_file_of_a_name_with_a_dash(self):
        self.assertEqual(tool.file_name_of("text.body-large"), "font_text_body_large.cpp")


if __name__ == "__main__":
    unittest.main()
