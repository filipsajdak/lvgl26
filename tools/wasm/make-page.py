#!/usr/bin/env python3
"""Writes the page with the module embedded: one file, no server.
   make-page.py page/index.html demo.wasm script.json <sha256 of the host's frames> [title [blurb]] > out.html"""
import base64, json, sys
page, wasm, script, expected = sys.argv[1:5]
title = sys.argv[5] if len(sys.argv) > 5 else "lvgl26 in the browser"
blurb = sys.argv[6] if len(sys.argv) > 6 else "The unchanged lvgl26 program examples/wasm/counter/app.hpp. Click the buttons, drag the slider."
t = open(page).read().replace('@TITLE@', title).replace('@BLURB@', blurb).replace('@FOOT@', '')
for old, new in [("const EMBEDDED = null;", "const EMBEDDED = '%s';" % base64.b64encode(open(wasm, 'rb').read()).decode()),
                 ("const SCRIPT = null;", "const SCRIPT = %s;" % json.dumps(json.load(open(script)))),
                 ("const EXPECTED_HASH = null;", "const EXPECTED_HASH = '%s';" % expected)]:
    assert old in t
    t = t.replace(old, new)
sys.stdout.write(t)
