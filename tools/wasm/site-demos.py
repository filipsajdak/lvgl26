#!/usr/bin/env python3
"""Writes the WebAssembly demos of the site: site/demo/<app>/index.html from the page of the
nightly run (tools/wasm/page/index.html), with the module left out. Each page loads demo.wasm
from its own folder.

    python3 tools/wasm/site-demos.py [<dir with counter.wasm and bounce.wasm>]

Without the folder, or without a module in it, only the pages are written: they then say that
the module is not published yet. pages.yml passes the modules of the release `wasm-demos`, which
the nightly job `web-browser` updates after the frames and the browser runs pass.
"""

import pathlib
import shutil
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
TEMPLATE = ROOT / "tools" / "wasm" / "page" / "index.html"
SOURCE = "https://github.com/filipsajdak/lvgl26/tree/main/examples/wasm/"

DEMOS = {
    "counter": ("lvgl26 in the browser: a counter",
                "The lvgl26 program <code>examples/wasm/counter/app.hpp</code>. Click the buttons, drag the slider."),
    "bounce": ("lvgl26 in the browser: the bouncing ball",
               "The bouncing ball of the motion demo, <code>examples/wasm/bounce/app.hpp</code>: a task that awaits one animation after another."),
}

NOTE = "<strong>Experimental:</strong> built by GCC trunk with an unmerged WebAssembly back end."


def page(app, title, blurb):
    foot = (f"<p><a href=\"{SOURCE}{app}\">The source</a> &middot; "
            "<a href=\"../../index.html\">lvgl26</a> &middot; "
            "<a href=\"../../status.html#target-browser\">What works on this target</a></p>")
    return (TEMPLATE.read_text(encoding="utf-8")
            .replace("@TITLE@", title).replace("@BLURB@", f"{NOTE} {blurb}").replace("@FOOT@", foot))


def main():
    modules = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else None
    missing = []
    for app, (title, blurb) in DEMOS.items():
        out = ROOT / "site" / "demo" / app
        out.mkdir(parents=True, exist_ok=True)
        (out / "index.html").write_text(page(app, title, blurb), encoding="utf-8")
        module = modules / f"{app}.wasm" if modules else None
        if module and module.is_file():
            shutil.copyfile(module, out / "demo.wasm")
        else:
            missing.append(app)
    for app in missing:
        print(f"site/demo/{app}: no module, the page says it is not published yet")
    return 0


if __name__ == "__main__":
    sys.exit(main())
