#!/usr/bin/env python3
"""The history of the nightly compile-cost run, and what it says about tonight's rows.

    python3 tools/cost_history.py judge --rows rows.json --history history.jsonl \
        --baseline baseline.json --commit SHA --date 2026-11-20 [--run URL]
        [--accept PROBE,PROBE --reason TEXT]
    python3 tools/cost_history.py judge-pr --base ROWS --head ROWS

`rows.json` is a list of rows of tools/compile_cost.py. `judge` appends the night to the history and
moves the baseline, both files in place, and prints the verdicts as JSON on standard output and a
Markdown summary on standard error. The exit status is 1 when a probe is a regression.

Two rules, both with numbers in tools/metrics/probes.json (a probe may name its own):

- A change (`judge-pr`, for a pull request): up to `warn_percent` it merges with no comment; between
  `warn_percent` and `block_percent` it merges when the pull request carries the cost table and a one-line
  reason; above `block_percent` it waits until it is fixed or the owner accepts it. Exit status 1 for a block.
- The drift (`judge`, every night): a probe is a regression when it is more than `drift_percent` above its
  baseline, and stays one until it is back under `clear_percent` (the hysteresis, from the verdict of the
  night before). The baseline moves when a probe is new, got cheaper by more than `warn_percent`, or a rise
  is accepted; it never creeps up by itself, so small rises add up until they cross `drift_percent`.

A rise that the owner accepted is recorded, never allowed in general: `--accept` names the probes and
`--reason` says why (the pull request and what it bought). A named probe that is a regression tonight gets
the verdict `accepted`, its baseline moves to tonight's value with the reason, and the night's line of the
history carries the same record. Nothing is rewritten: the history only grows. `--accept` without a reason
is refused.
Uses the standard library only.
"""

import argparse
import json
import pathlib
import sys

PROBES_FILE = pathlib.Path(__file__).resolve().parent / "metrics" / "probes.json"
# The four limits of a probe, in percent: probes.json holds them for all, and a probe may name its own.
LIMITS = ("warn_percent", "block_percent", "drift_percent", "clear_percent")


def value_of(row):
    return row["ir"] if row["kind"] == "compile" else row["value"]


def percent(now, before):
    return 100.0 * (now - before) / before if before else 0.0


def limits_of(config, probe):
    """The four limits of a probe: its own, else those of the file."""
    own = config["probes"][probe]
    return {key: own.get(key, config[key]) for key in LIMITS}


def judge(value, baseline, previous, limits):
    """(status, change in percent against the baseline): new, same, regression or improvement.

    `previous` is the verdict of the night before: a probe that was a regression stays one until it is back
    under clear_percent.
    """
    if baseline is None:
        return "new", 0.0
    change = percent(value, baseline)
    if change > limits["drift_percent"] or (previous == "regression" and change > limits["clear_percent"]):
        return "regression", change
    if change < -limits["warn_percent"]:
        return "improvement", change
    return "same", change


def previous_status(history, probe):
    """The verdict of `probe` in the last night of the history, or None."""
    return history[-1].get("verdicts", {}).get(probe) if history else None


def verdicts(rows, baseline, history, config, accepted=()):
    out = []
    for row in rows:
        probe = row["probe"]
        limits = limits_of(config, probe)
        before = baseline.get(probe, {}).get("value")
        status, change = judge(value_of(row), before, previous_status(history, probe), limits)
        if status == "regression" and probe in accepted:
            status = "accepted"
        out.append({"probe": probe, "status": status, "value": value_of(row), "baseline": before,
                    "baseline_commit": baseline.get(probe, {}).get("commit"),
                    "change_percent": round(change, 3), "drift_percent": limits["drift_percent"],
                    "clear_percent": limits["clear_percent"]})
    return out


def moved_baseline(baseline, found, commit, date, reason=""):
    """The baseline after tonight: moved for a new probe, an improvement and an accepted rise, kept otherwise."""
    out = dict(baseline)
    for v in found:
        if v["status"] in ("new", "improvement", "accepted"):
            out[v["probe"]] = {"value": v["value"], "commit": commit, "date": date}
            if v["status"] == "accepted":
                out[v["probe"]]["accepted"] = reason
    return out


