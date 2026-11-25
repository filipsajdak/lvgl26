#!/usr/bin/env python3
"""Tests of tools/check_includes.py. Run: python3 tools/test_check_includes.py"""

import contextlib
import io
import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import check_includes as tool  # noqa: E402

TABLE, _ = tool.load_table()


def check(files: dict[str, str], main: str = "main.cpp", exact: bool = False) -> list[str]:
    """The problems of `main`, with the other files beside it."""
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        for name, text in files.items():
            (root / name).write_text(text)
        return [p.split(": ", 1)[1] for p in tool.problems(root / main, TABLE, exact)]


class ThroughIncludeDirectories(unittest.TestCase):
    """A header reached through an include directory of the target counts, as one beside the file does."""

    def layout(self, root: pathlib.Path) -> pathlib.Path:
        (root / "include" / "design").mkdir(parents=True)
        (root / "src").mkdir()
        (root / "include" / "design" / "design.hpp").write_text("#include <lv/list.hpp>\n")
        main = root / "src" / "main.cpp"
        main.write_text('#include <lv/lv.hpp>\n#include "design/design.hpp"\nlv::list<int, 2> rows;\n')
        return main

    def test_the_include_of_a_header_reached_through_a_directory_satisfies_a_name(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            main = self.layout(root)
            self.assertEqual(tool.problems(main, TABLE, include_dirs=[root / "include"]), [])

    def test_without_the_directory_the_header_is_not_seen_and_the_include_is_reported(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            main = self.layout(root)
            self.assertEqual(len(tool.problems(main, TABLE)), 1)
            err = io.StringIO()
            with contextlib.redirect_stderr(err):
                tool.followed(main)
            self.assertIn('warning: check_includes.py could not find the include "design/design.hpp"', err.getvalue())

    def test_followed_gives_the_header_of_the_directory(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            main = self.layout(root)
            names = [f.name for f in tool.followed(main, [root / "include"])]
            self.assertEqual(names, ["main.cpp", "design.hpp"])


class CheckIncludes(unittest.TestCase):
    def test_a_name_of_an_opt_in_header_needs_it(self):
        found = check({"main.cpp": "#include <lv/lv.hpp>\nlv::list<int, 2> rows;\n"})
        self.assertEqual(found, ["lv::list needs #include <lv/list.hpp>"])

    def test_the_line_of_the_name_is_given(self):
        problems = []
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / "main.cpp"
            path.write_text("#include <lv/lv.hpp>\n\nconstexpr auto t = lv::theme({});\n")
            problems = tool.problems(path, TABLE)
        self.assertTrue(problems[0].endswith("main.cpp:3: lv::theme needs #include <lv/theme.hpp>"))

    def test_the_header_or_all_hpp_satisfies_it(self):
        self.assertEqual(check({"main.cpp": "#include <lv/list.hpp>\nlv::list<int, 2> rows;\n"}), [])
        self.assertEqual(check({"main.cpp": "#include <lv/all.hpp>\nlv::list<int, 2> rows;\n"}), [])

    def test_a_header_that_includes_it_satisfies_it(self):
        # lv/look.hpp includes lv/list.hpp.
        self.assertEqual(check({"main.cpp": "#include <lv/look.hpp>\nlv::list<int, 2> rows;\n"}), [])

    def test_a_header_of_the_user_counts(self):
        files = {"parts.hpp": "#pragma once\n#include <lv/list.hpp>\n", "main.cpp": '#include "parts.hpp"\nlv::list<int, 2> rows;\n'}
        self.assertEqual(check(files), [])

    def test_comments_and_strings_are_not_read(self):
        text = '#include <lv/lv.hpp>\n// lv::list<int, 2>\nconst char* s = "lv::theme";\n/* lv::look */\n'
        self.assertEqual(check({"main.cpp": text}), [])

    def test_a_name_of_the_core_needs_nothing(self):
        self.assertEqual(check({"main.cpp": "#include <lv/lv.hpp>\nconstexpr auto ui = lv::label();\n"}), [])

    def test_bind_of_a_member_needs_the_list(self):
        found = check({"main.cpp": "#include <lv/lv.hpp>\nauto b = lv::bind(&row::name);\n"})
        self.assertEqual(found, ["lv::bind(&row::field) needs #include <lv/list.hpp>"])

    def test_exact_reports_an_unused_header_and_all_hpp(self):
        unused = check({"main.cpp": "#include <lv/lv.hpp>\n#include <lv/theme.hpp>\n"}, exact=True)
        self.assertEqual(unused, ["<lv/theme.hpp> is included and not used"])
        everything = check({"main.cpp": "#include <lv/all.hpp>\n"}, exact=True)
        self.assertEqual(everything, ["includes <lv/all.hpp>; an example includes the headers it uses"])

    def test_the_table_names_each_opt_in_header(self):
        headers = {h for hs in TABLE.values() for h in hs}
        for header in ("lv/theme.hpp", "lv/look.hpp", "lv/font.hpp", "lv/scaled_font.hpp", "lv/image.hpp",
                       "lv/encoded_image.hpp", "lv/list.hpp", "lv/repeat.hpp", "lv/runtime.hpp", "lv/one_of.hpp",
                       "lv/pointer.hpp"):
            self.assertIn(header, headers)
        self.assertNotIn("lv/lv.hpp", headers)

    def test_the_build_form_writes_a_stamp_and_a_depfile(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            (root / "parts.hpp").write_text("#pragma once\n#include <lv/list.hpp>\n")
            (root / "main.cpp").write_text('#include "parts.hpp"\nlv::list<int, 2> rows;\n')
            stamp, depfile = root / "out" / "main.stamp", root / "out" / "main.d"
            self.assertEqual(tool.build([str(stamp), str(depfile), str(root / "main.cpp")]), 0)
            self.assertTrue(stamp.exists())
            depends = depfile.read_text()
            self.assertIn("main.cpp", depends)
            self.assertIn("parts.hpp", depends)
            self.assertIn("headers.json", depends)

    def test_the_build_form_fails_on_a_header_of_the_program(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            (root / "parts.hpp").write_text("#pragma once\nlv::list<int, 2> rows;\n")
            (root / "main.cpp").write_text('#include "parts.hpp"\n')
            stamp = root / "main.stamp"
            self.assertEqual(tool.build([str(stamp), str(root / "main.d"), str(root / "main.cpp")]), 1)
            self.assertFalse(stamp.exists())


if __name__ == "__main__":
    unittest.main()
