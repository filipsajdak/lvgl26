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

The nightly run (.github/workflows/nightly.yml) mutates the lines changed since its last
successful run, one header at a time (tools/mutation_plan.py), with
  --time-budget SECONDS   stop after this long and say how many mutants were judged
  --report FILE           write the counts and the survivors as JSON
  --accepted FILE         survivors that are triaged (tools/mutation_accepted.json)

Needs: build/_tools/venv with universalmutator (python3 -m venv build/_tools/venv &&
build/_tools/venv/bin/pip install universalmutator).
"""
import argparse
import json
import os
import pathlib
import random
import re
import shutil
import subprocess
import sys
import time

VENV_MUTATE = pathlib.Path("build/_tools/venv/bin/mutate")
# The CI image has universalmutator on the path; a local checkout has it in a venv.
MUTATE = str(VENV_MUTATE.resolve()) if VENV_MUTATE.exists() else (shutil.which("mutate") or str(VENV_MUTATE.resolve()))
TREE = pathlib.Path("build/mutation/tree")
# The lines of output a failed baseline shows: the errors are at the end of a build or a test run.
BASELINE_TAIL_LINES = 60
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


TOKEN = re.compile(r"[A-Za-z_]\w*|\d[\w.']*|<=>|<<=|>>=|[-+*/%&|^<>=!]=|&&|\|\||\+\+|--|<<|>>|->|::|\S")
OPERATORS = set("+ - * / % < > <= >= == != && || ++ -- += -= *= /= & | ^ << >>".split())
NOT_LOGIC = re.compile(r"^\s*(//|template\b|using\b|requires\b|#|concept\b|namespace\b)|\bstatic_assert\b")


def is_logic_mutant(before, after):
    """True if the mutant swaps one operator for another, or one number for another,
    on a line of statements. Text-level mutants of declarations, template argument
    lists and comments rarely compile, and say nothing about the tests when they do."""
    if NOT_LOGIC.search(before):
        return False
    a, b = list(TOKEN.finditer(before)), TOKEN.findall(after)
    if len(a) != len(b):
        return False
    changed = [(x, y) for x, y in zip(a, b) if x.group() != y]
    if len(changed) != 1:
        return False
    x, y = changed[0]
    if x.group()[0].isdigit():
        return y[0].isdigit()
    return x.group() in OPERATORS and y in OPERATORS and is_spaced(before, x)


def is_spaced(line, token):
    """The code is formatted, so a binary operator has a space on each side; a `*`,
    `&`, `<` or `>` without them is a declarator or a template bracket."""
    if token.group() in ("++", "--"):
        return True
    return line[token.start() - 1:token.start()] == " " and line[token.end():token.end() + 1] == " "


def changed_line(original_lines, mutant_text):
    """The (before, after) pair of the one line a mutant changes, or None."""
    mutant_lines = mutant_text.splitlines()
    if len(mutant_lines) != len(original_lines):
        return None
    pairs = [(x, y) for x, y in zip(original_lines, mutant_lines) if x != y]
    return pairs[0] if len(pairs) == 1 else None


def without_comment_mutants(mutants, original):
    """The mutants that change code. A mutant of a comment line changes nothing the compiler reads."""
    original_lines = original.splitlines()
    kept = []
    for m in mutants:
        pair = changed_line(original_lines, m.read_text())
        if not (pair and pair[0].lstrip().startswith("//")):
            kept.append(m)
    return kept


def logic_only(mutants, original):
    original_lines = original.splitlines()
    kept = []
    for m in mutants:
        pair = changed_line(original_lines, m.read_text())
        if pair and is_logic_mutant(*pair):
            kept.append(m)
    return kept


# The compile-fail tests match the words of GCC's messages, written with plain quotes. Python
# started in the C locale switches its own to C.UTF-8 and hands that on, and GCC then writes
# the quotes as curly ones: a regex with 'name' finds nothing. Plain ctest keeps the locale.
CHILD_ENV = {**os.environ, "LC_ALL": "C"}


def run(command, cwd=TREE):
    r = subprocess.run(command, shell=True, capture_output=True, text=True, cwd=cwd, env=CHILD_ENV)
    return r.returncode, r.stdout + r.stderr


def track_files(tree):
    """Makes the copy a git repository of its own, so that a test that asks git for the tracked
    files (tools/check_tokens.py) finds the same files as in the working tree."""
    for command in (["git", "init", "-q"], ["git", "add", "-A"]):
        subprocess.run(command, cwd=tree, check=True, capture_output=True)


def make_copy(preset, cmake_args):
    """Copies the working tree (without build/ and .git), makes it a repository and configures it."""
    TREE.mkdir(parents=True, exist_ok=True)
    subprocess.run(["rsync", "-a", "--delete", "--exclude", "/build", "--exclude", "/.git",
                    "./", str(TREE) + "/"], check=True)
    track_files(TREE)
    code, output = run(f"cmake --preset {preset} {cmake_args}")
    if code != 0:
        sys.exit("Configuring the copy failed:\n" + output[-2000:])


def judge(preset, tests, targets, limit):
    """Builds and runs the chosen tests against the header as it is now."""
    return judge_with_output(preset, tests, targets, limit)[0]


def judge_with_output(preset, tests, targets, limit):
    """The verdict of judge, and the output of the step that decided it."""
    code, output = run(f"cmake --build --preset {preset}" + (f" --target {targets}" if targets else ""))
    if code != 0:
        # The first error says what is wrong; later ones often only follow from it.
        first = next((line for line in output.splitlines() if "error:" in line), "")
        caught = CAUGHT_AT_COMPILE_TIME.search(output) and not NOT_CPP.search(first)
        return ("killed" if caught else "invalid"), output
    # A mutant can turn a loop into an endless one. The limit ends such a test, and
    # a test that is ended has failed: the mutant is killed.
    code, output = run(f"ctest --preset {preset} -R '{tests}' --timeout {limit}")
    return ("survived" if code == 0 else "killed"), output


def baseline_failure(verdict, output):
    """What to say when the tests do not pass on the unchanged header: the verdict
    (a build that fails is 'invalid', a test that fails is 'killed') and the end of the
    output that decided it, because a shard of the nightly run has nothing else to show."""
    tail = "\n".join(output.strip().splitlines()[-BASELINE_TAIL_LINES:])
    return ("The tests do not pass on the unchanged header; fix that first.\n"
            f"The build or the tests said ({verdict}):\n{tail}")


def describe(header_text, mutant):
    """The line a mutant changed, as 'line N: before -> after'."""
    before, after = header_text.splitlines(), mutant.read_text().splitlines()
    for number, (a, b) in enumerate(zip(before, after), start=1):
        if a != b:
            return f"line {number}: {a.strip()}  ->  {b.strip()}"
    return "(a line was removed or added)"


def parse_args(argv=None):
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
    parser.add_argument("--logic", action="store_true",
                        help="only mutants that swap one operator or one number on a line of "
                             "statements; use it for template and reflection headers")
    parser.add_argument("--test-timeout", type=int, default=60,
                        help="seconds a test may run against a mutant before it counts as failed")
    parser.add_argument("--cmake-args", default="", help="extra arguments for configuring the copy")
    parser.add_argument("--time-budget", type=int, help="seconds after which the run stops and "
                        "reports how many mutants it judged")
    parser.add_argument("--report", help="write the result as JSON to this file")
    parser.add_argument("--accepted", help="JSON file of triaged survivors (see accepted_for)")
    return parser.parse_args(argv)


def selected_lines(args):
    """The lines to mutate: those changed since --since, narrowed by --lines; None is all."""
    lines = changed_lines(args.header, args.since) if args.since else None
    if args.lines:
        wanted = parse_ranges(args.lines)
        lines = wanted if lines is None else lines & wanted
    return lines


def mutant_text(description):
    """A mutant without its line number, so that a triage survives an edit above it."""
    return description.split(": ", 1)[1] if description.startswith("line ") else description


def accepted_for(path, header):
    """{mutant text: reason} of the triaged survivors of `header`, from a JSON file of
    [{"header": ..., "mutant": "before  ->  after", "reason": ...}]."""
    if not path or not pathlib.Path(path).exists():
        return {}
    rows = json.loads(pathlib.Path(path).read_text())
    return {row["mutant"]: row["reason"] for row in rows if row["header"] == header}


def judge_all(args, original, mutants, accepted):
    """Judges the mutants one by one, until the time budget is spent.
    Returns the counts, the survivors that need a triage, and the accepted ones."""
    header = TREE / args.header
    deadline = time.monotonic() + args.time_budget if args.time_budget else None
    counts = {"killed": 0, "survived": 0, "invalid": 0}
    survivors, triaged = [], []
    try:
        for index, mutant in enumerate(mutants, start=1):
            if deadline is not None and time.monotonic() > deadline:
                break
            header.write_text(mutant.read_text())
            verdict = judge(args.preset, args.tests, args.targets, args.test_timeout)
            what = describe(original, mutant)
            counts[verdict] += 1
            if verdict == "survived":
                (triaged if mutant_text(what) in accepted else survivors).append(what)
            print(f"[{index}/{len(mutants)}] {verdict:8} {what}", flush=True)
    finally:
        header.write_text(original)  # always put the real header back
    return counts, survivors, triaged


def result_row(args, counts, survivors, triaged, judged, total):
    return {"header": args.header, "since": args.since, "mutants": total, "judged": judged,
            "complete": judged == total, "killed": counts["killed"],
            "survived": counts["survived"] - len(triaged), "accepted": len(triaged),
            "invalid": counts["invalid"], "survivors": survivors}


def choose_mutants(args, lines, original):
    """The mutants to judge: all of the chosen lines, the logic ones, or a sample."""
    mutants = make_mutants(args.header, lines, pathlib.Path("build/mutation/mutants") / (TREE / args.header).stem)
    mutants = without_comment_mutants(mutants, original)
    if args.logic:
        mutants = logic_only(mutants, original)
    total = len(mutants)
    if args.sample and args.sample < total:
        mutants = sorted(random.Random(args.seed).sample(mutants, args.sample))
    print(f"{len(mutants)} of {total} mutants of {args.header}", flush=True)
    return mutants


def print_result(args, row):
    """The result line, the survivors, and the JSON report if one was asked for."""
    if args.report:
        pathlib.Path(args.report).write_text(json.dumps(row, indent=2) + "\n")
    scope = "all mutants" if row["complete"] else f"{row['judged']} of {row['mutants']} mutants"
    print(f"\nMutation: universalmutator, {args.header}, {scope}: {row['killed']} killed, "
          f"{row['survived']} survived, {row['accepted']} accepted, {row['invalid']} invalid (not counted)")
    for s in row["survivors"]:
        print("  survived:", s)


def exit_code(row):
    """1 for a survivor that needs a triage, 2 when the time budget left mutants unjudged
    (the nightly run must not move its base past lines it did not cover), else 0."""
    if row["survivors"]:
        return 1
    return 0 if row["complete"] else 2


def main():
    args = parse_args()
    header = TREE / args.header
    lines = selected_lines(args)
    make_copy(args.preset, args.cmake_args)
    original = header.read_text()
    # The copy keeps the working tree's file times, so after an earlier run the header
    # looks older than the objects built from that run's last mutant. Writing it again
    # makes the build system compile the real header for the baseline.
    header.write_text(original)

    verdict, output = judge_with_output(args.preset, args.tests, args.targets, args.test_timeout)
    if verdict != "survived":
        sys.exit(baseline_failure(verdict, output))

    mutants = choose_mutants(args, lines, original)
    counts, survivors, triaged = judge_all(args, original, mutants, accepted_for(args.accepted, args.header))
    row = result_row(args, counts, survivors, triaged, sum(counts.values()), len(mutants))
    print_result(args, row)
    return exit_code(row)


if __name__ == "__main__":
    sys.exit(main())
