#!/usr/bin/env python3
"""The history of the nightly compile-cost run, and what it says about tonight's rows.

    python3 tools/cost_history.py judge --rows rows.json --history history.jsonl \
        --baseline baseline.json --commit SHA --date 2026-11-20 [--run URL]

`rows.json` is a list of rows of tools/compile_cost.py. The command appends the night to the history
and moves the baseline, both files in place, and prints the verdicts as JSON on standard output and a
Markdown summary on standard error. The exit status is 1 when a probe got dearer beyond its threshold.

The baseline of a probe is the value of the last night that was neither a regression nor a change
under the threshold: it moves when a probe is new or got cheaper beyond the threshold, and it stays
while a probe is within the threshold, so that a slow creep adds up until it crosses. A regression
must also stand against the median of the last nights, so that one odd baseline cannot raise it.
Uses the standard library only.
"""

import argparse
import json
import pathlib
import statistics
import sys

PROBES_FILE = pathlib.Path(__file__).resolve().parent / "metrics" / "probes.json"
# Nights in the rolling median. A week: a regression that lasts longer is the new normal anyway.
MEDIAN_NIGHTS = 7
# The regression must be at least this share of the threshold against the median, too.
MEDIAN_SHARE = 0.5
# Fewest nights before the median is used. Before that only the baseline judges.
MEDIAN_MINIMUM = 3


def value_of(row):
    return row["ir"] if row["kind"] == "compile" else row["value"]


def percent(now, before):
    return 100.0 * (now - before) / before if before else 0.0


def median_of(history, probe, nights=MEDIAN_NIGHTS):
    """The median of the probe over the last nights, or None when there are too few."""
    values = [value_of(r) for night in history[-nights:] for r in night["rows"] if r["probe"] == probe]
    return statistics.median(values) if len(values) >= MEDIAN_MINIMUM else None


def judge(value, baseline, median, threshold):
    """(status, change in percent against the baseline): new, same, regression or improvement."""
    if baseline is None:
        return "new", 0.0
    change = percent(value, baseline)
    if change > threshold and (median is None or percent(value, median) > threshold * MEDIAN_SHARE):
        return "regression", change
    if change < -threshold:
        return "improvement", change
    return "same", change


def verdicts(rows, baseline, history, config):
    out = []
    for row in rows:
        probe = row["probe"]
        threshold = config["probes"][probe].get("threshold_percent", config["default_threshold_percent"])
        before = baseline.get(probe, {}).get("value")
        status, change = judge(value_of(row), before, median_of(history, probe), threshold)
        out.append({"probe": probe, "status": status, "value": value_of(row), "baseline": before,
                    "baseline_commit": baseline.get(probe, {}).get("commit"),
                    "change_percent": round(change, 3), "threshold_percent": threshold})
    return out


def moved_baseline(baseline, found, commit, date):
    """The baseline after tonight: moved for a new probe and an improvement, kept otherwise."""
    out = dict(baseline)
    for v in found:
        if v["status"] in ("new", "improvement"):
            out[v["probe"]] = {"value": v["value"], "commit": commit, "date": date}
    return out


def summary(found, run, commit):
    """The Markdown of the run's page, and the body of an issue."""
    lines = ["| probe | now | baseline | change | threshold | verdict |", "|---|---:|---:|---:|---:|---|"]
    for v in found:
        base = "-" if v["baseline"] is None else f"{v['baseline']:,}"
        lines.append(f"| {v['probe']} | {v['value']:,} | {base} | {v['change_percent']:+.2f}% "
                     f"| {v['threshold_percent']}% | {v['status']} |")
    bad = [v for v in found if v["status"] == "regression"]
    if bad:
        lines += [""] + [f"- {v['probe']}: the commits since its baseline: `git log --oneline {v['baseline_commit']}..{commit}`"
                         for v in bad]
        lines += ["", "Bisect with `python3 tools/compile_report.py build/host-v96 <probe file>` (its instruction "
                  "count is another counter: compare the trend, not the numbers), or run `python3 tools/compile_cost.py "
                  "compile build/host-v96 <probe>` where valgrind is installed (the CI image has it)."]
    if run:
        lines += ["", f"The run: {run}"]
    return "\n".join(lines)


def read_json(path, default):
    p = pathlib.Path(path)
    return json.loads(p.read_text()) if p.is_file() and p.read_text().strip() else default


def read_history(path):
    p = pathlib.Path(path)
    return [json.loads(line) for line in p.read_text().splitlines() if line.strip()] if p.is_file() else []


def judge_command(args):
    config = json.loads(PROBES_FILE.read_text())
    config["probes"] = {p["id"]: p for p in config["probes"]}
    rows = json.loads(pathlib.Path(args.rows).read_text())
    baseline, history = read_json(args.baseline, {}), read_history(args.history)
    found = verdicts(rows, baseline, history, config)
    night = {"date": args.date, "commit": args.commit, "run": args.run, "rows": rows}
    with open(args.history, "a") as out:
        out.write(json.dumps(night, sort_keys=True) + "\n")
    pathlib.Path(args.baseline).write_text(
        json.dumps(moved_baseline(baseline, found, args.commit, args.date), indent=1, sort_keys=True) + "\n")
    print(json.dumps(found))
    print(summary(found, args.run, args.commit), file=sys.stderr)
    return 1 if any(v["status"] == "regression" for v in found) else 0


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
    parser.add_argument("command", choices=["judge"])
    for name in ("rows", "history", "baseline", "commit", "date"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument("--run", default="")
    return judge_command(parser.parse_args(argv))


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
