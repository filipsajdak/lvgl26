#!/usr/bin/env python3
"""One JSON row of compile cost per file: python3 tools/compile_report.py build/host-v96 tests/smoke.cpp

Runs the real cc1plus (found with `g++ -###`), because the driver's own instruction
count is wrong for this. Times come from the SARIF time report of GCC 16
(-ftime-report with -fdiagnostics-add-output=sarif:file=...), peak memory and
instructions retired from /usr/bin/time (setuid-free fallback: wait4 resource usage, no instructions;
LVGL26_NO_TIME=1 forces it). See docs/research/compile-time.md.
"""

import argparse
import concurrent.futures
import json
import os
import pathlib
import re
import shlex
import subprocess
import sys
import tempfile
import time

OPS_START = 1_000_000
OPS_FACTOR = 4
OPS_STEPS = 3
OPS_CEILING = 40_000_000_000
OPS_MESSAGE = "operation count"
BYTES_PER_MB = 1024 * 1024
KB_PER_MB = 1024
TIME_BINARY = "/usr/bin/time"
NO_TIME_VARIABLE = "LVGL26_NO_TIME"

DROP_WITH_VALUE = ("-o", "-MF", "-MT", "-MQ")
DROP_ALONE = ("-c", "-MD", "-MMD", "-MP")
OPS_LIMIT_OPTION = "-fconstexpr-ops-limit="

ROWS = {
    "constexpr_s": "constant expression evaluation",
    "template_instantiation_s": "template instantiation",
    "parsing_s": "phase parsing",
    "total_s": "TOTAL",
}


class ReportError(Exception):
    """The input is not usable; the message says what to fix."""


def without_ops_limit(tokens):
    return [t for t in tokens if not t.startswith(OPS_LIMIT_OPTION)]


def strip_options(tokens):
    """The compile command without its outputs and dependency files; the ops limit stays."""
    kept = []
    skip = False
    for token in tokens:
        if skip:
            skip = False
        elif token in DROP_WITH_VALUE:
            skip = True
        elif token not in DROP_ALONE:
            kept.append(token)
    return kept


def find_cc1(driver_output):
    """The cc1plus command line out of the stderr of `g++ -###`, or None."""
    for line in driver_output.splitlines():
        if re.search(r"/cc1plus\s", line):
            return shlex.split(line)
    return None


def with_output_to_null(cc1):
    """The cc1plus command with its assembler file thrown away."""
    out = list(cc1)
    if "-o" in out:
        out[out.index("-o") + 1] = os.devnull
    else:
        out += ["-o", os.devnull]
    return out


def rewrite(entry_tokens, driver_output):
    """(mode, command) for one file: cc1plus when -### printed it, the driver otherwise."""
    cc1 = find_cc1(driver_output)
    if cc1 is None:
        return "driver", strip_options(entry_tokens) + ["-c", "-o", os.devnull]
    return "cc1plus", with_output_to_null(cc1)


def parse_time(text):
    """Peak memory in MB and instructions retired, from the output of /usr/bin/time -l or -v."""
    rss = re.search(r"^\s*(\d+)\s+maximum resident set size", text, re.M)
    if rss:
        peak = int(rss.group(1)) / BYTES_PER_MB
    else:
        kb = re.search(r"Maximum resident set size \(kbytes\):\s*(\d+)", text)
        peak = int(kb.group(1)) / KB_PER_MB if kb else None
    instructions = re.search(r"^\s*(\d+)\s+instructions retired", text, re.M)
    return {"peak_rss_mb": None if peak is None else round(peak, 1),
            "instructions": int(instructions.group(1)) if instructions else None}


def parse_report(sarif_text, analyzer_on):
    """The timevar rows, in seconds, out of a SARIF file written with -ftime-report."""
    props = json.loads(sarif_text)["runs"][0]["invocations"][0].get("properties", {})
    rows = {row["name"]: row["elapsed"]["wall"] for row in props.get("gcc/timeReport", {}).get("timevars", [])}
    out = {key: round(rows.get(name, 0.0), 3) for key, name in ROWS.items()}
    analyzer = sum(v for k, v in rows.items() if k.lower().startswith("analyzer"))
    out["analyzer_s"] = round(analyzer, 3) if analyzer_on else None
    return out


def first_error(stderr):
    for line in stderr.splitlines():
        if "error" in line.lower() and "maximum resident" not in line:
            return line.strip()
    return None


def load_entries(build):
    path = pathlib.Path(build) / "compile_commands.json"
    if not path.is_file():
        raise ReportError(f"{path} does not exist: configure the build directory first")
    return json.loads(path.read_text())


def output_of(entry):
    if "output" in entry:
        return os.path.normpath(os.path.join(entry["directory"], entry["output"]))
    tokens = shlex.split(entry.get("command") or " ".join(entry["arguments"]))
    return os.path.normpath(os.path.join(entry["directory"], tokens[tokens.index("-o") + 1]))


def select(entries, name):
    """The compile command for a source file or an object name."""
    wanted = os.path.abspath(name)
    for entry in entries:
        if os.path.normpath(entry["file"]) == wanted or output_of(entry) == wanted:
            return entry
    for entry in entries:
        if output_of(entry).endswith("/" + name) or entry["file"].endswith("/" + name):
            return entry
    raise ReportError(f"{name} is not in compile_commands.json")


def tokens_of(entry):
    return shlex.split(entry["command"]) if "command" in entry else list(entry["arguments"])


def run_driver_dry(entry):
    tokens = strip_options(tokens_of(entry)) + ["-c", "-o", "x.o", "-###"]
    result = subprocess.run(tokens, capture_output=True, text=True, cwd=entry["directory"], check=False)
    return result.stderr


