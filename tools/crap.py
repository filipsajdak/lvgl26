#!/usr/bin/env python3
"""CRAP report for include/lv.

CRAP = CC^2 * (1 - coverage)^3 + CC, per function.

  CC        cyclomatic complexity, measured by lizard
  coverage  share of the function's instrumented lines that ran, from gcov

Rules (see CLAUDE.md):
  - every function has CC <= 8, compile-time or not;
  - a function that runs at run time has CRAP <= 8;
  - a consteval function has no run-time coverage; its tests are static_asserts and
    compile-fail tests, so only its CC is checked here.

Usage:
  cmake --preset host-v96-coverage && cmake --build --preset host-v96-coverage
  ctest --preset host-v96-coverage
  tools/crap.py build/host-v96-coverage
"""
import collections
import json
import pathlib
import subprocess
import sys

import lizard

LIMIT = 8


def line_counts(build_dir, gcov="gcov-16", root="include/lv"):
    """{file: {line: times run}}, summed over every test and every template instantiation.

    gcov reports a line of a function template once per instantiation. The counts are
    added up here; gcovr 8.6 does not add them and reports such lines as never run.
    """
    counts = collections.defaultdict(lambda: collections.defaultdict(int))
    here = pathlib.Path.cwd()
    for gcda in sorted(pathlib.Path(build_dir).rglob("*.gcda")):
        out = subprocess.run([gcov, "--json-format", "--stdout", gcda.name], cwd=gcda.parent,
                             capture_output=True, text=True, check=True).stdout
        for line in out.splitlines():  # one JSON document per line
            for f in json.loads(line)["files"]:
                path = pathlib.Path(f["file"])
                path = path if path.is_absolute() else (gcda.parent / path)
                try:
                    name = str(path.resolve().relative_to(here))
                except ValueError:
                    continue
                if not name.startswith(root):
                    continue
                for l in f["lines"]:
                    counts[name][l["line_number"]] += l["count"]
    return counts


def is_consteval(source_lines, start):
    """Whether the function that starts at `start` is declared consteval."""
    head = " ".join(source_lines[max(0, start - 4):start])
    return "consteval" in head


def coverage_of(lines, start, end):
    """Share of instrumented lines in [start, end] that ran, or None if none is instrumented."""
    seen = [n for line, n in lines.items() if start <= line <= end]
    return sum(1 for n in seen if n > 0) / len(seen) if seen else None


def crap(cc, coverage):
    return cc * cc * (1 - coverage) ** 3 + cc


def main(build_dir, root="include/lv"):
    counts = line_counts(build_dir, root=root)
    rows, failures = [], 0
    for path in sorted(pathlib.Path(root).rglob("*.hpp")):
        source = path.read_text().splitlines()
        lines = counts.get(str(path), {})
        for fn in lizard.analyze_file(str(path)).function_list:
            cc = fn.cyclomatic_complexity
            cov = coverage_of(lines, fn.start_line, fn.end_line)
            if is_consteval(source, fn.start_line):
                kind, score = "compile time", None
            elif cov is None:
                kind, score, cov = "never ran", crap(cc, 0.0), 0.0
            else:
                kind, score = "run time", crap(cc, cov)
            bad = cc > LIMIT or (score is not None and score > LIMIT)
            failures += bad
            rows.append((bad, cc, score, cov, kind, f"{path}:{fn.start_line}", fn.name))

    print(f"{'CC':>3} {'CRAP':>6} {'cov':>5}  {'kind':<12} function")
    for bad, cc, score, cov, kind, where, name in sorted(rows, key=lambda r: (-r[0], -r[1])):
        if not bad:
            continue
        score_text = f"{score:6.1f}" if score is not None else "     -"
        cov_text = f"{cov * 100:4.0f}%" if cov is not None else "    -"
        print(f"{cc:>3} {score_text} {cov_text}  {kind:<12} {name}  ({where})")
    print(f"\n{len(rows)} functions, {failures} over the limit of {LIMIT}.")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main(*sys.argv[1:]))
