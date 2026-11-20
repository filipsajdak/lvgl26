#!/usr/bin/env python3
"""Plans the nightly mutation run: one shard for each header of include/lv that changed.

    python3 tools/mutation_plan.py --since <git ref> [--github-output FILE]

Prints the plan as JSON: {"since": ..., "any": bool, "matrix": {"include": [shard, ...]}}.
A shard names the header, the ctest expression of its tests and the targets to build, from
tools/mutation_tests.json (a header that is not listed there gets the default, and its shard
says so: "fallback"). A header may also say "logic": true (only mutants that swap an operator or a
number) and "parts": n (n shards that each judge every n-th mutant): `logic` and `part` of the shard. With
--github-output the same values are written as `any=`, `since=` and `matrix=` lines.
"""

import argparse
import json
import pathlib
import re
import subprocess
import sys

TOOLS = pathlib.Path(__file__).resolve().parent
ROOT = TOOLS.parent
CONFIG = TOOLS / "mutation_tests.json"
HEADER_DIR = "include/lv"
HEADER_SUFFIX = ".hpp"
# The name of a header goes into the matrix of a workflow. It holds letters, digits and
# the signs of a path only, so that no part of it can be read as a command.
HEADER_NAME = re.compile(r"include/lv/[A-Za-z0-9_/.-]+\.hpp")


def changed_headers(since, root=ROOT):
    """The headers under include/lv that differ from `since` and still exist.

    A header with a name outside HEADER_NAME ends the plan: it is not skipped silently.
    """
    out = subprocess.run(["git", "diff", "--name-only", since, "HEAD", "--", HEADER_DIR],
                         capture_output=True, text=True, check=True, cwd=root).stdout
    names = sorted(n for n in out.splitlines() if n.endswith(HEADER_SUFFIX))
    for name in names:
        if not HEADER_NAME.fullmatch(name):
            raise SystemExit(f"mutation_plan: a header has a name the plan does not accept: {name!r}")
    return [n for n in names if (pathlib.Path(root) / n).exists()]


def shard_name(header):
    """A short name for the matrix: include/lv/detail/x.hpp is detail-x."""
    return header.removeprefix(HEADER_DIR + "/").removesuffix(HEADER_SUFFIX).replace("/", "-")


def shard_for(header, config):
    """The shards of a header: one, or `parts` of them that each judge every n-th mutant."""
    chosen = config["headers"].get(header, config["default"])
    parts = chosen.get("parts", 1)
    return [{"name": shard_name(header) + (f"-{part}of{parts}" if parts > 1 else ""), "header": header,
             "tests": chosen["tests"], "targets": chosen.get("targets") or "",
             "fallback": header not in config["headers"], "logic": bool(chosen.get("logic", False)),
             "part": f"{part}/{parts}" if parts > 1 else ""} for part in range(1, parts + 1)]


def plan(since, config, root=ROOT):
    shards = [shard for h in changed_headers(since, root) for shard in shard_for(h, config)]
    return {"since": since, "any": bool(shards), "matrix": {"include": shards}}


def write_github_output(path, result):
    with open(path, "a") as out:
        out.write(f"any={str(result['any']).lower()}\n")
        out.write(f"since={result['since']}\n")
        out.write(f"matrix={json.dumps(result['matrix'], separators=(',', ':'))}\n")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--since", required=True, help="the git ref the last good run ended at")
    parser.add_argument("--github-output", help="file named by $GITHUB_OUTPUT")
    args = parser.parse_args(argv)
    result = plan(args.since, json.loads(CONFIG.read_text()))
    print(json.dumps(result, indent=2))
    if args.github_output:
        write_github_output(args.github_output, result)
    return 0


if __name__ == "__main__":
    sys.exit(main())