def prepared(entry):
    return rewrite(tokens_of(entry), run_driver_dry(entry))


def time_binary(system):
    """The /usr/bin/time option (-l or -v) when the binary can be used, else None."""
    if os.environ.get(NO_TIME_VARIABLE) or not os.access(TIME_BINARY, os.X_OK):
        return None
    return "-l" if system == "Darwin" else "-v"


def rusage_peak_mb(maxrss, system):
    """ru_maxrss is bytes on macOS and kilobytes on Linux."""
    return round(maxrss / (BYTES_PER_MB if system == "Darwin" else KB_PER_MB), 1)


def run_rusage(command, directory, system):
    """No /usr/bin/time: wait4 gives the resource use of this child alone, even with --jobs."""
    with tempfile.TemporaryFile("w+") as err:
        start = time.perf_counter()
        child = subprocess.Popen(command, cwd=directory, stdout=subprocess.DEVNULL, stderr=err)
        _, status, usage = os.wait4(child.pid, 0)
        wall = time.perf_counter() - start
        child.returncode = os.waitstatus_to_exitcode(status)
        err.seek(0)
        text = err.read()
    stats = {"timer": "rusage", "peak_rss_mb": rusage_peak_mb(usage.ru_maxrss, system), "instructions": None}
    return child.returncode, wall, text, stats


def run_time_binary(option, command, directory):
    start = time.perf_counter()
    result = subprocess.run([TIME_BINARY, option] + command, capture_output=True, text=True, cwd=directory, check=False)
    stats = {"timer": "time" + option, **parse_time(result.stderr)}
    return result.returncode, time.perf_counter() - start, result.stderr, stats


def run_timed(command, directory, extra):
    """Run once. Returns (status, wall seconds, stderr, {timer, peak_rss_mb, instructions})."""
    system = os.uname().sysname
    option = time_binary(system)
    if option is None:
        return run_rusage(command + extra, directory, system)
    return run_time_binary(option, command + extra, directory)


def measure(entry, name):
    mode, command = prepared(entry)
    analyzer_on = "-fanalyzer" in command
    with tempfile.TemporaryDirectory() as tmp:
        sarif = os.path.join(tmp, "report.sarif")
        extra = ["-ftime-report", f"-fdiagnostics-add-output=sarif:file={sarif}"]
        status, wall, stderr, stats = run_timed(command, entry["directory"], extra)
        row = {"file": name, "mode": mode, "wall_s": round(wall, 3), "status": status}
        row.update(stats)
        row["error"] = first_error(stderr) if status else None
        if os.path.exists(sarif):
            row.update(parse_report(pathlib.Path(sarif).read_text(), analyzer_on))
    return row


def exceeds(command, directory, limit):
    probe = without_ops_limit(command) + ["-fsyntax-only", f"{OPS_LIMIT_OPTION}{limit}"]
    result = subprocess.run(probe, capture_output=True, text=True, cwd=directory, check=False)
    return OPS_MESSAGE in result.stderr


def ops_range(command, directory):
    """(highest limit that fails, lowest that passes) after the growth steps and the bisection."""
    low, high = 0, OPS_START
    while exceeds(command, directory, high):
        low, high = high, high * OPS_FACTOR
        if high > OPS_CEILING:
            return None
    for _ in range(OPS_STEPS if low else 0):
        mid = (low + high) // 2
        low, high = (mid, high) if exceeds(command, directory, mid) else (low, mid)
    return low, high


def report(entry, name, with_ops):
    row = measure(entry, name)
    if with_ops and row["status"] == 0:
        mode, command = prepared(entry)
        found = ops_range(command, entry["directory"]) if mode == "cc1plus" else None
        row["ops_between"] = list(found) if found else None
    return row


TABLE_COLUMNS = ["file", "wall_s", "constexpr_s", "template_instantiation_s", "parsing_s", "analyzer_s",
                 "total_s", "peak_rss_mb", "instructions"]


def cell(row, column):
    return "-" if row.get(column) is None else str(row[column])


def table(rows):
    ordered = sorted(rows, key=lambda r: r.get("constexpr_s") or 0.0, reverse=True)
    cells = [TABLE_COLUMNS] + [[cell(r, c) for c in TABLE_COLUMNS] for r in ordered]
    widths = [max(len(line[i]) for line in cells) for i in range(len(TABLE_COLUMNS))]
    return "\n".join("  ".join(c.ljust(w) for c, w in zip(line, widths)).rstrip() for line in cells)


def parse_args(argv):
    parser = argparse.ArgumentParser(description="Compile cost of single files, as JSON rows.")
    parser.add_argument("build", help="build directory with compile_commands.json")
    parser.add_argument("files", nargs="+", help="source files or ninja object names")
    parser.add_argument("--table", action="store_true", help="print a table sorted by constexpr_s")
    parser.add_argument("--ops", action="store_true", help="count constant-evaluation operations by bisection (slow)")
    parser.add_argument("--jobs", type=int, default=1, help="files at once")
    return parser.parse_args(argv)


def main(argv):
    args = parse_args(argv)
    try:
        entries = load_entries(args.build)
        chosen = [(select(entries, name), name) for name in args.files]
    except ReportError as error:
        print(error, file=sys.stderr)
        return 2
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        rows = list(pool.map(lambda c: report(c[0], c[1], args.ops), chosen))
    if args.table:
        print(table(rows))
    else:
        for row in rows:
            print(json.dumps(row))
    return 1 if any(row["status"] for row in rows) else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
