#!/usr/bin/env python3
"""Parses source files against the editor stubs, the way clangd will.

    python3 tools/editor/check.py --clang clang++ --commands build/compile_commands.json
        --stubs build/lvgl26-editor [--like FILE] [--verify]
        [--examples DIR] [--docs DIR] [FILE...]

For each file it takes the command the real build compiles it with, keeps what an
editor keeps (definitions, include and embed directories, the language standard),
adds the file overlay that leads to the stubs (overlay.yaml in the stub directory),
and runs `clang++ -fsyntax-only`.
A file the build does not compile borrows the command of --like, with its own
directory as an embed directory.

--examples adds every *.cpp file under a directory. --docs adds every C++ file that a
page under that directory shows as a snippet (`<!-- snippet: path name -->`), so the
code on a page is code the editor reads without an error.

Without --verify every file must parse with no error and no warning. With --verify a file says
which errors it expects, in Clang's own form, on the line they belong to:

    lv::label({.align = "centre"})   // expected-error {{not a constant expression}}

Uses the standard library only. Exits with 1 if a file does not do as expected.
"""

import argparse
import json
import pathlib
import re
import shlex
import subprocess
import sys

# #embed is an extension to Clang before C++26 mode has it. GCC refuses designated
# initializers out of order; Clang only warns, unless told.
EDITOR_WARNINGS = ["-Wno-c23-extensions", "-Werror=reorder-init-list"]
KEPT_PREFIXES = ("-D", "-I", "-std=", "--embed-dir=")
KEPT_WITH_VALUE = ("-isystem", "-I", "-D", "-include")
SNIPPET = re.compile(r"<!-- snippet: (\S+\.[ch]pp)(?: \S+)? -->")


def arguments_of(entry: dict) -> list[str]:
    return entry["arguments"] if "arguments" in entry else shlex.split(entry["command"])


def editor_flags(arguments: list[str]) -> list[str]:
    """What an editor keeps of a GCC command: no warnings, no GCC-only options, no output."""
    kept, at = [], 1
    while at < len(arguments):
        argument = arguments[at]
        if argument in KEPT_WITH_VALUE and at + 1 < len(arguments):
            kept += [argument, arguments[at + 1]]
            at += 1
        elif argument.startswith(KEPT_PREFIXES):
            kept.append(argument)
        at += 1
    return kept


def commands_by_file(path: pathlib.Path) -> dict[pathlib.Path, list[str]]:
    return {pathlib.Path(entry["file"]).resolve(): arguments_of(entry) for entry in json.loads(path.read_text())}


def flags_for(source: pathlib.Path, commands: dict[pathlib.Path, list[str]], like: pathlib.Path | None) -> list[str]:
    """The editor's flags for `source`: from its own command, or from the one of `like`."""
    if source in commands:
        return editor_flags(commands[source])
    if like is None or like.resolve() not in commands:
        sys.exit(f"editor check: the build does not compile {source}, and --like names no file it compiles")
    return editor_flags(commands[like.resolve()]) + [f"--embed-dir={source.parent}"]


def embed_flags(directories: list[pathlib.Path]) -> list[str]:
    return [f"--embed-dir={directory.resolve()}" for directory in directories]


def clang_command(clang: str, source: pathlib.Path, flags: list[str], stubs: pathlib.Path, verify: bool) -> list[str]:
    command = [clang, "-fsyntax-only", *EDITOR_WARNINGS, "-ivfsoverlay", str(stubs / "overlay.yaml"),
               "-idirafter", str(stubs / "fallback"), *flags]
    if verify:
        command += ["-Xclang", "-verify", "-Xclang", "-verify-ignore-unexpected=note,warning"]
    else:
        command.append("-Werror")  # a warning from the stubs would be a mark in the editor
    return command + [str(source)]


def snippet_sources(docs: pathlib.Path) -> list[pathlib.Path]:
    """The C++ files that the pages under `docs` show a part of. A path is from the repository's root."""
    root = docs.resolve().parent
    return [root / path for page in sorted(docs.glob("*.md")) for path in SNIPPET.findall(page.read_text())]


def files_of(options: argparse.Namespace) -> list[pathlib.Path]:
    """Every file to read, once: the named ones, the examples, the snippets of the pages."""
    files = [pathlib.Path(name) for name in options.files]
    for directory in options.examples:
        files += sorted(directory.rglob("*.cpp"))
    for directory in options.docs:
        files += snippet_sources(directory)
    return list(dict.fromkeys(path.resolve() for path in files))


SKIP = re.compile(r"^// editor-check: skip, (.+)$", re.MULTILINE)
SKIP_LINES = 10  # the line is at the top of the file


def skip_reason(source: pathlib.Path) -> str | None:
    """Why a file asks not to be read, or None. It says so in a comment near its top."""
    head = "\n".join(source.read_text().splitlines()[:SKIP_LINES])
    found = SKIP.search(head)
    return found.group(1) if found else None


def check(options: argparse.Namespace) -> int:
    commands = commands_by_file(options.commands)
    files = files_of(options)
    failed = 0
    skipped = 0
    for source in files:
        name = source.name if source.parent == pathlib.Path.cwd() else str(source)
        if (reason := skip_reason(source)) is not None:
            skipped += 1
            print(f"skipped: {name} ({reason})")
            continue
        command = clang_command(options.clang, source, flags_for(source, commands, options.like) + embed_flags(options.embed_dir), options.stubs,
                                options.verify)
        done = subprocess.run(command, capture_output=True, text=True, check=False)
        if done.returncode != 0:
            failed += 1
            print(done.stderr)
        print(f"{'FAILED' if done.returncode else 'ok'}: {name}")
    print(f"{len(files)} files, {failed} failed, {skipped} skipped")
    return 1 if failed or not files else 0


def main(arguments: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Parses source files against the editor stubs.")
    parser.add_argument("--clang", required=True, help="clang++ of version 19 or newer")
    parser.add_argument("--commands", type=pathlib.Path, required=True, help="compile_commands.json of the build")
    parser.add_argument("--stubs", type=pathlib.Path, required=True, help="the directory generate.py wrote")
    parser.add_argument("--like", type=pathlib.Path, help="a file of the build whose command the others borrow")
    parser.add_argument("--embed-dir", type=pathlib.Path, action="append", default=[], help="one more embed directory")
    parser.add_argument("--verify", action="store_true", help="the files say which errors they expect")
    parser.add_argument("--examples", type=pathlib.Path, action="append", default=[], help="a directory of *.cpp files")
    parser.add_argument("--docs", type=pathlib.Path, action="append", default=[], help="a directory of pages")
    parser.add_argument("files", nargs="*")
    return check(parser.parse_args(arguments))


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
