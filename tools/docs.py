#!/usr/bin/env python3
"""Builds and checks the documentation in docs/.

    python3 tools/docs.py build    docs/*.md -> site/docs/*.html, and the files for agents
    python3 tools/docs.py check    every snippet equals its source; no C names in code
    python3 tools/docs.py sync     rewrites the snippets from their sources

The Markdown in docs/ is the one source. People read it on the site, or on GitHub.
Agents read site/llms.txt (an index), site/llms-full.txt (everything in one file) or
the page itself: every page is also published as Markdown, next to its HTML.

A code block that shows a tested file is marked on the line before it:

    <!-- snippet: examples/quick_start/main.cpp tree -->

It must equal the lines between the two `//! [tree]` lines of that file. Without a
name it must equal the whole file. `check` fails when they differ, so a page cannot
show code that the tests have not compiled.

A page for people who still have C code says `<!-- c-api: shown -->`. Every other
page must not name LVGL's C API in its code.

A compiler message on a page is tied to the compile-fail test that pins it:

    <!-- message: each_plain_key -->

before a ```text block. `check` fails when the text does not show what the test pins.

The rules of a guide are a block that `sync` writes from docs/reference/rules.json:

    <!-- rules: lists-1 fast-run-2 -->
    ... one line per rule ...
    <!-- rules: end -->

A page listed under `outline` in docs/docs.json follows the outline of a guide (docs/README.md):
its title, one sentence, the line `Header:`, then the sections of OUTLINE in their order.

Needs pandoc for `build`. `check` and `sync` use the standard library only.
"""

import html
import json
import pathlib
import re
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
DOCS = ROOT / "docs"
SITE = ROOT / "site"
OUT = SITE / "docs"
NOT_PAGES = {"research", "reference"}  # notes for contributors, and data: not pages

SNIPPET = re.compile(r"<!-- snippet: (\S+)(?: (\S+))? -->\n```(\w*)\n(.*?)```\n", re.S)
FENCE = re.compile(r"```(\w*)\n(.*?)```\n", re.S)
C_NAME = re.compile(r"\blv_[a-z0-9_]+\s*\(|\blv_[a-z0-9_]+_t\b|\bLV_[A-Z0-9_]+")
LANGUAGES = {"cpp": "C++26", "cmake": "CMake", "sh": "shell", "json": "JSON", "text": "output", "": "text"}


def pages() -> list[pathlib.Path]:
    """Every documentation page, the index first."""
    found = [p for p in sorted(DOCS.rglob("*.md")) if p.relative_to(DOCS).parts[0] not in NOT_PAGES]
    return sorted(found, key=lambda p: (p.name != "README.md", str(p)))


def source_of(path: str, name: str | None) -> str:
    """The lines of `path` that a snippet shows: the whole file, or one named part."""
    lines = (ROOT / path).read_text().splitlines()
    if name is not None:
        marks = [i for i, line in enumerate(lines) if line.strip() == f"//! [{name}]"]
        if len(marks) != 2:
            sys.exit(f"docs: {path} needs exactly two lines `//! [{name}]`, found {len(marks)}")
        lines = lines[marks[0] + 1:marks[1]]
    kept = [line for line in lines if not line.strip().startswith("//! [")]
    return "\n".join(kept).strip("\n") + "\n"


def snippet_problems(page: pathlib.Path, text: str) -> list[str]:
    problems = []
    for path, name, _, shown in SNIPPET.findall(text):
        if shown != source_of(path, name or None):
            problems.append(f"{page.relative_to(ROOT)}: the snippet of {path} {name} differs from the file")
    return problems


def c_name_problems(page: pathlib.Path, text: str) -> list[str]:
    if "<!-- c-api: shown -->" in text:
        return []
    return [f"{page.relative_to(ROOT)}: a code block names LVGL's C API: {found.group(0)}"
            for _, code in FENCE.findall(text) for found in [C_NAME.search(code)] if found]


def link_problems(page: pathlib.Path, text: str) -> list[str]:
    problems = []
    for target in re.findall(r"\]\(([^)#\s]+)(?:#[^)]*)?\)", text):
        if "://" in target or target.startswith("mailto:"):
            continue
        if not (page.parent / target).exists():
            problems.append(f"{page.relative_to(ROOT)}: the link to {target} leads nowhere")
    return problems


# ---- messages, rules and the outline of a guide ------------------------------------

