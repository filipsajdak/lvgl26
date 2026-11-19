#!/usr/bin/env python3
"""Writes the page with the module embedded: one file, no server.
   make-page.py page/index.html demo.wasm script.json <sha256 of the host's last frame> > out.html"""
import base64, json, sys
page, wasm, script, expected = sys.argv[1:5]
t = open(page).read()
for old, new in [("const EMBEDDED = null;", "const EMBEDDED = '%s';" % base64.b64encode(open(wasm, 'rb').read()).decode()),
                 ("const SCRIPT = null;", "const SCRIPT = %s;" % json.dumps(json.load(open(script)))),
                 ("const EXPECTED_HASH = null;", "const EXPECTED_HASH = '%s';" % expected)]:
    assert old in t
    t = t.replace(old, new)
sys.stdout.write(t)
