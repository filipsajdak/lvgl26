#!/usr/bin/env python3
"""The rules of good lvgl26 code: docs/reference/rules.json, and the page written from it.

    python3 tools/rules.py --page docs/rules.md         writes the page from the data
    python3 tools/rules.py --check-page docs/rules.md   fails when the data is wrong or the page stale

Each rule has an id such as `fast-build-2`: its quality and a number. An id never changes and is
never used again, so a review, a commit, a guide or a message can cite it. The data says, per rule:

    rule          one sentence a person and an agent can act on
    why           what the rule gives
    example       `path name`: the part of a built file between two `//! [name]` lines, or
                  `path` for the whole file; or null, with `example_note` saying why there is none
    check         null, or what enforces the rule in a program: by the compiler, the build or a
                  tool, the test that pins its message, and that message; a test that is no
                  compile-fail test names the file that pins it, `pinned_in`
    guide         the page that explains it: a page in docs/, or one of `pages_planned`
    evidence      null ("not measured yet"), or the note that holds a measurement, its date and
                  what it found
    wins_over     optional: which pull the rule beats when two qualities disagree, and why

The checks: unique ids of the form <quality>-<n>; an example's region exists; a check's test exists,
and the message equals what a compile-fail test pins; a guide exists or is planned, and a planned
page is not written yet; an evidence note exists. Uses the standard library only.
"""

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
DATA = ROOT / "docs" / "reference" / "rules.json"
DOCS = ROOT / "docs"
TESTS = ROOT / "tests"
PAGE_TITLE = "Writing good lvgl26 code"
REPOSITORY = "https://github.com/filipsajdak/lvgl26/blob/main/"
FIELDS = {"id", "quality", "rule", "why", "example", "check", "guide", "evidence"}
OPTIONAL = {"example_note", "wins_over"}
CHECKERS = {"compiler", "build", "tool"}
MEASURED = {"fast-run", "fast-build"}  # their rules rest on a measurement, or say there is none
COMPILE_FAIL = re.compile(r'^lvgl26_compile_fail\((\w+) "((?:\\.|[^"\\])*)"\)', re.M)
ADD_TEST = re.compile(r"add_test\(NAME (\w+)")
DATE = re.compile(r"^\d{4}-\d{2}-\d{2}$")
LANGUAGES = {".cpp": "cpp", ".hpp": "cpp", ".json": "json", ".txt": "cmake"}


def load(path: pathlib.Path = DATA) -> dict:
    return json.loads(path.read_text())


def cmake_text(quoted: str) -> str:
    """The value of a quoted CMake argument: `\\\\.` is `\\.` and `\\"` is `"`."""
    return re.sub(r'\\(["\\])', r"\1", quoted)


def pins() -> dict[str, str]:
    """Each compile-fail test of tests/CMakeLists.txt, with the regular expression it pins."""
    text = (TESTS / "CMakeLists.txt").read_text()
    return {name: cmake_text(quoted) for name, quoted in COMPILE_FAIL.findall(text)}


def test_names() -> set[str]:
    """Every test that tests/CMakeLists.txt adds, compile-fail tests by their own name."""
    text = (TESTS / "CMakeLists.txt").read_text()
    return set(ADD_TEST.findall(text)) | set(pins())


def message_problem(test: str, message: str, pinned: dict[str, str]) -> str | None:
    """Why `message` does not show what the test `test` pins, or None."""
    pin = pinned.get(test)
    if pin is None:
        return None
    if "${" in pin:
        return f"the test {test} pins its message through a CMake variable, which this check cannot read"
    if re.search(pin, message) is None:
        return f"the message differs from what the test {test} pins: {pin}"
    return None


def region_problem(reference: str) -> str | None:
    """Why `path name` names no region of a file, or None."""
    path, _, name = reference.partition(" ")
    file = ROOT / path
    if not file.is_file():
        return f"{path} does not exist"
    if name and sum(line.strip() == f"//! [{name}]" for line in file.read_text().splitlines()) != 2:
        return f"{path} needs exactly two lines `//! [{name}]`"
    return None