def summary(found, run, commit):
    """The Markdown of the run's page, and the body of an issue."""
    lines = ["| probe | now | baseline | change | drift limit | verdict |", "|---|---:|---:|---:|---:|---|"]
    for v in found:
        base = "-" if v["baseline"] is None else f"{v['baseline']:,}"
        lines.append(f"| {v['probe']} | {v['value']:,} | {base} | {v['change_percent']:+.2f}% "
                     f"| {v['drift_percent']}% | {v['status']} |")
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
    accepted = [p for p in args.accept.split(",") if p]
    unknown = [p for p in accepted if p not in config["probes"]]
    if accepted and not args.reason.strip():
        print("--accept needs a --reason: what the rise bought, and the pull request", file=sys.stderr)
        return 2
    if unknown:
        print("--accept names no probe: " + ", ".join(unknown), file=sys.stderr)
        return 2
    found = verdicts(rows, baseline, history, config, accepted)
    night = {"date": args.date, "commit": args.commit, "run": args.run, "rows": rows,
             "verdicts": {v["probe"]: v["status"] for v in found}}
    if accepted:
        night["accepted"] = {"probes": accepted, "reason": args.reason.strip()}
    with open(args.history, "a") as out:
        out.write(json.dumps(night, sort_keys=True) + "\n")
    pathlib.Path(args.baseline).write_text(
        json.dumps(moved_baseline(baseline, found, args.commit, args.date, args.reason.strip()), indent=1, sort_keys=True) + "\n")
    print(json.dumps(found))
    print(summary(found, args.run, args.commit), file=sys.stderr)
    return 1 if any(v["status"] == "regression" for v in found) else 0


def rows_of(path):
    """The rows of a file (a list, or one row) or of every .json file under a directory."""
    p = pathlib.Path(path)
    files = sorted(p.rglob("*.json")) if p.is_dir() else [p]
    rows = []
    for f in files:
        data = json.loads(f.read_text())
        rows += data if isinstance(data, list) else [data]
    return {row["probe"]: row for row in rows if "probe" in row}


def judge_pr(base, head, config):
    """The verdict of a change on each probe: ok, table or block, with the change in percent."""
    out = []
    for probe, row in head.items():
        if probe not in base or probe not in config["probes"]:
            continue
        limits = limits_of(config, probe)
        change = percent(value_of(row), value_of(base[probe]))
        status = "block" if change > limits["block_percent"] else "table" if change > limits["warn_percent"] else "ok"
        out.append({"probe": probe, "base": value_of(base[probe]), "head": value_of(row),
                    "change_percent": round(change, 3), "status": status})
    return out


def judge_pr_text(found):
    lines = ["| probe | base | head | change | verdict |", "|---|---:|---:|---:|---|"]
    for v in found:
        lines.append(f"| {v['probe']} | {v['base']:,} | {v['head']:,} | {v['change_percent']:+.3f}% | {v['status']} |")
    notes = {"table": "A rise between the warn and the block limit merges with this table and a one-line reason in the pull request.",
             "block": "A rise above the block limit waits until it is fixed or the owner accepts it."}
    lines += [""] + sorted({notes[v["status"]] for v in found if v["status"] in notes})
    return "\n".join(lines)


def judge_pr_command(args):
    config = json.loads(PROBES_FILE.read_text())
    config["probes"] = {p["id"]: p for p in config["probes"]}
    found = judge_pr(rows_of(args.base), rows_of(args.head), config)
    print(judge_pr_text(found))
    return 1 if any(v["status"] == "block" for v in found) else 0


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
    parser.add_argument("command", choices=["judge", "judge-pr"])
    parser.add_argument("--base", help="judge-pr: the rows of the base, a file or a directory of rows")
    parser.add_argument("--head", help="judge-pr: the rows of the change")
    for name in ("rows", "history", "baseline", "commit", "date"):
        parser.add_argument("--" + name)
    parser.add_argument("--run", default="")
    parser.add_argument("--accept", default="", help="probes whose rise tonight is accepted, comma separated")
    parser.add_argument("--reason", default="", help="why the rise is accepted; required with --accept")
    args = parser.parse_args(argv)
    if args.command == "judge-pr":
        if not (args.base and args.head):
            parser.error("judge-pr needs --base and --head")
        return judge_pr_command(args)
    missing = [n for n in ("rows", "history", "baseline", "commit", "date") if not getattr(args, n)]
    if missing:
        parser.error("judge needs --" + ", --".join(missing))
    return judge_command(args)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
