#!/usr/bin/env python3
"""Checks the website: every local link, asset and anchor resolves, every image
has alt text, and nothing is loaded from another site.

    python3 tools/check_site.py

Uses the standard library only. Exits with 1 and a list of problems if any.
"""

import pathlib
import sys
from html.parser import HTMLParser

SITE = pathlib.Path(__file__).resolve().parent.parent / "site"

# Attributes that make the browser fetch something for the page itself.
LOADS = {("img", "src"), ("script", "src"), ("link", "href"), ("source", "src"),
         ("source", "srcset"), ("video", "src"), ("video", "poster"), ("iframe", "src")}


class Page(HTMLParser):
    def __init__(self):
        super().__init__()
        self.ids = set()
        self.refs = []      # (tag, attribute, value)
        self.problems = []
        self.no_script = 0  # depth inside <noscript>

    def handle_starttag(self, tag, attrs):
        a = dict(attrs)
        if "id" in a:
            self.ids.add(a["id"])
        for name in ("href", "src", "srcset", "poster", "data-src"):
            if a.get(name):
                self.refs.append((tag, name, a[name]))
        if tag == "img" and not (a.get("alt") or "").strip():
            self.problems.append(f"an <img> without alt text: {a.get('src')}")
        if tag == "a" and not a.get("href"):
            self.problems.append("an <a> without href")


def external(value):
    return value.startswith(("http://", "https://", "//"))


def read(path):
    page = Page()
    page.feed(path.read_text(encoding="utf-8"))
    return page


def reference_problems(name, page, pages):
    """What is wrong with the links and the loaded files of one page."""
    problems = []
    for tag, attribute, value in page.refs:
        if value.startswith(("data:", "mailto:")):
            continue
        if external(value):
            # A link to another site is fine. Loading from one is not.
            if (tag, attribute) in LOADS or attribute == "data-src":
                problems.append(f"{name}: <{tag} {attribute}> loads from another site: {value}")
            continue
        target, _, anchor = value.partition("#")
        file = (SITE / name).parent / target if target else SITE / name
        if not file.is_file():
            problems.append(f"{name}: <{tag} {attribute}> points at a missing file: {value}")
            continue
        key = file.resolve().relative_to(SITE).as_posix()
        if anchor and file.suffix == ".html" and anchor not in pages[key].ids:
            problems.append(f"{name}: no id '{anchor}' in {key} (from {value})")
    return problems


def main():
    # Every page, also the generated ones in subfolders, by its path inside the site.
    pages = {p.relative_to(SITE).as_posix(): read(p) for p in sorted(SITE.rglob("*.html"))}
    problems = []
    if not pages:
        problems.append("no pages found")

    for name, page in pages.items():
        problems += [f"{name}: {p}" for p in page.problems]
        problems += reference_problems(name, page, pages)

    css = (SITE / "style.css").read_text(encoding="utf-8")
    if "@import" in css or "url(" in css:
        problems.append("style.css: loads another resource (@import or url())")

    for p in problems:
        print(p)
    print(f"{len(pages)} pages, {sum(len(p.refs) for p in pages.values())} references, "
          f"{len(problems)} problems")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
