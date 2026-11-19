#!/usr/bin/env python3
"""Writes site/metrics.html, the trend of the nightly compile-cost run, from its history.

    python3 tools/cost_page.py history.jsonl site/metrics.html

The history is the file history.jsonl of the branch `metrics`: one JSON object a night. The page is
generated: pages.yml writes it before the site is checked, and it is not committed.
Uses the standard library only.
"""

import html
import json
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import cost_history as history  # noqa: E402

REPOSITORY = "https://github.com/filipsajdak/lvgl26"
NIGHTS_SHOWN = 30
SPARK_WIDTH = 240
SPARK_HEIGHT = 48
SHORT_SHA = 7
UNITS = {"stack": "bytes of stack", "run": "instructions per simulated second"}

HEAD = """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="color-scheme" content="light dark">
<title>Compile cost: lvgl26</title>
<meta name="description" content="What a file that uses lvgl26 costs the compiler, measured every night as Valgrind's instruction count, with its trend.">
<link rel="stylesheet" href="style.css">
</head>
<body>
<header class="top">
  <div class="wrap">
    <a class="mark" href="index.html">lvgl26 <span>namespace lv</span></a>
    <nav aria-label="Main">
      <ul>
        <li><a href="index.html">Overview</a></li>
        <li><a href="docs/index.html">Docs</a></li>
        <li><a href="agents.html">Agents</a></li>
        <li><a href="design.html">Design</a></li>
        <li><a href="status.html">Status</a></li>
        <li><a href="%s">GitHub</a></li>
      </ul>
    </nav>
  </div>
</header>
<main id="main">
<section class="hero plain"><div class="wrap">
<p class="eyebrow">Compile cost</p>
<h1>What a file costs the compiler, night by night.</h1>
<p class="lede">Each probe is one file compiled by GCC. The number is Valgrind's count of the instructions that the compiler ran, so it is the same on every run. A lower number is better. The history is on the branch <a href="%s/tree/metrics">metrics</a>.</p>
</div></section>
<section class="band"><div class="wrap">
"""

FOOT = """</div></section>
</main>
<footer class="foot"><div class="wrap"><p>Generated from the history of the nightly run. <a href="status.html">Status</a></p></div></footer>
</body>
</html>
"""


def series(nights, probe):
    """[(date, commit, value)] of one probe, oldest first."""
    out = []
    for night in nights:
        for row in night["rows"]:
            if row["probe"] == probe:
                out.append((night["date"], night["commit"], history.value_of(row)))
    return out


def sparkline(values):
    """An inline SVG polyline of the values, scaled to their own range."""
    if len(values) < 2:
        return ""
    low, high = min(values), max(values)
    span = (high - low) or 1
    step = SPARK_WIDTH / (len(values) - 1)
    points = " ".join(f"{i * step:.1f},{SPARK_HEIGHT - (v - low) / span * SPARK_HEIGHT:.1f}" for i, v in enumerate(values))
    return (f'<svg width="{SPARK_WIDTH}" height="{SPARK_HEIGHT}" viewBox="0 0 {SPARK_WIDTH} {SPARK_HEIGHT}" '
            f'role="img" aria-label="trend"><polyline fill="none" stroke="currentColor" stroke-width="2" points="{points}"/></svg>')


def probe_section(probe, nights):
    points = series(nights, probe["id"])[-NIGHTS_SHOWN:]
    if not points:
        return ""
    rows = "".join(
        f'<tr><td>{html.escape(d)}</td><td><a href="{REPOSITORY}/commit/{c}">{c[:SHORT_SHA]}</a></td><td class="num">{v:,}</td></tr>'
        for d, c, v in reversed(points[-10:]))
    unit = UNITS.get(probe["kind"], "instructions")
    return (f'<h2 id="{html.escape(probe["id"])}">{html.escape(probe["id"])}</h2>'
            f'<p>{html.escape(probe["stands_for"])}. The unit is {unit}.</p>{sparkline([p[2] for p in points])}'
            f'<div class="table"><table><thead><tr><th>Night</th><th>Commit</th><th>{unit}</th></tr></thead>'
            f'<tbody>{rows}</tbody></table></div>')


def render(nights, probes):
    body = "".join(probe_section(p, nights) for p in probes)
    if not body:
        body = "<p>No night has been measured yet.</p>"
    return HEAD % (REPOSITORY, REPOSITORY) + body + FOOT


def main(argv):
    if len(argv) != 2:
        print(__doc__.strip().splitlines()[0], file=sys.stderr)
        return 2
    probes = json.loads(history.PROBES_FILE.read_text())["probes"]
    pathlib.Path(argv[1]).write_text(render(history.read_history(argv[0]), probes))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
