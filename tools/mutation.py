#!/usr/bin/env python3
"""Mutation testing for one header, with universalmutator and GCC.

Mull needs LLVM, and Clang cannot compile this code, so the mutants are made at
source level and each one is built and tested with the normal preset.

For every mutant of the header:
  survived   the tests still build and pass: a test is missing
  killed     a test fails, or a static_assert / contract / compile-fail test catches it
  invalid    it does not compile for another reason; not counted

Usage:
  tools/mutation.py include/lv/json.hpp --tests 'json|runtime_ui|tokens'
  tools/mutation.py include/lv/json.hpp --tests json --since origin/main   # changed lines only
  tools/mutation.py include/lv/json.hpp --tests json --lines 60-120

Needs: build/_tools/venv with universalmutator (python3 -m venv build/_tools/venv &&
build/_tools/venv/bin/pip install universalmutator), and a configured preset.
"""
import argparse
import pathlib
import re
import shutil
import subprocess
import sys

MUTATE = "build/_tools/venv/bin/mutate"
# A compile error with one of these is a test doing its job, not a broken mutant.
CAUGHT_AT_COMPILE_TIME = re.compile(
    r"static assertion failed|non-constant condition for static assertion|"
    r"contract predicate is false|uncaught exception of type 'lv::detail::unknown_name'")


def changed_lines(header, since):
    """Line numbers of `header` that differ from the git ref `since`."""
    diff = subprocess.run(["git", "diff", "-U0", since, "--", header],
                          capture_output=True, text=True, check=True).stdout
    lines = set()
    for start, count in re.findall(r"^@@ -\S+ \+(\d+)(?:,(\d+))? @@", diff, re.M):
        lines.update(range(int(start), int(start) + int(count or 1)))
    return lines


def parse_ranges(text):
    lines = set()
    for part in text.split(","):
        first, _, last = part.partition("-")
        lines.update(range(int(first), int(last or first) + 1))
    return lines


def make_mutants(header, lines, out_dir):
    shutil.rmtree(out_dir, ignore_errors=True)
    out_dir.mkdir(parents=True)
    command = [MUTATE, header, "cpp", "--noCheck", "--mutantDir", str(out_dir)]
    if lines is not None:
        lines_file = out_dir / "lines.txt"
        lines_file.write_text("\n".join(str(n) for n in sorted(lines)) + "\n")
        command += ["--lines", str(lines_file)]
    subprocess.run(command, capture_output=True, text=True, check=True)
    return sorted(p for p in out_dir.iterdir() if p.suffix == ".hpp")


def run(command):
    r = subprocess.run(command, shell=True, capture_output=True, text=True)
    return r.returncode, r.stdout + r.stderr


def judge(preset, tests):
    """Builds and runs the chosen tests against the header as it is now."""
    code, output = run(f"cmake --build --preset {preset}")
    if code != 0:
        return "killed" if CAUGHT_AT_COMPILE_TIME.search(output) else "invalid"
    code, _ = run(f"ctest --preset {preset} -R '{tests}'")
    return "survived" if code == 0 else "killed"


def describe(header_text, mutant):
    """The line a mutant changed, as 'line N: before -> after'."""
    before, after = header_text.splitlines(), mutant.read_text().splitlines()
    for number, (a, b) in enumerate(zip(before, after), start=1):
        if a != b:
            return f"line {number}: {a.strip()}  ->  {b.strip()}"
    return "(a line was removed or added)"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("header")
    parser.add_argument("--tests", required=True, help="ctest -R expression of the tests to run")
    parser.add_argument("--preset", default="host-v96")
    parser.add_argument("--since", help="only mutate lines changed since this git ref")
    parser.add_argument("--lines", help="only mutate these lines, e.g. 60-120,200")
    parser.add_argument("--limit", type=int, help="stop after this many mutants")
    args = parser.parse_args()

    header = pathlib.Path(args.header)
    lines = None
    if args.since:
        lines = changed_lines(args.header, args.since)
    if args.lines:
        wanted = parse_ranges(args.lines)
        lines = wanted if lines is None else lines & wanted
    original = header.read_text()

    if judge(args.preset, args.tests) != "survived":
        sys.exit("The tests do not pass on the unchanged header; fix that first.")

    mutants = make_mutants(args.header, lines, pathlib.Path("build/mutants") / header.stem)
    if args.limit:
        mutants = mutants[:args.limit]
    counts = {"killed": 0, "survived": 0, "invalid": 0}
    survivors = []
    try:
        for index, mutant in enumerate(mutants, start=1):
            header.write_text(mutant.read_text())
            verdict = judge(args.preset, args.tests)
            counts[verdict] += 1
            if verdict == "survived":
                survivors.append(describe(original, mutant))
            print(f"[{index}/{len(mutants)}] {verdict:8} {describe(original, mutant)}", flush=True)
    finally:
        header.write_text(original)  # always put the real header back

    print(f"\nMutation: universalmutator, {args.header}: {counts['killed']} killed, "
          f"{counts['survived']} survived, {counts['invalid']} invalid (not counted)")
    for s in survivors:
        print("  survived:", s)
    return 1 if survivors else 0


if __name__ == "__main__":
    sys.exit(main())
