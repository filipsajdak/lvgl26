#!/usr/bin/env python3
"""Unit tests of tools/compile_report.py: python3 tools/test_compile_report.py"""

import json
import pathlib
import subprocess
import sys
import unittest

TOOLS = pathlib.Path(__file__).resolve().parent
ROOT = TOOLS.parent
DATA = TOOLS / "testdata"
sys.path.insert(0, str(TOOLS))

import compile_report as report  # noqa: E402

BUILD = ROOT / "build" / "host-v96"
MACOS_BYTES = 315129856
MB_OF_MACOS_SAMPLE = MACOS_BYTES / 1024 / 1024
LINUX_SAMPLE_KB = 307984


def text(name):
    return (DATA / name).read_text()


class ParseTime(unittest.TestCase):
    def test_macos_gives_bytes_as_mb_and_instructions(self):
        got = report.parse_time(text("time_macos.txt"))
        self.assertAlmostEqual(got["peak_rss_mb"], MB_OF_MACOS_SAMPLE, places=1)
        self.assertIsInstance(got["instructions"], int)
        self.assertGreater(got["instructions"], 0)

    def test_linux_gives_kbytes_as_mb_and_no_instructions(self):
        got = report.parse_time(text("time_linux.txt"))
        self.assertAlmostEqual(got["peak_rss_mb"], LINUX_SAMPLE_KB / 1024, places=1)
        self.assertIsNone(got["instructions"])

    def test_unknown_text_gives_nulls(self):
        self.assertEqual(report.parse_time("nothing"), {"peak_rss_mb": None, "instructions": None})


class ParseReport(unittest.TestCase):
    def test_rows_of_the_real_sample(self):
        got = report.parse_report(text("time_report.sarif"), analyzer_on=False)
        self.assertGreater(got["constexpr_s"], 0)
        self.assertGreater(got["total_s"], got["constexpr_s"])
        self.assertGreater(got["parsing_s"], 0)
        self.assertIsNone(got["analyzer_s"])

    def test_analyzer_rows_are_summed_when_the_analyzer_ran(self):
        sarif = {"runs": [{"invocations": [{"properties": {"gcc/timeReport": {"timevars": [
            {"name": "Analyzer: processing worklist", "elapsed": {"wall": 2.0}},
            {"name": "analyzer: other", "elapsed": {"wall": 0.5}},
            {"name": "TOTAL", "elapsed": {"wall": 3.0}}]}}}]}]}
        got = report.parse_report(json.dumps(sarif), analyzer_on=True)
        self.assertEqual(got["analyzer_s"], 2.5)
        self.assertEqual(got["constexpr_s"], 0.0)

    def test_a_report_without_properties_gives_zeros(self):
        got = report.parse_report('{"runs":[{"invocations":[{}]}]}', analyzer_on=False)
        self.assertEqual(got["total_s"], 0.0)


class Rewrite(unittest.TestCase):
    def test_strip_drops_outputs_and_dependency_files_but_keeps_the_ops_limit(self):
        tokens = "g++ -Ia -MD -MT x.o -MF x.d -fconstexpr-ops-limit=5 -o x.o -c f.cpp".split()
        self.assertEqual(report.strip_options(tokens), ["g++", "-Ia", "-fconstexpr-ops-limit=5", "f.cpp"])

    def test_the_cc1plus_line_of_the_real_build_keeps_the_ops_limit(self):
        cc1 = report.find_cc1(text("driver_dry_run.txt"))
        self.assertTrue(any(t.startswith("-fconstexpr-ops-limit=") for t in cc1))

    def test_probing_replaces_the_limit(self):
        self.assertEqual(report.without_ops_limit(["a", "-fconstexpr-ops-limit=9", "b"]), ["a", "b"])

    def test_cc1plus_line_is_found_in_the_driver_dry_run(self):
        cc1 = report.find_cc1(text("driver_dry_run.txt"))
        self.assertTrue(cc1[0].endswith("/cc1plus"))
        self.assertIn("-std=gnu++26", cc1)

    def test_the_assembler_output_goes_to_dev_null(self):
        cc1 = report.find_cc1(text("driver_dry_run.txt"))
        out = report.with_output_to_null(cc1)
        self.assertEqual(out[out.index("-o") + 1], "/dev/null")
        self.assertEqual(len(out), len(cc1))

    def test_without_a_cc1plus_line_the_driver_is_used(self):
        mode, command = report.rewrite(["g++", "-c", "f.cpp", "-o", "f.o"], "nothing here")
        self.assertEqual(mode, "driver")
        self.assertEqual(command, ["g++", "f.cpp", "-c", "-o", "/dev/null"])

    def test_with_a_cc1plus_line_the_mode_says_so(self):
        mode, _ = report.rewrite([], text("driver_dry_run.txt"))
        self.assertEqual(mode, "cc1plus")

    def test_time_prefix_per_system(self):
        self.assertEqual(report.time_prefix("Darwin"), ["/usr/bin/time", "-l"])
        self.assertEqual(report.time_prefix("Linux"), ["/usr/bin/time", "-v"])


class Selecting(unittest.TestCase):
    ENTRIES = [{"directory": "/b", "file": "/s/a.cpp", "command": "g++ -o t/a.cpp.o -c /s/a.cpp", "output": "t/a.cpp.o"},
               {"directory": "/b", "file": "/s/b.cpp", "command": "g++ -o t/b.cpp.o -c /s/b.cpp"}]

    def test_by_source_path(self):
        self.assertEqual(report.select(self.ENTRIES, "/s/a.cpp")["file"], "/s/a.cpp")

    def test_by_object_name_taken_from_the_output_key_or_the_command(self):
        self.assertEqual(report.select(self.ENTRIES, "t/a.cpp.o")["file"], "/s/a.cpp")
        self.assertEqual(report.select(self.ENTRIES, "t/b.cpp.o")["file"], "/s/b.cpp")

    def test_unknown_name_is_an_error(self):
        with self.assertRaises(report.ReportError):
            report.select(self.ENTRIES, "nothing.cpp")

    def test_first_error_line(self):
        self.assertEqual(report.first_error("ok\nf.cpp:1:2: error: boom\nmore error"), "f.cpp:1:2: error: boom")
        self.assertIsNone(report.first_error("fine"))


class Table(unittest.TestCase):
    def test_sorted_by_constexpr_descending(self):
        rows = [{"file": "small", "constexpr_s": 1.0}, {"file": "big", "constexpr_s": 9.0}]
        lines = report.table(rows).splitlines()
        self.assertTrue(lines[1].startswith("big"))
        self.assertTrue(lines[2].startswith("small"))


@unittest.skipUnless((BUILD / "compile_commands.json").exists(), "build/host-v96 is not configured")
class OnTheRealBuild(unittest.TestCase):
    def test_smoke_has_a_row_with_times(self):
        run = subprocess.run([sys.executable, str(TOOLS / "compile_report.py"), str(BUILD), str(ROOT / "tests" / "smoke.cpp")],
                             capture_output=True, text=True, check=False)
        self.assertEqual(run.returncode, 0, run.stderr)
        row = json.loads(run.stdout)
        self.assertEqual(row["status"], 0)
        self.assertGreater(row["constexpr_s"], 0)
        self.assertGreater(row["peak_rss_mb"], 0)


if __name__ == "__main__":
    unittest.main()