def example_problems(rule: dict) -> list[str]:
    if rule["example"] is None:
        return [] if rule.get("example_note") else ["an example of null needs an example_note"]
    found = region_problem(rule["example"])
    return [found] if found else []


def check_shape_problems(check: dict) -> list[str]:
    problems = [] if check.get("by") in CHECKERS else [f"a check is by one of {sorted(CHECKERS)}"]
    if not check.get("message"):
        problems.append("a check has the message it prints")
    if "tool" in check and not (ROOT / check["tool"]).exists():
        problems.append(f"the tool {check['tool']} does not exist")
    return problems + pinned_in_problems(check)


def pinned_in_problems(check: dict) -> list[str]:
    """A test that is no compile-fail test names the file that pins its message: it must hold it."""
    if "pinned_in" not in check:
        return []
    file = ROOT / check["pinned_in"]
    if not file.is_file() or check.get("message", "") not in file.read_text():
        return [f"{check['pinned_in']} does not hold the message"]
    return []


def check_test_problems(check: dict, tests: set[str], pinned: dict[str, str]) -> list[str]:
    test = check.get("test")
    if test is None:
        return []
    if test not in tests:
        return [f"the test {test} does not exist"]
    found = message_problem(test, check.get("message", ""), pinned)
    return [found] if found else []


def check_problems(check: dict | None, tests: set[str], pinned: dict[str, str]) -> list[str]:
    if check is None:
        return []
    return check_shape_problems(check) + check_test_problems(check, tests, pinned)


def guide_problems(guide: str, planned: dict[str, str]) -> list[str]:
    written = (DOCS / guide).is_file()
    if guide in planned:
        return [f"the planned page {guide} exists: remove it from pages_planned"] if written else []
    return [] if written else [f"the guide {guide} is neither a page nor planned"]


def evidence_problems(evidence: dict | None) -> list[str]:
    if evidence is None:
        return []
    problems = [] if (ROOT / evidence.get("source", "")).is_file() else ["the evidence names no file"]
    if not DATE.match(evidence.get("date", "")):
        problems.append("the evidence needs the date of the measurement")
    return problems + ([] if evidence.get("finding") else ["the evidence needs what was found"])


def shape_problems(rule: dict, qualities: set[str]) -> list[str]:
    keys = set(rule)
    problems = [f"no {key}" for key in sorted(FIELDS - keys)]
    problems += [f"an unknown field {key}" for key in sorted(keys - FIELDS - OPTIONAL)]
    quality, _, number = rule.get("id", "").rpartition("-")
    if quality != rule.get("quality") or quality not in qualities or not number.isdigit():
        problems.append("the id is <quality>-<n>, with a known quality")
    return problems


def rule_problems(rule: dict, data: dict, context: tuple[set[str], dict[str, str]]) -> list[str]:
    qualities = {q["id"] for q in data["qualities"]}
    problems = shape_problems(rule, qualities)
    if problems:
        return problems
    tests, pinned = context
    return (example_problems(rule) + check_problems(rule["check"], tests, pinned) +
            guide_problems(rule["guide"], data["pages_planned"]) + evidence_problems(rule["evidence"]))


def problems(data: dict) -> list[str]:
    """Everything wrong with the data, one line each, prefixed by the rule's id."""
    context = (test_names(), pins())
    found = []
    seen: set[str] = set()
    for rule in data["rules"]:
        name = rule.get("id", "?")
        if name in seen:
            found.append(f"{name}: the id is used twice")
        seen.add(name)
        found += [f"{name}: {p}" for p in rule_problems(rule, data, context)]
    return found


# ---- the page --------------------------------------------------------------------

def page_title(guide: str, data: dict) -> str:
    if guide in data["pages_planned"]:
        return data["pages_planned"][guide]
    heading = re.search(r"^# (.+)$", (DOCS / guide).read_text(), re.M)
    return heading.group(1) if heading else guide


def guide_line(guide: str, data: dict) -> str:
    title = page_title(guide, data)
    if guide in data["pages_planned"]:
        return f"Guide: {title} (not written yet)."
    return f"Guide: [{title}]({guide})."


