#!/usr/bin/env python3
"""Renders one artboard of a Claude Design canvas to a PNG, to look at it.

    python3 tools/design/preview_artboard.py Main.dc.html out.png 1920x720 \\
        --fonts design/signal/fonts --asset <blob id>=car.png

An artboard file needs the canvas runtime to run. This script takes the markup
between <x-dc> and </x-dc>, which is plain HTML with inline styles, and wraps
it in a page of its own. It suits static artboards; holes such as {{name}} are
not filled.

  --fonts DIR     .ttf files to load: BarlowCondensed-Bold, BarlowCondensed-SemiBold,
                  Barlow-Medium (the families of the Signal design system)
  --asset ID=FILE replaces /_blob/ID in the markup with a local file; repeatable

Standard library only. Needs Chrome (see chrome_shot.py).
"""

import argparse
import pathlib
import re
import tempfile

import chrome_shot

FACES = [("Barlow Condensed", 700, "BarlowCondensed-Bold.ttf"),
         ("Barlow Condensed", 600, "BarlowCondensed-SemiBold.ttf"),
         ("Barlow", 500, "Barlow-Medium.ttf")]


def body_of(artboard: str) -> str:
    """The markup of the artboard, without the canvas wrapper and its <helmet>."""
    start, end = artboard.index("<x-dc>") + len("<x-dc>"), artboard.index("</x-dc>")
    return re.sub(r"<helmet>.*?</helmet>", "", artboard[start:end], flags=re.S)


def font_faces(fonts: pathlib.Path | None) -> str:
    if fonts is None:
        return ""
    rule = '@font-face{{font-family:"{0}";font-weight:{1};src:url("{2}")}}'
    return "\n".join(rule.format(family, weight, (fonts / name).resolve().as_uri())
                     for family, weight, name in FACES if (fonts / name).exists())


def with_assets(markup: str, assets: list[str]) -> str:
    for pair in assets:
        blob, path = pair.split("=", 1)
        markup = markup.replace("/_blob/" + blob, pathlib.Path(path).resolve().as_uri())
    return markup


def page(artboard: str, fonts: pathlib.Path | None, assets: list[str]) -> str:
    style = font_faces(fonts) + "\nhtml,body{margin:0;background:#000}"
    return ('<!doctype html><html><head><meta charset="utf-8"><style>' + style +
            "</style></head><body>" + with_assets(body_of(artboard), assets) + "</body></html>")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("artboard")
    parser.add_argument("out")
    parser.add_argument("size")
    parser.add_argument("--fonts", type=pathlib.Path)
    parser.add_argument("--asset", action="append", default=[])
    args = parser.parse_args()
    html = page(pathlib.Path(args.artboard).read_text(), args.fonts, args.asset)
    with tempfile.NamedTemporaryFile("w", suffix=".html", delete=False) as file:
        file.write(html)
    chrome_shot.shot(pathlib.Path(file.name).as_uri(), args.out, chrome_shot.parse_size(args.size))
    pathlib.Path(file.name).unlink()


if __name__ == "__main__":
    main()
