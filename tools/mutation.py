#!/usr/bin/env python3
"""Mutation testing for one header, with universalmutator and GCC.

Mull needs LLVM, and Clang cannot compile this code, so the mutants are made at
source level and each one is built and tested with the normal preset.

The run happens in a copy of the working tree under build/mutation/tree, with its
own build directory. The header you are working on is never touched, so you can
keep working (and building) while a run is in progress.

For every mutant of the header:
  survived   the tests still build and pass: a test is missing
  killed     a test fails, or a static_assert / contract / compile-fail test catches it
  invalid    it does not compile for another reason; not counted

Usage:
  tools/mutation.py include/lv/json.hpp --tests '^json$|fuzz_corpus_json' \
      --targets 'json fuzz_json_replay'
  tools/mutation.py include/lv/json.hpp --tests json --since origin/main   # changed lines only
  tools/mutation.py include/lv/json.hpp --tests json --sample 200          # a random 200

  --cmake-args passes extra arguments to the configure step of the copy, for
  example -DFETCHCONTENT_SOURCE_DIR_LVGL=... to avoid downloading LVGL again.

Needs: build/_tools/venv with universalmutator (python3 -m venv build/_tools/venv &&
build/_tools/venv/bin/pip install universalmutator).
"""
import argparse
import pathlib
import random
import re
import shutil
import subprocess
import sys

MUTATE = str(pathlib.Path("build/_tools/venv/bin/mutate").resolve())
TREE = pathlib.Path("build/mutation/tree")
# A compile error with one of these is a test doing its job, not a broken mutant.
CAUGHT_AT_COMPILE_TIME = re.compile(
    r"static assertion failed|non-constant condition for static assertion|"
    r"contract predicate is false|uncaught exception of type|"
    # Code that runs in constant evaluation is tested by compiling: a mutant that
    # reads out of bounds, overflows or never ends there is caught by the compiler.
    r"is not a constant expression|in 'constexpr' expansion of|"
    r"outside the bounds of|overflow in constant expression|"
    r"'constexpr' loop iteration count exceeds limit|use of deleted function")
# ...unless the mutant does not even parse.
NOT_CPP = re.compile(r"error: expected |error: stray |error: missing terminating|"
                     r"was not declared in this scope|error: template argument .* is invalid")


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


def run(command, cwd=TREE):
    r = subprocess.run(command, shell=True, capture_output=True, text=True, cwd=cwd)
    return r.returncode, r.stdout + r.stderr


def make_copy(preset, cmake_args):
    """Copies the working tree (without build/ and .git) and configures it."""
    TREE.mkdir(parents=True, exist_ok=True)
    subprocess.run(["rsync", "-a", "--delete", "--exclude", "/build", "--exclude", "/.git",
                    "./", str(TREE) + "/"], check=True)
    code, output = run(f"cmake --preset {preset} {cmake_args}")
    if code != 0:
        sys.exit("Configuring the copy failed:\n" + output[-2000:])


def judge(preset, tests, targets):
    """Builds and runs the chosen tests against the header as it is now."""
    code, output = run(f"cmake --build --preset {preset}" + (f" --target {targets}" if targets else ""))
    if code != 0:
        caught = CAUGHT_AT_COMPILE_TIME.search(output) and not NOT_CPP.search(output)
        return "killed" if caught else "invalid"
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
    parser.add_argument("--targets", help="build only these targets (space separated); "
                        "use it when few tests include the header, to save a full rebuild")
    parser.add_argument("--since", help="only mutate lines changed since this git ref")
    parser.add_argument("--lines", help="only mutate these lines, e.g. 60-120,200")
    parser.add_argument("--sample", type=int, help="judge a random sample of this many mutants")
    parser.add_argument("--seed", type=int, default=1, help="seed of the sample")
    parser.add_argument("--cmake-args", default="", help="extra arguments for configuring the copy")
    args = parser.parse_args()

    header = TREE / args.header
    lines = None
    if args.since:
        lines = changed_lines(args.header, args.since)
    if args.lines:
        wanted = parse_ranges(args.lines)
        lines = wanted if lines is None else lines & wanted
    make_copy(args.preset, args.cmake_args)
    original = header.read_text()

    if judge(args.preset, args.tests, args.targets) != "survived":
        sys.exit("The tests do not pass on the unchanged header; fix that first.")

    mutants = make_mutants(args.header, lines, pathlib.Path("build/mutation/mutants") / header.stem)
    total = len(mutants)
    if args.sample and args.sample < total:
        mutants = sorted(random.Random(args.seed).sample(mutants, args.sample))
    print(f"{len(mutants)} of {total} mutants of {args.header}", flush=True)
    counts = {"killed": 0, "survived": 0, "invalid": 0}
    survivors = []
    try:
        for index, mutant in enumerate(mutants, start=1):
            header.write_text(mutant.read_text())
            verdict = judge(args.preset, args.tests, args.targets)
            counts[verdict] += 1
            if verdict == "survived":
                survivors.append(describe(original, mutant))
            print(f"[{index}/{len(mutants)}] {verdict:8} {describe(original, mutant)}", flush=True)
    finally:
        header.write_text(original)  # always put the real header back

    scope = f"a sample of {len(mutants)} of {total} mutants" if len(mutants) < total else "all mutants"
    print(f"\nMutation: universalmutator, {args.header}, {scope}: {counts['killed']} killed, "
          f"{counts['survived']} survived, {counts['invalid']} invalid (not counted)")
    for s in survivors:
        print("  survived:", s)
    return 1 if survivors else 0


if __name__ == "__main__":
    sys.exit(main())
