#!/usr/bin/env python3
"""Finds compile-time work that a header does where it is read, in every file that includes it.

    python3 tools/check_eager.py            checks include/lv; exit 1 on a finding
    python3 tools/check_eager.py FILE...    checks the files given

Two mistakes of CLAUDE.md ("Anything a header does at namespace scope costs compile time in
every file"):

1. An initializer at namespace scope that calls a function with no template argument:
   `template <class D> inline constexpr auto t = define_static_array(table());`. GCC evaluates a
   call that depends on nothing where the template is read, though no file names `t<...>`.
   The fix is a template argument that depends on D: `table<D>()`.
2. A consteval function with no template parameter whose body scans with constant arguments
   (`std::meta::members_of(^^::, ...)`): its scan is run where the header is read.

It reads text, not what GCC does: a finding is a likely cost, and a cost the text does not show
(an eager call hidden behind a macro-free alias, a call through a lambda) is not found. The allow
list, tools/eager_allowed.json, names the deliberate cases with a reason each. Uses the standard
library only.
"""

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
HEADERS = ROOT / "include" / "lv"
ALLOWED = ROOT / "tools" / "eager_allowed.json"

# `inline constexpr <type> name = <initializer>;` or `constexpr <type> name<...> = ...;`, with the
# template line before it, at namespace scope (no indentation).
NAMESPACE_CONSTANT = re.compile(r"^(?:inline\s+)?constexpr\s+[^=;(]*?\b(\w+)\s*=\s*(.*?);", re.M | re.S)
# A call with no template argument: `name(` not preceded by `>` of `name<...>(` and not a cast.
PLAIN_CALL = re.compile(r"(?<![\w>:.])(?:\w+::)*(\w+)\s*\(")
TEMPLATE_CALL = re.compile(r"\b(\w+)\s*<[^;]*?>\s*\(")
# Calls that do no work worth a finding: constructors of values, casts, the standard's own.
HARMLESS = {
    "static_cast", "reinterpret_cast", "const_cast", "sizeof", "alignof", "decltype", "noexcept",
    "define_static_array", "define_static_object", "define_static_string", "reflect_constant",
    "reflect_constant_string", "data", "size", "span", "string_view", "array", "requires", "void",
    "constexpr", "unchecked",
}
SCAN = re.compile(r"std::meta::(?:members_of|nonstatic_data_members_of|enumerators_of)\s*\(\s*\^\^\s*::")
CONSTEVAL_FUNCTION = re.compile(r"^consteval\s+[^;{]*?\b(\w+)\s*\([^)]*\)\s*\{", re.M)


def strip_comments(text: str) -> str:
    """The text with comments and string contents blanked, line numbers kept."""
    text = re.sub(r"//[^\n]*", "", text)
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), text, flags=re.S)
    return re.sub(r'"(?:\\.|[^"\\\n])*"', '""', text)


def line_of(text: str, at: int) -> int:
    return text.count("\n", 0, at) + 1


def plain_calls(initializer: str) -> list[str]:
    """The functions an initializer calls with no template argument."""
    templated = {m.group(1) for m in TEMPLATE_CALL.finditer(initializer)}
    calls = []
    for m in PLAIN_CALL.finditer(initializer):
        name = m.group(1)
        if name in HARMLESS or name in templated or name[0].isupper():
            continue
        calls.append(name)
    return calls


TEMPLATE_HEAD = re.compile(r"template\s*<(.*)>\s*(?:requires[^\n]*)?$")
PARAMETER = re.compile(r"(?:class|typename|auto|[\w:]+)\s*(?:&|\.\.\.)?\s*(\w+)\s*(?:=[^,]*)?(?:,|$)")


def template_parameters(text: str, at: int) -> list[str] | None:
    """The parameter names of the template line just before `at`, or None when there is none."""
    before = text[:at].rstrip().splitlines()
    if not before:
        return None
    head = TEMPLATE_HEAD.match(before[-1].strip())
    if not head:
        return None
    return [m.group(1) for m in PARAMETER.finditer(head.group(1))]


def depends_on(initializer: str, names: list[str]) -> bool:
    return any(re.search(rf"\b{re.escape(n)}\b", initializer) for n in names)


# A reflection scan: a search of a scope or of a type's members.
ANY_SCAN = re.compile(r"std::meta::(?:members_of|nonstatic_data_members_of|enumerators_of|parameters_of)\s*\(")
FUNCTION = re.compile(r"^(?:template\s*<[^\n]*>\s*\n)?(?:consteval|constexpr)\s+[^;{=]*?\b(\w+)\s*\([^;{]*\)\s*\{", re.M)


