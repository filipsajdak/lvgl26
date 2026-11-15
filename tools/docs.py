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
NOT_PAGES = {"research"}  # notes for contributors, not documentation

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


def check() -> int:
    problems = []
    for page in pages():
        text = page.read_text()
        problems += snippet_problems(page, text) + c_name_problems(page, text) + link_problems(page, text)
    for problem in problems:
        print(problem)
    print(f"{len(pages())} pages, {len(problems)} problems")
    return 1 if problems else 0


def sync() -> int:
    for page in pages():
        text = page.read_text()
        fresh = SNIPPET.sub(lambda m: f"<!-- snippet: {m[1]}{' ' + m[2] if m[2] else ''} -->\n"
                                      f"```{m[3]}\n{source_of(m[1], m[2] or None)}```\n", text)
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
