#!/usr/bin/env python3
"""Checks that every font that a program names has a file that draws it.

    python3 tools/check_fonts.py [-I dir ...] main.cpp screens.hpp font_title.cpp ...
    python3 tools/check_fonts.py --defined text.body main.cpp

A file that styles with lv::font_of<"text.title"> holds the address of an object that another
file defines (include/lv/look.hpp). A program that names a font and has no file that defines it
fails at the link, with the mangled name of a variable template. This check says it first, where
the font is named:

    screens.hpp:12: the font 'text.title' has no file that draws it. Add font_text_title.cpp with
    template <> const lv::drawn_font lv::font_object<"text.title"> = lv::draw<"text.title">(type);

The files are the sources of one program and the files they include, beside the file that names
them or through an include directory (-I dir, as the target has them: tools/include_graph.py). An
include that no directory resolves is a warning on stderr, never read as a file without fonts. Comments and
strings are not read, so a name in a comment or in a text is no use. --defined NAME says that a
library that the program links draws that font (the property LVGL26_FONTS of a target): `text.title`, or
`text.title@lv::signal::type_design` for a font of another design.

--build STAMP DEPFILE [-I dir ...] sources... is the form a build runs (cmake/check_fonts.cmake): it writes
STAMP when all is well, and DEPFILE, so that the check runs again only when a file changes.
Exits with 1 if a font is missing. Uses the standard library only.
"""

import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import include_graph  # noqa: E402

# A font of the project is named by its token alone; a font of another design adds the design
# (lv::font_of<"text.title", lv::signal::type_design>), and is told apart by it.
USE = re.compile(r'\bfont_of\s*<\s*"([^"]+)"\s*(?:,\s*([\w:]+)\s*)?>')
DEFINITION = re.compile(r'\bfont_object\s*<\s*"([^"]+)"\s*(?:,\s*([\w:]+)\s*)?>\s*=')
COMMENT = re.compile(r"//[^\n]*|/\*.*?\*/", re.S)
PLACEHOLDER = re.compile(r'"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'')


def key_of(name: str, design: str | None) -> str:
    """The name of a font as --defined and the messages write it: text.title, or text.title@design."""
    return f"{name}@{design}" if design else name


def file_name_of(font: str) -> str:
    """font_text_title.cpp for 'text.title'."""
    return "font_" + re.sub(r"[^A-Za-z0-9]+", "_", font) + ".cpp"


def without_comments(text: str) -> str:
    return COMMENT.sub(lambda m: re.sub(r"[^\n]", " ", m.group(0)), text)


def program_files(sources: list[pathlib.Path], include_dirs: list[pathlib.Path] = ()) -> list[pathlib.Path]:
    """The sources and the files they include, beside them or through an include directory."""
    files, unresolved = include_graph.files_of(sources, list(include_dirs))
    include_graph.warn(unresolved, "check_fonts.py")
    return files


def missing_fonts(files: list[pathlib.Path], defined: set[str]) -> list[str]:
    """One message for each name that is used and defined nowhere, at its first use."""
    uses: dict[str, tuple[pathlib.Path, int]] = {}
    for path in files:
        text = without_comments(path.read_text(errors="replace"))
        defined |= {key_of(*m) for m in DEFINITION.findall(text)}
        for match in USE.finditer(text):
            key = key_of(match.group(1), match.group(2))
            uses.setdefault(key, (path, text.count("\n", 0, match.start()) + 1))
    out = []
    for key, (path, line) in uses.items():
        if key in defined:
            continue
        name, _, design = key.partition("@")
        if design:
            out.append(
                f"{path.name}:{line}: the font '{name}' of the design {design} has no file that draws it. Link the library "
                f"that draws it, or add {file_name_of(name)} with template <> const lv::drawn_font "
                f'lv::font_object<"{name}", {design}> = lv::draw<"{name}", {design}>(type);'
            )
            continue
        out.append(
            f"{path.name}:{line}: the font '{name}' has no file that draws it. Add {file_name_of(name)} with "
            f'template <> const lv::drawn_font lv::font_object<"{name}"> = lv::draw<"{name}">(type);'
        )
    return out


def main(argv: list[str]) -> int:
    args = argv[1:]
    defined: set[str] = set()
    stamp = depfile = None
    while args and args[0].startswith("--"):
        flag = args.pop(0)
        if flag == "--defined":
            defined.add(args.pop(0))
        elif flag == "--build":
            stamp, depfile = args.pop(0), args.pop(0)
        else:
            print(f"unknown option {flag}", file=sys.stderr)
            return 2
    include_dirs, args = include_graph.split_include_dirs(args)
    files = program_files([pathlib.Path(a) for a in args], include_dirs)
    problems = missing_fonts(files, defined)
    for problem in problems:
        print(problem)
    if problems:
        return 1
    if stamp:
        pathlib.Path(stamp).parent.mkdir(parents=True, exist_ok=True)
        pathlib.Path(stamp).write_text("")
        pathlib.Path(depfile).write_text(f"{stamp}: " + " ".join(str(f).replace(" ", "\\ ") for f in files) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