def function_bodies(texts: list[str]) -> dict[str, str]:
    """The body of each function of the headers, by name (overloads joined)."""
    bodies: dict[str, str] = {}
    for text in texts:
        for m in FUNCTION.finditer(text):
            bodies[m.group(1)] = bodies.get(m.group(1), "") + function_body(text, m.end() - 1)
    return bodies


def calls_any(body: str, names: set[str]) -> bool:
    return any(re.search(rf"\b{re.escape(n)}\s*[<(]", body) for n in names)


def scanning_functions(texts: list[str]) -> set[str]:
    """The functions of the headers that scan, or call one that does: the calls that cost."""
    bodies = function_bodies(texts)
    scanning = {name for name, body in bodies.items() if ANY_SCAN.search(body)}
    while True:
        more = {name for name, body in bodies.items() if name not in scanning and calls_any(body, scanning)}
        if not more:
            return scanning
        scanning |= more


def eager_initializers(text: str, scanning: set[str]) -> list[tuple[int, str, str]]:
    """(line, constant, call) for each namespace-scope initializer that does work where the header
    is read: in a variable template, any call in an initializer that names none of its parameters;
    in a variable, a call of a function that scans (a value such as lv::ease::in_out is cheap)."""
    found = []
    for m in NAMESPACE_CONSTANT.finditer(text):
        names = template_parameters(text, m.start())
        if names is not None and depends_on(m.group(2), names):
            continue
        for call in plain_calls(m.group(2)):
            if names is not None or call in scanning:
                found.append((line_of(text, m.start()), m.group(1), call))
    return found


def function_body(text: str, start: int) -> str:
    """The body of the function whose `{` is at `start`."""
    depth = 0
    for i in range(start, len(text)):
        depth += {"{": 1, "}": -1}.get(text[i], 0)
        if depth == 0:
            return text[start:i + 1]
    return text[start:]


def eager_scans(text: str) -> list[tuple[int, str, str]]:
    """(line, function, "scan") for each non-template consteval function that scans the global scope."""
    found = []
    for m in CONSTEVAL_FUNCTION.finditer(text):
        preceding = text[:m.start()].rstrip().splitlines()[-1:] or [""]
        if preceding[0].startswith("template"):
            continue
        if SCAN.search(function_body(text, m.end() - 1)):
            found.append((line_of(text, m.start()), m.group(1), "scan"))
    return found


def findings(name: str, text: str, scanning: set[str]) -> list[tuple[str, str, str]]:
    """(key, message) of each finding of one header; the key is `file: name`."""
    clean = strip_comments(text)
    out = []
    for line, what, call in eager_initializers(clean, scanning):
        out.append((f"{name}: {what}", f"{name}:{line}: {what} calls {call}() where the header is read"))
    for line, what, _ in eager_scans(clean):
        out.append((f"{name}: {what}",
                    f"{name}:{line}: the consteval function {what} scans the global scope and has no template parameter"))
    return out


def allowed() -> dict[str, str]:
    """`file: name` and why its work is deliberate."""
    return json.loads(ALLOWED.read_text())["allowed"]


def display_name(path: pathlib.Path) -> str:
    return path.relative_to(ROOT).as_posix() if path.is_relative_to(ROOT) else path.name


def stale(allow: dict[str, str], keys: set[str], checked: set[str]) -> list[str]:
    """Allowed findings of the checked files that are no longer found."""
    return [f"tools/eager_allowed.json: '{key}' is allowed but no longer found: remove it"
            for key in allow if key not in keys and key.split(":")[0] in checked]


def problems_in(files: list[pathlib.Path], allow: dict[str, str]) -> list[str]:
    scanning = scanning_functions([strip_comments(h.read_text()) for h in sorted(HEADERS.rglob("*.hpp"))])
    found = [f for path in files for f in findings(display_name(path), path.read_text(), scanning)]
    problems = [message for key, message in found if key not in allow]
    return problems + stale(allow, {key for key, _ in found}, {display_name(p) for p in files})


def main(argv: list[str]) -> int:
    files = [pathlib.Path(a).resolve() for a in argv] or sorted(HEADERS.rglob("*.hpp"))
    problems = problems_in(files, allowed())
    for p in problems:
        print(p, file=sys.stderr)
    if problems:
        print("check_eager: give the call a template argument that depends on the template's parameter "
              "(table<D>()), or list it in tools/eager_allowed.json with a reason", file=sys.stderr)
    print(f"{len(files)} headers, {len(problems)} eager findings")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
