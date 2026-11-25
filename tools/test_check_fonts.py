#!/usr/bin/env python3
"""Tests of tools/check_fonts.py: ctest check_fonts_tool."""

import contextlib
import io
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import check_fonts as tool  # noqa: E402


def program(files: dict[str, str], *, defined=(), include_dirs=()) -> list[str]:
    """The missing fonts of a program of `files`, the first of them a source; `include_dirs` are named under the root."""
    with tempfile.TemporaryDirectory() as d:
        paths = []
        for name, text in files.items():
            p = pathlib.Path(d) / name
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_text(text)
            paths.append(p)
        sources = [p for p in paths if p.suffix == ".cpp"]
        return tool.missing_fonts(tool.program_files(sources, [pathlib.Path(d) / i for i in include_dirs]), set(defined))


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

    def test_a_font_of_another_design_is_told_apart_and_names_the_library(self):
        use = 'auto f = lv::font_of<"text.title", lv::signal::type_design>;\n'
        (message,) = program({"main.cpp": use + USE, "font_text_title.cpp": DEFINE})
        self.assertIn("the font 'text.title' of the design lv::signal::type_design has no file that draws it", message)
        self.assertIn("Link the library that draws it", message)

    def test_a_library_defines_the_fonts_of_its_design(self):
        use = 'auto f = lv::font_of<"text.title", lv::signal::type_design>;\n'
        self.assertEqual(program({"main.cpp": use}, defined={"text.title@lv::signal::type_design"}), [])

    def test_a_definition_of_one_design_is_none_for_the_other(self):
        define = 'template <> const lv::drawn_font lv::font_object<"text.title", other> = lv::draw<"text.title", other>(t);\n'
        self.assertEqual(len(program({"main.cpp": USE, "font_text_title.cpp": define})), 1)

    def test_a_header_reached_through_an_include_directory_is_followed(self):
        files = {"src/main.cpp": '#include "design/design.hpp"\n', "include/design/design.hpp": USE}
        self.assertEqual(len(program(files, include_dirs=["include"])), 1)

    def test_a_definition_reached_through_an_include_directory_counts(self):
        files = {
            "src/main.cpp": '#include "design/design.hpp"\n#include "design/fonts.hpp"\n',
            "include/design/design.hpp": USE,
            "include/design/fonts.hpp": DEFINE,
        }
        self.assertEqual(program(files, include_dirs=["include"]), [])

    def test_an_angle_include_of_the_program_is_followed_through_a_directory(self):
        files = {"src/main.cpp": "#include <design/design.hpp>\n", "include/design/design.hpp": USE}
        self.assertEqual(len(program(files, include_dirs=["include"])), 1)

    def test_an_include_that_no_directory_has_is_a_warning_never_a_pass_without_a_word(self):
        files = {"src/main.cpp": '#include "design/design.hpp"\n', "include/design/design.hpp": USE}
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            found = program(files)
        self.assertEqual(found, [])
        self.assertIn('main.cpp:1: warning: check_fonts.py could not find the include "design/design.hpp"', err.getvalue())

    def test_the_files_of_lvgl26_and_the_library_are_not_warned_about(self):
        err = io.StringIO()
        with contextlib.redirect_stderr(err):
            program({"main.cpp": "#include <lv/lv.hpp>\n#include <vector>\n"})
        self.assertEqual(err.getvalue(), "")

    def test_the_file_of_a_name_with_a_dash(self):
        self.assertEqual(tool.file_name_of("text.body-large"), "font_text_body_large.cpp")


if __name__ == "__main__":
    unittest.main()
