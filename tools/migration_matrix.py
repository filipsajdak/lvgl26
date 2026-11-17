#!/usr/bin/env python3
"""Writes the migration readiness section of site/status.html from docs/migration/coverage.json.

    python3 tools/migration_matrix.py           writes the section into site/status.html
    python3 tools/migration_matrix.py --check   fails when the file is not what the data says

The data names, for each toolkit people migrate from, the concepts it has and whether lvgl26
Works, is Partial, In review, Planned or Not planned, and the gaps behind the rows that do not
work. A skill or a person reads the same file. Uses the standard library only.
"""

import argparse
import html
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
DATA = ROOT / "docs" / "migration" / "coverage.json"
PAGE = ROOT / "site" / "status.html"
START = "<!-- migration:start (written by tools/migration_matrix.py from docs/migration/coverage.json: do not edit) -->"
END = "<!-- migration:end -->"
PILL = {"works": "ok", "partial": "part", "in-review": "part", "planned": "plan", "not-planned": "none"}
CODE = re.compile(r"(lv::[A-Za-z0-9_:]+(?:<\\?\"[a-z_]+\\?\">)?(?:\(\.\.\.\))?|co_await [A-Za-z:.()0-9_]+)")


def gap_problems(gaps: list[dict], statuses: set[str]) -> list[str]:
    """Unknown statuses, duplicate ids, and a review without a pull request."""
    found, seen = [], set()
    for gap in gaps:
        if gap["id"] in seen:
            found.append(f"gap {gap['id']} is listed twice")
        seen.add(gap["id"])
        if gap["status"] not in statuses:
            found.append(f"gap {gap['id']} has the unknown status {gap['status']}")
        if gap["status"] == "in-review" and "pr" not in gap:
            found.append(f"gap {gap['id']} is in review and names no pull request")
    return found


def needs_gap(row: dict) -> bool:
    """A row that does not work, is not given up, and is not partial must say what is missing."""
    return row["status"] in ("planned", "in-review")


def row_problems(framework: dict, row: dict, gap_ids: set[str], statuses: set[str]) -> list[str]:
    """What is wrong with one row of a toolkit."""
    name = f"{framework['id']}: {row['concept']}"
    found = []
    if row["status"] not in statuses:
        found.append(f"{name} has the unknown status {row['status']}")
    if needs_gap(row) and "gap" not in row:
        found.append(f"{name} does not work and names no gap")
    if "gap" in row and row["gap"] not in gap_ids:
        found.append(f"{name} names the unknown gap {row['gap']}")
    return found


def problems(data: dict, root: pathlib.Path = ROOT) -> list[str]:
    """What is wrong with the data: unknown statuses and gaps, missing pages, a review without a number."""
    statuses = set(data["statuses"])
    gap_ids = {gap["id"] for gap in data["gaps"]}
    found = gap_problems(data["gaps"], statuses)
    for framework in data["frameworks"]:
        if not (root / framework["page"]).exists():
            found.append(f"{framework['id']}: the page {framework['page']} does not exist")
        for row in framework["rows"]:
            found += row_problems(framework, row, gap_ids, statuses)
    return found


def counts(framework: dict) -> tuple[int, int]:
    """How many concepts work, and how many there are."""
    rows = framework["rows"]
    return sum(row["status"] == "works" for row in rows), len(rows)


def code(text: str) -> str:
    """Escaped text, with the code in it set in <code>."""
    out, last = [], 0
    for found in CODE.finditer(text):
        out.append(html.escape(text[last:found.start()], quote=False))
        out.append(f"<code>{html.escape(found.group(0).replace(chr(92), ''), quote=False)}</code>")
        last = found.end()
    out.append(html.escape(text[last:], quote=False))
    return "".join(out)


def gap_item(gap: dict, label: dict) -> str:
    state = "" if gap["status"] == "planned" else f' data-state="{label[gap["status"]]}"'
    unblocks = "; ".join(gap["unblocks"])
    note = f"Unblocks: {unblocks}" if unblocks else "Replaced by the live preview and the design tools"
    if "pr" in gap:
        note += f". Pull request {gap['pr']}"
    return (f'      <li id="gap-{gap["id"]}"{state}><strong>{code(gap["title"])}</strong> '
            f'<span>{code(gap["what"])}</span> <small>{html.escape(note, quote=False)}</small></li>')


