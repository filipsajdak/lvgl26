#!/usr/bin/env python3
"""One row of compile cost for one probe of tools/metrics/probes.json, as JSON.

    python3 tools/compile_cost.py compile build/host-v96 smoke            # needs valgrind
    python3 tools/compile_cost.py run build/host-v96 visibility_hidden    # needs valgrind and the target visibility_probe
    python3 tools/compile_cost.py stack path/to/set_probe.cpp.su set_stack

The headline number is Valgrind's total instruction count (callgrind) of one cc1plus: the
hosted runners have no hardware counter, and this count is the same on every run
(docs/research/compile-time.md, "The nightly measure"). What GCC reports about itself explains a
change: the allocated total of -fmem-report and the element counts of -fstats. Wall time is only
a coarse check. Peak memory is not recorded: it varies by a factor of two between runs.
Uses the standard library and tools/compile_report.py.
"""

import json
import pathlib
import re
import subprocess
import sys
import time

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import compile_report as cr  # noqa: E402
import stack_budget  # noqa: E402

PROBES_FILE = pathlib.Path(__file__).resolve().parent / "metrics" / "probes.json"
CALLGRIND = ["valgrind", "--tool=callgrind", "--callgrind-out-file=/dev/null"]
# The element counts of -fstats that move when templates or specializations are made.
STATS_KEYS = ("decl_specializations", "type_specializations", "Type hash")
# Simulated seconds of a run-time probe: its cost a second is the difference with a run of none.
RUN_SECONDS = 2
UNITS = {"": 1, "k": 1024, "M": 1024**2, "G": 1024**3}


class CostError(Exception):
    """The probe cannot be measured; the message says why."""


def load_probes(path=PROBES_FILE):
    return {p["id"]: p for p in json.loads(pathlib.Path(path).read_text())["probes"]}


def callgrind_total(stderr):
    """The instructions of the program, out of the 'Collected : N' line of callgrind."""
    found = re.search(r"Collected\s*:\s*(\d+)", stderr)
    if not found:
        raise CostError("callgrind printed no total: " + stderr[-200:])
    return int(found.group(1))


def allocated_total(report):
    """Bytes of the first 'Total' row of -fmem-report, whose unit is a letter after the number."""
    found = re.search(r"^Total\s+(\d+)([kMG]?)\b", report, re.M)
    if not found:
        raise CostError("-fmem-report has no Total row")
    return int(found.group(1)) * UNITS[found.group(2)]


def stats_counts(report):
    """The element counts of -fstats: {'decl_specializations': 6738, ...}."""
    out = {}
    for line in report.splitlines():
        name, _, rest = line.partition(":")
        found = re.search(r"(\d+) elements", rest)
        if found and name.strip() in STATS_KEYS:
            out[name.strip().lower().replace(" ", "_") + "_elements"] = int(found.group(1))
    return out


def command_of(build, probe):
    """(cc1plus command, directory) for the probe's file, found in the build's compile database."""
    entry = cr.select(cr.load_entries(build), probe["file"])
    mode, command = cr.prepared(entry)
    if mode != "cc1plus":
        raise CostError(f"{probe['file']}: the driver did not print its cc1plus command")
    return command, entry["directory"]


def run(command, directory):
    return subprocess.run(command, capture_output=True, text=True, cwd=directory, check=False)


def native_part(command, directory):
    """Wall time, -fmem-report and -fstats, from two runs of the compiler itself."""
    start = time.perf_counter()
    mem = run(command + ["-fmem-report"], directory)
    wall = time.perf_counter() - start
    return {"wall_s": round(wall, 2), "mem_allocated": allocated_total(mem.stderr),
            **stats_counts(run(command + ["-fstats"], directory).stderr)}


def compile_row(build, probe):
    command, directory = command_of(build, probe)
    row = {"probe": probe["id"], "kind": "compile", **native_part(command, directory)}
    start = time.perf_counter()
    counted = run(CALLGRIND + command, directory)
    row["valgrind_wall_s"] = round(time.perf_counter() - start, 1)
    row["ir"] = callgrind_total(counted.stderr)
    if probe.get("ops"):
        found = cr.ops_range(command, directory)
        row["ops_between"] = list(found) if found else None
    return row


def run_row(build, probe):
    """Instructions a simulated second of tools/probes/visibility.cpp costs, from two runs under callgrind."""
    program = str(pathlib.Path(build, "tests", "visibility_probe"))

    def total(seconds):
        return callgrind_total(run(CALLGRIND + [program, probe["scenario"], str(seconds)], ".").stderr)

    return {"probe": probe["id"], "kind": "run", "value": (total(RUN_SECONDS) - total(0)) // RUN_SECONDS}


def stack_row(report_text, probe):
    """The stack of the probe's function in a -fstack-usage report."""
    for name, size in stack_budget.frames(report_text).items():
        if f" {probe['function']}(" in name:
            return {"probe": probe["id"], "kind": "stack", "value": size}
    raise CostError(f"the report names no {probe['function']}")


ROW_OF = {"compile": compile_row, "run": run_row,
          "stack": lambda where, probe: stack_row(pathlib.Path(where).read_text(), probe)}


def main(argv):
    if len(argv) != 3 or argv[0] not in ("compile", "run", "stack"):
        print(__doc__.strip().splitlines()[0], file=sys.stderr)
        return 2
    mode, where, name = argv
    probe = load_probes()[name]
    try:
        row = ROW_OF[mode](where, probe)
    except CostError as error:
        print(f"compile_cost: {error}", file=sys.stderr)
        return 1
    print(json.dumps(row))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