MESSAGE = re.compile(r"<!-- message: (\w+) -->\n```text\n(.*?)```\n", re.S)
RULES = re.compile(r"<!-- rules: ([\w -]+?) -->\n(.*?)<!-- rules: end -->\n", re.S)
OUTLINE = ("The smallest example", "How it works", "What you can do", "What the compiler checks",
           "What it costs", "Rules", "Limits", "Names")
COSTS = ("Processor time", "RAM", "ROM", "Stack", "Compile time")
NOT_MEASURED = "not measured yet"
STATUS_PAGE = "status.html"
REFERENCE = "docs/reference/"
HEADER_LINE = re.compile(r"^Header: (.+)$", re.M)
HEADER_NAME = re.compile(r"`<(lv/[\w/.]+)>`")
FAIL_SNIPPET = re.compile(r"<!-- snippet: (tests/fail/(\w+)\.cpp) -->")


def message_problems(page: pathlib.Path, text: str) -> list[str]:
    """Every quoted message shows what its compile-fail test pins."""
    import rules  # tools/rules.py reads tests/CMakeLists.txt
    pinned = rules.pins()
    problems = []
    for test, shown in MESSAGE.findall(text):
        if test not in pinned:
            problems.append(f"{page.relative_to(ROOT)}: no compile-fail test is called {test}")
        elif found := rules.message_problem(test, shown, pinned):
            problems.append(f"{page.relative_to(ROOT)}: {found}")
    return problems


def rules_block(ids: str) -> str:
    import rules
    return f"<!-- rules: {ids} -->\n{rules.rule_list(ids.split(), rules.load())}<!-- rules: end -->\n"


def rules_problems(page: pathlib.Path, text: str) -> list[str]:
    import rules
    known = {r["id"] for r in rules.load()["rules"]}
    problems = []
    for ids, _ in RULES.findall(text):
        unknown = [i for i in ids.split() if i not in known]
        if unknown:
            problems.append(f"{page.relative_to(ROOT)}: no rule is called {', '.join(unknown)}")
        elif rules_block(ids) not in text:
            problems.append(f"{page.relative_to(ROOT)}: the rules of {ids} are stale: run python3 tools/docs.py sync")
    return problems


def without_code(text: str) -> str:
    """The text with each fenced block emptied, so that a `#` in code is no heading."""
    return FENCE.sub(lambda m: "```\n```\n", text)


def sections(text: str) -> dict[str, str]:
    """The `## ` sections of a page by title, each with its text, in order. Code is no heading."""
    found: dict[str, str] = {}
    title, fenced = None, False
    for line in text.splitlines(keepends=True):
        fenced = fenced != line.startswith("```")
        if not fenced and line.startswith("## "):
            title = line[3:].strip()
            found[title] = ""
        elif title is not None:
            found[title] += line
    return found


def order_problems(found: list[str]) -> list[str]:
    if found == list(OUTLINE):
        return []
    missing = [s for s in OUTLINE if s not in found]
    extra = [s for s in found if s not in OUTLINE]
    if missing or extra:
        return [f"the sections are not the outline's: missing {missing}, not in it {extra}"]
    return [f"the sections are out of order: {found}, the outline is {list(OUTLINE)}"]


def opening_blocks(text: str) -> list[str]:
    """The paragraphs before the first section, without comments."""
    opening = re.split(r"^## ", without_code(text), maxsplit=1, flags=re.M)[0]
    return [b.strip() for b in opening.split("\n\n") if b.strip() and not b.strip().startswith("<!--")]


def one_sentence(paragraph: str) -> bool:
    plain = re.sub(r"`[^`]*`", "", paragraph)
    return "\n" not in paragraph and plain.endswith(".") and ". " not in plain


def opening_problems(text: str) -> list[str]:
    """The title, one sentence of what the feature is for, and the line `Header:`."""
    blocks = opening_blocks(text)
    if len(blocks) < 3 or not blocks[0].startswith("# "):
        return ["a guide opens with its title, one sentence and the line `Header:`"]
    problems = [] if one_sentence(blocks[1]) else ["the line under the title is one sentence: what the feature is for"]
    return problems + ([] if HEADER_LINE.match(blocks[2]) else ["the third line is `Header: <the header>`"])