def snippet(reference: str) -> list[str]:
    """The block that shows the region: the docs check compares it with the file."""
    import docs  # tools/docs.py imports this module for the guides' rules
    path, _, name = reference.partition(" ")
    language = LANGUAGES.get(pathlib.Path(path).suffix, "")
    return [f"<!-- snippet: {reference} -->", f"```{language}", docs.source_of(path, name or None).rstrip("\n"), "```"]


def example_lines(rule: dict) -> list[str]:
    if rule["example"] is None:
        return [rule["example_note"]]
    return snippet(rule["example"])


def check_lines(check: dict | None) -> list[str]:
    if check is None:
        return ["Checked: no tool checks this rule yet."]
    test = f" The test `{check['test']}` pins the message." if check.get("test") else ""
    who = {"compiler": "the compiler", "build": f"the build (`{check.get('tool', '')}`)",
           "tool": f"`{check.get('tool', '')}`"}[check["by"]]
    scope = f", in {check['scope']}" if check.get("scope") else ""
    return [f"Checked by {who}{scope}.{test}", "", "```text", check["message"], "```"]


def evidence_lines(rule: dict) -> list[str]:
    evidence = rule["evidence"]
    if evidence is None:
        return ["Measured: not measured yet."] if rule["quality"] in MEASURED else []
    link = f"[{pathlib.Path(evidence['source']).name}]({REPOSITORY}{evidence['source']})"
    return [f"Measured on {evidence['date']}, in {link}: {evidence['finding']}"]


def rule_section(rule: dict, data: dict) -> list[str]:
    lines = [f"### {rule['id']}", "", f"**{rule['rule']}**", "", rule["why"], ""]
    lines += example_lines(rule) + [""] + check_lines(rule["check"]) + [""]
    for extra in (evidence_lines(rule), [f"Wins over: {rule['wins_over']}"] if rule.get("wins_over") else []):
        lines += extra + ([""] if extra else [])
    return lines + [guide_line(rule["guide"], data), ""]


def index_table(data: dict) -> list[str]:
    rows = ["| Id | Rule |", "|---|---|"]
    rows += [f"| [`{r['id']}`](#{r['id']}) | {r['rule'].replace('|', '&#124;')} |" for r in data["rules"]]
    return rows


INTRO = [
    "The rules that make lvgl26 code short, fast to run, fast to compile, checked by the compiler, clear and easy to grow.",
    "",
    "<!-- Written by tools/rules.py from docs/reference/rules.json. Change the data, then run: python3 tools/rules.py --page docs/rules.md -->",
    "",
    "Each rule has an id that never changes. A guide ends with the rules of its feature, and a review or a commit can cite one by its id.",
    "",
    "Every example is a part of a file that the tests build. A message is the one the compiler prints, as its compile-fail test pins it.",
    "",
    "Where two qualities pull apart, a rule says which one wins, and why.",
    "",
]


def page(data: dict) -> str:
    lines = [f"# {PAGE_TITLE}", ""] + INTRO + index_table(data) + [""]
    for quality in data["qualities"]:
        lines += [f"## {quality['title']}", "", quality["aim"], ""]
        for rule in (r for r in data["rules"] if r["quality"] == quality["id"]):
            lines += rule_section(rule, data)
    return "\n".join(lines).rstrip("\n") + "\n"


def rule_list(ids: list[str], data: dict) -> str:
    """The lines a guide's Rules section shows for `ids`: a link to each rule, and its sentence."""
    by_id = {r["id"]: r for r in data["rules"]}
    return "".join(f"- [`{i}`](rules.md#{i}): {by_id[i]['rule']}\n" for i in ids)


def main(argv: list[str]) -> int:
    if len(argv) != 2 or argv[0] not in ("--page", "--check-page"):
        sys.exit(__doc__)
    data = load()
    found = problems(data)
    for problem in found:
        print(f"docs/reference/rules.json: {problem}")
    if found:
        return 1
    path = pathlib.Path(argv[1])
    if argv[0] == "--page":
        path.write_text(page(data))
        return 0
    if not path.exists() or path.read_text() != page(data):
        print(f"{path}: the page is stale. Write it: python3 tools/rules.py --page {path}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
