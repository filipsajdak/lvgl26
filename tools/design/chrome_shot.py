#!/usr/bin/env python3
"""Takes a screenshot of a page with headless Chrome.

    python3 tools/design/chrome_shot.py page.html out.png 1920x720

Used by the other scripts in this folder. Standard library only.

Every call uses a fresh browser profile. A profile that is used twice clamps
the window to a part of the screen, and the picture then has the wrong size.
"""

import os
import pathlib
import shutil
import subprocess
import sys
import tempfile

MAC_CHROME = "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"


def chrome() -> str:
    """The Chrome binary: $CHROME, else the macOS default, else one on PATH."""
    found = os.environ.get("CHROME") or (MAC_CHROME if os.path.exists(MAC_CHROME) else None)
    found = found or shutil.which("google-chrome") or shutil.which("chromium")
    if not found:
        sys.exit("chrome_shot: no Chrome found; set CHROME to its path")
    return found


def parse_size(text: str) -> tuple[int, int]:
    """'1920x720' as (1920, 720)."""
    width, height = text.lower().split("x")
    return int(width), int(height)


def shot(url: str, out: str, size: tuple[int, int], *, webgl: bool = False,
         transparent: bool = False, wait_ms: int = 0) -> None:
    """Writes a PNG of `url` at `size`. `webgl` renders with SwiftShader."""
    profile = tempfile.mkdtemp(prefix="lvgl26-shot-")
    args = [chrome(), "--headless=new", "--hide-scrollbars", f"--user-data-dir={profile}",
            f"--window-size={size[0]},{size[1]}", f"--screenshot={out}"]
    args += ["--enable-unsafe-swiftshader", "--use-angle=swiftshader"] if webgl else ["--disable-gpu"]
    if transparent:
        args.append("--default-background-color=00000000")
    if wait_ms:
        args.append(f"--timeout={wait_ms}")
    try:
        subprocess.run(args + [url], check=True, capture_output=True)
    finally:
        shutil.rmtree(profile, ignore_errors=True)
    if not os.path.exists(out):
        sys.exit(f"chrome_shot: Chrome wrote no picture for {url}")


def main() -> None:
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    page = pathlib.Path(sys.argv[1]).resolve().as_uri()
    shot(page, sys.argv[2], parse_size(sys.argv[3]))


if __name__ == "__main__":
    main()