def needed_headers(text: str) -> set[str]:
    """The opt-in headers that the names in the page's examples need."""
    import check_includes
    table, _ = check_includes.load_table()
    code = "".join(shown for path, _, _, shown in SNIPPET.findall(text) if path.startswith("examples/"))
    names = {m.group(1) for m in check_includes.NAME.finditer(check_includes.code_of(code))}
    needed = {table[n][0] for n in names if n in table}
    return needed | ({"lv/list.hpp"} if check_includes.ROW_BIND.search(code) else set())


def header_problems(text: str) -> list[str]:
    line = HEADER_LINE.search(text)
    named = set(HEADER_NAME.findall(line.group(1))) if line else set()
    problems = [f"the header <{h}> does not exist" for h in sorted(named) if not (ROOT / "include" / h).is_file()]
    problems += [f"the line `Header:` lacks <{h}>, which the examples need"
                 for h in sorted(needed_headers(text) - named)]
    return problems + ([] if named else ["the line `Header:` names a header in backticks: `<lv/lv.hpp>`"])


def compiler_section_problems(body: str) -> list[str]:
    """Each compile-fail file shown is followed by its message; nothing else is code."""
    blocks = FENCE.findall(body)
    if not blocks:
        return [] if body.strip() else ["the section is empty"]
    tied = {test for test, _ in MESSAGE.findall(body)}
    shown = {name for _, name in FAIL_SNIPPET.findall(body)}
    problems = [f"tests/fail/{name}.cpp is shown without `<!-- message: {name} -->`" for name in sorted(shown - tied)]
    if len(blocks) != len(shown) + len(tied):
        problems.append("a block here is a file of tests/fail/ or a message tied to its test")
    return problems


def cost_rows(body: str) -> dict[str, list[str]]:
    rows = [line.strip().strip("|").split("|") for line in body.splitlines() if line.startswith("|")]
    return {cells[0].strip(): [c.strip() for c in cells[1:]] for cells in rows[2:] if cells}


def cost_problems(body: str) -> list[str]:
    """A table with a row per cost; each row says where its number comes from."""
    rows = cost_rows(body)
    if list(rows) != list(COSTS):
        return [f"the table of costs has the rows {list(COSTS)}, in this order"]
    problems = []
    for name, cells in rows.items():
        source = cells[-1] if cells else ""
        if "](" not in source and source != NOT_MEASURED:
            problems.append(f"the cost {name} links its source, or says {NOT_MEASURED}")
        elif source == NOT_MEASURED and cells[0] != NOT_MEASURED:
            problems.append(f"the cost {name} has no source, so it says {NOT_MEASURED}")
    return problems


def needs(found: bool, problem: str) -> list[str]:
    return [] if found else [problem]


# What each section must hold beyond some text. A section not named here holds anything.
SECTION_CHECKS = {
    "What the compiler checks": compiler_section_problems,
    "What it costs": cost_problems,
    "Rules": lambda body: needs(RULES.search(body) is not None, "the section holds a block `<!-- rules: ... -->`"),
    "Limits": lambda body: needs(STATUS_PAGE in body, f"the section links the status page ({STATUS_PAGE}) for what is planned"),
    "Names": lambda body: needs(REFERENCE in body, f"the section links the reference data ({REFERENCE})"),
}


def section_problems(title: str, body: str) -> list[str]:
    if not body.strip():
        return ["the section is empty; a section that does not apply says so in one line"]
    return SECTION_CHECKS.get(title, lambda _: [])(body)


def guide_rule_problems(page: pathlib.Path, body: str) -> list[str]:
    """Every rule that names this page as its guide is in its Rules section."""
    import rules
    listed = {i for ids, _ in RULES.findall(body) for i in ids.split()}
    own = [r["id"] for r in rules.load()["rules"] if r["guide"] == page.name]
    return [f"the rule {i} names this page as its guide, and the section Rules lacks it" for i in own if i not in listed]


def free_code_problems(text: str) -> list[str]:
    """Every C++ block is a snippet of a file that a test builds."""
    free = sum(language == "cpp" for language, _ in FENCE.findall(SNIPPET.sub("", text)))
    return [f"{free} C++ block(s) without a `<!-- snippet: -->` line: show a part of a built file"] if free else []


def outline_problems(page: pathlib.Path, text: str) -> list[str]:
    found = sections(text)
    problems = opening_problems(text) + header_problems(text) + order_problems(list(found)) + free_code_problems(text)
    for title, body in found.items():
        problems += [f"{title}: {p}" for p in section_problems(title, body)] if title in OUTLINE else []
    problems += guide_rule_problems(page, found.get("Rules", ""))
    return [f"{page.relative_to(ROOT)}: {p}" for p in problems]


