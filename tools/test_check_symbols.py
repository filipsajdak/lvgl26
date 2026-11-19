#!/usr/bin/env python3
"""Unit tests of tools/check_symbols.py: python3 tools/test_check_symbols.py"""

import pathlib
import shutil
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import check_symbols as symbols  # noqa: E402

NM = "__Z3foov T 0 0\n__ZTAXtlA1_Kv U 0 0\n\n_main T 10 0\n"


class Parsing(unittest.TestCase):
    def test_names_are_the_first_field_and_blank_lines_are_skipped(self):
        self.assertEqual(symbols.names(NM), ["__Z3foov", "__ZTAXtlA1_Kv", "_main"])

    def test_only_names_over_the_budget_are_listed_longest_first(self):
        found = symbols.too_long(["a" * 5, "b" * 10, "c" * 7], 6)
        self.assertEqual([len(s) for s in found], [10, 7])

    def test_a_name_at_the_budget_is_allowed(self):
        self.assertEqual(symbols.too_long(["a" * 6], 6), [])


@unittest.skipUnless(shutil.which("cc") and shutil.which("nm"), "needs cc and nm")
class Command(unittest.TestCase):
    def object_with(self, length):
        directory = pathlib.Path(tempfile.mkdtemp())
        (directory / "x.c").write_text("int " + "v" * length + " = 1;\n")
        subprocess.run(["cc", "-c", str(directory / "x.c"), "-o", str(directory / "x.o")], check=True)
        return directory / "x.o"

    def test_a_long_name_fails_and_a_short_one_passes(self):
        long_object, short_object = self.object_with(500), self.object_with(20)
        self.assertEqual(symbols.main(["--budget", "100", str(long_object)]), 1)
        self.assertEqual(symbols.main(["--budget", "100", str(short_object)]), 0)

    def test_a_budget_over_the_linker_limit_is_refused(self):
        with self.assertRaises(SystemExit):
            symbols.main(["--budget", str(symbols.LINKER_LIMIT + 1), "x.o"])


if __name__ == "__main__":
    unittest.main()
