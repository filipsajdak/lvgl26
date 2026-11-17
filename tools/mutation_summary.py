#!/usr/bin/env python3
"""The job summary of one nightly mutation shard, from the JSON that tools/mutation.py wrote.

    python3 tools/mutation_summary.py mutation-report.json [--fallback] >> "$GITHUB_STEP_SUMMARY"
"""

import json
import pathlib
import sys

HOW_TO_TRIAGE = ("Triage each survivor: write a test that kills it, or add it to "
                 "tools/mutation_accepted.json with the reason it is equivalent (the program "
                 "means the same) or arid (nothing observable depends on it).")


FALLBACK = ("{header} has no entry in tools/mutation_tests.json, so every test judged it and "
            "everything was built for each mutant. Add an entry with the tests and targets that cover it.")


def summary(row, fallback=False):
    """The report as Markdown: the counts, whether the shard covered every mutant, the
    survivors, and a note when the header fell back to the default tests."""
    lines = [f"### Mutation: {row['header']}", "",
             f"{row['killed']} killed, {row['survived']} survived, {row['accepted']} accepted, "
             f"{row['invalid']} invalid (not counted), of {row['mutants']} mutants "
             f"(since {row['since']})."]
    if not row["complete"]:
        lines += ["", f"The time budget ended the run: {row['judged']} of {row['mutants']} mutants "
                  "were judged; the rest were not reached."]
    if fallback:
        lines += ["", FALLBACK.format(header=row["header"])]
    if row["survivors"]:
        lines += ["", "```", *row["survivors"], "```", "", HOW_TO_TRIAGE]
    return "\n".join(lines) + "\n"


def main(argv=None):
    args = argv or sys.argv
    path = pathlib.Path(args[1])
    sys.stdout.write(summary(json.loads(path.read_text()), fallback="--fallback" in args[2:]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