def outlined() -> set[str]:
    return set(json.loads((DOCS / "docs.json").read_text()).get("outline", []))


def outline_name_problems(outline: set[str]) -> list[str]:
    """A name under `outline` that is no page would leave the outline unchecked."""
    return [f"docs/docs.json: the outline names {name}, which is no page" for name in sorted(outline)
            if not (DOCS / name).is_file()]


def page_problems(page: pathlib.Path, text: str, outline: set[str]) -> list[str]:
    problems = snippet_problems(page, text) + c_name_problems(page, text) + link_problems(page, text)
    problems += message_problems(page, text) + rules_problems(page, text)
    if str(page.relative_to(DOCS)) in outline:
        problems += outline_problems(page, text)
    return problems


def check() -> int:
    outline = outlined()
    problems = outline_name_problems(outline)
    for page in pages():
        problems += page_problems(page, page.read_text(), outline)
    for problem in problems:
        print(problem)
    print(f"{len(pages())} pages, {len(problems)} problems")
    return 1 if problems else 0


def synced(text: str) -> str:
    """The page with its snippets and its rules written again from their sources."""
    text = SNIPPET.sub(lambda m: f"<!-- snippet: {m[1]}{' ' + m[2] if m[2] else ''} -->\n"
                                 f"```{m[3]}\n{source_of(m[1], m[2] or None)}```\n", text)
    return RULES.sub(lambda m: rules_block(m[1]), text)


def sync() -> int:
    for page in pages():
        text = page.read_text()
        fresh = synced(text)
        if fresh != text:
            page.write_text(fresh)
            print(f"updated {page.relative_to(ROOT)}")
    return 0


# ---- build -------------------------------------------------------------------------

CPP_TOKEN = re.compile(
    r"(?P<c>//[^\n]*)|(?P<s>\"(?:\\.|[^\"\\\n])*\"|<[a-z_/.\-]+>(?=\s*$))|"
    r"(?P<k>#\s*\w+|\b(?:auto|bool|char|const|consteval|constexpr|constinit|co_await|co_return|else|for|"
    r"if|int|namespace|return|struct|template|using|void|while)\b)|"
    r"(?P<t>\b(?:lv|std|mp_units)::[\w:]+)|(?P<n>\b\d[\w.]*\b)", re.M)
OTHER_TOKEN = re.compile(r"(?P<c>#[^\n]*)|(?P<s>\"(?:\\.|[^\"\\\n])*\")", re.M)


def highlight(code: str, language: str) -> str:
    """`code` as HTML, with the site's five token classes."""
    pattern = CPP_TOKEN if language == "cpp" else OTHER_TOKEN if language in ("cmake", "sh") else None
    if pattern is None:
        return html.escape(code)
    out, at = [], 0
    for token in pattern.finditer(code):
        out.append(html.escape(code[at:token.start()]))
        out.append(f'<span class="{token.lastgroup}">{html.escape(token.group(0))}</span>')
        at = token.end()
    return "".join(out) + html.escape(code[at:])


def figure(code: str, language: str, label: str) -> str:
    kind = LANGUAGES.get(language, language)
    bar = f"<figcaption><span>{html.escape(label)}</span><span>{kind}</span></figcaption>" if label \
        else f'<div class="bar"><span>{kind}</span></div>'
    return f'<figure class="code">{bar}\n<pre tabindex="0"><code>{highlight(code, language)}</code></pre>\n</figure>'


def body_of(text: str, repository: str, depth: int) -> str:
    """The page as HTML: pandoc for the prose, our own figures for the code."""
    blocks: list[str] = []

    def keep(code: str, language: str, label: str) -> str:
        blocks.append(figure(code, language, label))
        return f"\nDOCSCODEBLOCK{len(blocks) - 1}\n\n"

    text = SNIPPET.sub(lambda m: keep(m[4], m[3], m[1]), text)
    text = FENCE.sub(lambda m: keep(m[2], m[1], ""), text)
    text = re.sub(r"<!--.*?-->\n?", "", text, flags=re.S)
    out = subprocess.run(["pandoc", "-f", "gfm", "-t", "html"], input=text, capture_output=True,
                         text=True, check=True).stdout
    out = re.sub(r"<p>DOCSCODEBLOCK(\d+)</p>", lambda m: blocks[int(m[1])], out)
    out = out.replace("<table>", '<div class="table"><table>').replace("</table>", "</table></div>")
    up = "../" * (depth + 1)
    out = re.sub(r'href="((?:\.\./)+)(?!docs/)([^":]+)"',
                 lambda m: f'href="{repository}/blob/main/{m[2]}"' if m[1] == up else m[0], out)
    return re.sub(r'href="([^":#]+)\.md(#[^"]*)?"', lambda m: f'href="{html_name(m[1])}.html{m[2] or ""}"', out)


