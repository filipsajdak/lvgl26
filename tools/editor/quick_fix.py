#!/usr/bin/env python3
"""Checks clangd's quick fix for a missing lvgl26 header, as an editor sees it.

    python3 tools/editor/quick_fix.py --clangd clangd --project DIR --file main.cpp --start theme.cpp
        --expect "Include <lv/scaled_font.hpp> for symbol lv::scaled_font" [--timeout 1200]

It starts clangd with its background index in DIR (a project configured with
lvgl26_editor_support(), so that DIR/.clangd leads to the stubs), opens START, waits until
the index of lvgl26 is built, opens FILE and looks for a diagnostic whose quick fixes include EXPECT.
It prints how long the index took. The test editor_quick_fix runs it every night.
"""

import argparse
import json
import pathlib
import queue
import subprocess
import sys
import threading
import time

INDEX_TOKEN = "backgroundIndexProgress"
# How long clangd may take to answer an opened file, once its index is built.
DIAGNOSTICS_SECONDS = 120


class Server:
    """A clangd process, spoken to in the language server protocol."""

    def __init__(self, clangd: str, project: pathlib.Path):
        self.process = subprocess.Popen([clangd, "--background-index", "--log=error"], cwd=project,
                                        stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
        self.messages: queue.Queue = queue.Queue()
        threading.Thread(target=self._read, daemon=True).start()

    def _read(self) -> None:
        stream = self.process.stdout
        while True:
            header = b""
            while not header.endswith(b"\r\n\r\n"):
                byte = stream.read(1)
                if not byte:
                    return
                header += byte
            length = next(int(line.split(b":")[1]) for line in header.split(b"\r\n")
                          if line.lower().startswith(b"content-length"))
            self.messages.put(json.loads(stream.read(length)))

    def send(self, message: dict) -> None:
        body = json.dumps({"jsonrpc": "2.0", **message}).encode()
        self.process.stdin.write(b"Content-Length: %d\r\n\r\n" % len(body) + body)
        self.process.stdin.flush()

    def next(self, deadline: float) -> dict | None:
        """The next message, answering clangd's own requests on the way; None after the deadline."""
        try:
            message = self.messages.get(timeout=max(0.0, deadline - time.monotonic()))
        except queue.Empty:
            return None
        if "id" in message and "method" in message:
            self.send({"id": message["id"], "result": None})
        return message

    def stop(self) -> None:
        self.process.kill()


def index_done(message: dict) -> bool:
    return (message.get("method") == "$/progress" and message["params"].get("token") == INDEX_TOKEN
            and message["params"]["value"].get("kind") == "end")


def fixes_of(message: dict, uri: str) -> list[str] | None:
    """The titles of the quick fixes in a diagnostics message for `uri`, or None for another message."""
    if message.get("method") != "textDocument/publishDiagnostics" or message["params"]["uri"] != uri:
        return None
    return [action.get("title", "") for diagnostic in message["params"]["diagnostics"]
            for action in diagnostic.get("codeActions", [])]


def wait_for(server: Server, seconds: float, found) -> object:
    """The first non-None result of found(message), or None when the time is up."""
    deadline = time.monotonic() + seconds
    while (message := server.next(deadline)) is not None:
        if (result := found(message)) is not None:
            return result
    return None


def start(server: Server, project: pathlib.Path) -> None:
    capabilities = {"window": {"workDoneProgress": True},
                    "textDocument": {"publishDiagnostics": {"codeActionsInline": True}}}
    server.send({"id": 1, "method": "initialize",
                 "params": {"processId": None, "rootUri": project.as_uri(), "capabilities": capabilities}})
    server.send({"method": "initialized", "params": {}})


def open_file(server: Server, source: pathlib.Path) -> None:
    document = {"uri": source.as_uri(), "languageId": "cpp", "version": 1, "text": source.read_text()}
    server.send({"method": "textDocument/didOpen", "params": {"textDocument": document}})


def check(options: argparse.Namespace) -> int:
    project = options.project.resolve()
    source = project / options.file
    server = Server(options.clangd, project)
    try:
        start(server, project)
        began = time.monotonic()
        # Opening a file makes clangd read the compile database, which starts the index. It is
        # another file: clangd does not work out again the diagnostics of a file whose text is
        # unchanged, and those of FILE must be worked out with the index.
        open_file(server, project / options.start)
        if wait_for(server, options.timeout, lambda m: True if index_done(m) else None) is None:
            print(f"quick_fix: clangd did not finish its index within {options.timeout} s", file=sys.stderr)
            return 1
        print(f"quick_fix: clangd built its index in {time.monotonic() - began:.0f} s")
        open_file(server, source)
        fixes = wait_for(server, DIAGNOSTICS_SECONDS,
                         lambda m: f if (f := fixes_of(m, source.as_uri())) and options.expect in f else None)
    finally:
        server.stop()
    if fixes is None:
        print(f"quick_fix: no quick fix '{options.expect}' in {options.file}", file=sys.stderr)
        return 1
    print(f"quick_fix: {options.file}: {options.expect}")
    return 0


def main(arguments: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Checks clangd's quick fix for a missing lvgl26 header.")
    parser.add_argument("--clangd", required=True)
    parser.add_argument("--project", type=pathlib.Path, required=True, help="a project with lvgl26_editor_support()")
    parser.add_argument("--file", required=True, help="the file, relative to the project")
    parser.add_argument("--start", required=True, help="another file of the project, opened to start the index")
    parser.add_argument("--expect", required=True, help="the title of the quick fix")
    parser.add_argument("--timeout", type=float, default=1200, help="seconds for the index")
    return check(parser.parse_args(arguments))


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