def framework_table(framework: dict, label: dict, gaps: dict) -> str:
    works, total = counts(framework)
    rows = []
    for row in framework["rows"]:
        gap = gaps.get(row.get("gap", ""))
        link = f'<a href="#gap-{gap["id"]}">{html.escape(gap["title"], quote=False)}</a>' if gap else ""
        rows.append(f'          <tr><th scope="row">{code(row["concept"])}</th>'
                    f'<td><span class="pill {PILL[row["status"]]}">{label[row["status"]]}</span></td>'
                    f'<td>{code(row["answer"])}</td><td>{link}</td></tr>')
    page = "docs/" + pathlib.Path(framework["page"]).stem + ".html"
    return "\n".join([
        f'    <h3 id="migration-{framework["id"]}">{html.escape(framework["name"], quote=False)}</h3>',
        f'    <p class="note">{works} of {total} concepts work. <a href="{page}">The concept map</a> explains each one.</p>',
        '    <div class="table wide">', "      <table>",
        "        <thead><tr><th>Concept</th><th>Status</th><th>In lvgl26</th><th>Gap</th></tr></thead>",
        "        <tbody>", *rows, "        </tbody>", "      </table>", "    </div>"])


def section(data: dict) -> str:
    label = data["statuses"]
    gaps = {gap["id"]: gap for gap in data["gaps"]}
    names = ", ".join(framework["name"] for framework in data["frameworks"][:-1]) + " and " + data["frameworks"][-1]["name"]
    parts = [
        START,
        '<section class="band tint" id="migration">',
        '  <div class="wrap">',
        '    <div class="head">',
        '      <div class="no">Migration</div>',
        "      <div>",
        "        <h2>Coming from another toolkit.</h2>",
        f"        <p>What lvgl26 covers of {names}, and what a port still has to work around. "
        f"Written from <code>docs/migration/coverage.json</code>, updated {data['updated']}.</p>",
        "      </div>", "    </div>", "",
        '    <h3 id="migration-gaps">What blocks a migration</h3>',
        '    <p class="note">The first two are built first. The rest is ordered by what each unblocks, and the order may change.</p>',
        '    <ul class="features planned">',
        *[gap_item(gap, label) for gap in data["gaps"]],
        "    </ul>", ""]
    for framework in data["frameworks"]:
        parts += [framework_table(framework, label, gaps), ""]
    parts += ["  </div>", "</section>", END]
    return "\n".join(parts)


def replaced(page: str, block: str) -> str:
    """The page with its migration block replaced, or put before the limits band when it has none."""
    start, end = page.find(START), page.find(END)
    if start >= 0 and end > start:
        return page[:start] + block + page[end + len(END):]
    marker = '<section class="band tint" id="limits">'
    at = page.index(marker)
    rule = page.rfind("<!-- ====", 0, at)
    return page[:rule] + block + "\n\n" + page[rule:]


def main(arguments: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Writes the migration section of the status page.")
    parser.add_argument("--check", action="store_true", help="fail when the page is not what the data says")
    options = parser.parse_args(arguments)
    data = json.loads(DATA.read_text())
    found = problems(data)
    for problem in found:
        print(f"migration: {problem}")
    if found:
        return 1
    page = PAGE.read_text()
    wanted = replaced(page, section(data))
    if options.check:
        if wanted != page:
            print("migration: site/status.html is not what docs/migration/coverage.json says: run tools/migration_matrix.py")
            return 1
        print(f"migration: {sum(len(f['rows']) for f in data['frameworks'])} rows, {len(data['gaps'])} gaps, 0 problems")
        return 0
    PAGE.write_text(wanted)
    print(f"migration: wrote {sum(len(f['rows']) for f in data['frameworks'])} rows and {len(data['gaps'])} gaps")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