def html_name(stem: str) -> str:
    """README is the index of its folder."""
    return re.sub(r"(^|/)README$", r"\1index", stem)


def title_and_summary(text: str) -> tuple[str, str]:
    title = re.search(r"^# (.+)$", text, re.M)
    prose = [p for p in re.split(r"\n\s*\n", text) if p.strip() and not p.lstrip().startswith(("#", "<!--", "```"))]
    plain = re.sub(r"[`*]|\[([^\]]+)\]\([^)]+\)", r"\1", prose[0]).replace("\n", " ") if prose else ""
    return (title.group(1) if title else "lvgl26"), plain.strip()


def sidebar(config: dict, current: str, root: str) -> str:
    out = [f'<p><a href="{root}docs/index.html">Documentation</a></p>']
    for section in config["sections"]:
        out.append(f"<h2>{html.escape(section['title'])}</h2>\n<ul>")
        for name in section["pages"]:
            title, _ = title_and_summary((DOCS / name).read_text())
            here = ' aria-current="page"' if name == current else ""
            out.append(f'<li><a href="{root}docs/{html_name(name[:-3])}.html"{here}>{html.escape(title)}</a></li>')
        out.append("</ul>")
    return "\n".join(out)


def render(page: pathlib.Path, config: dict, template: str) -> None:
    name = str(page.relative_to(DOCS))
    text = page.read_text()
    title, summary = title_and_summary(text)
    depth = name.count("/")
    root = "../" * (depth + 1)
    target = OUT / (html_name(name[:-3]) + ".html")
    target.parent.mkdir(parents=True, exist_ok=True)
    values = {"title": html.escape(title), "description": html.escape(summary, quote=True), "root": root,
              "sidebar": sidebar(config, name, root), "body": body_of(text, config["repository"], depth),
              "markdown": page.name, "edit": f"{config['repository']}/blob/main/docs/{name}"}
    target.write_text(re.sub(r"\{\{(\w+)\}\}", lambda m: values[m[1]], template))
    shutil.copyfile(page, OUT / name)


def agent_files(config: dict) -> None:
    """llms.txt lists the pages; llms-full.txt holds them all."""
    base = config["site"] + "docs/"
    index = [f"# {config['title']}", "", f"> {config['summary']}", "", config["for_agents"], ""]
    full = [f"# {config['title']}: the whole documentation", "", f"> {config['summary']}", ""]
    for section in config["sections"]:
        index += [f"## {section['title']}", ""]
        for name in section["pages"]:
            text = (DOCS / name).read_text()
            title, summary = title_and_summary(text)
            index.append(f"- [{title}]({base}{name}): {summary}")
            full += [f"<!-- {base}{name} -->", "", text.strip(), ""]
        index.append("")
    (SITE / "llms.txt").write_text("\n".join(index))
    (SITE / "llms-full.txt").write_text("\n".join(full))


def build() -> int:
    if shutil.which("pandoc") is None:
        sys.exit("docs: `build` needs pandoc (https://pandoc.org)")
    config = json.loads((DOCS / "docs.json").read_text())
    template = (ROOT / "tools" / "docs_page.html").read_text()
    shutil.rmtree(OUT, ignore_errors=True)
    for page in pages():
        render(page, config, template)
    if (DOCS / "media").is_dir():
        shutil.copytree(DOCS / "media", OUT / "media")
    agent_files(config)
    print(f"{len(pages())} pages -> {OUT.relative_to(ROOT)}, llms.txt, llms-full.txt")
    return 0


if __name__ == "__main__":
    commands = {"build": build, "check": check, "sync": sync}
    if len(sys.argv) != 2 or sys.argv[1] not in commands:
        sys.exit(__doc__)
    sys.exit(commands[sys.argv[1]]())
