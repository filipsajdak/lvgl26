#!/usr/bin/env python3
"""Inspects a binary glTF model, or rewrites it as one text file.

    python3 tools/design/glb.py info car.glb [more.glb ...]
    python3 tools/design/glb.py embed car.glb car.gltf

`info` prints the triangle and vertex counts, the textures and the bounds.
`embed` writes glTF 2.0 as JSON with the binary data inside it as a data URI,
for a store that takes text files but not .glb.

Standard library only.
"""

import base64
import json
import struct
import sys

GLB_MAGIC, JSON_CHUNK, BIN_CHUNK = b"glTF", 0x4E4F534A, 0x004E4942


def read_glb(path: str) -> tuple[dict, bytes]:
    """The JSON part and the binary part of a .glb file."""
    data = open(path, "rb").read()
    if data[:4] != GLB_MAGIC:
        sys.exit(f"glb: {path} is not a binary glTF file")
    json_length, json_kind = struct.unpack("<II", data[12:20])
    if json_kind != JSON_CHUNK:
        sys.exit(f"glb: {path} does not start with a JSON chunk")
    document = json.loads(data[20:20 + json_length])
    rest = 20 + json_length
    if rest >= len(data):
        return document, b""
    bin_length, bin_kind = struct.unpack("<II", data[rest:rest + 8])
    if bin_kind != BIN_CHUNK:
        sys.exit(f"glb: {path} has no binary chunk after the JSON")
    return document, data[rest + 8:rest + 8 + bin_length]


def counts(document: dict) -> tuple[int, int]:
    """(triangles, vertices) over all meshes."""
    accessors, triangles, vertices = document["accessors"], 0, 0
    for mesh in document.get("meshes", []):
        for primitive in mesh["primitives"]:
            vertices += accessors[primitive["attributes"]["POSITION"]]["count"]
            if "indices" in primitive:
                triangles += accessors[primitive["indices"]]["count"] // 3
    return triangles, vertices


def textures(document: dict) -> list[str]:
    views = document.get("bufferViews", [])
    return [f'{image.get("mimeType")} {views[image["bufferView"]]["byteLength"]} bytes'
            for image in document.get("images", []) if "bufferView" in image]


def info(path: str) -> None:
    document, binary = read_glb(path)
    triangles, vertices = counts(document)
    first = document["meshes"][0]["primitives"][0]["attributes"]["POSITION"]
    bounds = document["accessors"][first]
    print(f"{path}: {len(binary)} bytes of data, {len(document.get('meshes', []))} mesh(es), "
          f"{triangles} triangles, {vertices} vertices")
    print(f"  textures: {textures(document) or 'none'}")
    print(f"  bounds: min {bounds.get('min')} max {bounds.get('max')}")


def embed(source: str, target: str) -> None:
    document, binary = read_glb(source)
    buffers = document.get("buffers", [])
    if len(buffers) != 1 or "uri" in buffers[0]:
        sys.exit(f"glb: {source} must have exactly one buffer, held in the file")
    payload = base64.b64encode(binary[:buffers[0]["byteLength"]]).decode()
    buffers[0]["uri"] = "data:application/octet-stream;base64," + payload
    with open(target, "w") as file:
        json.dump(document, file, separators=(",", ":"))


def main() -> None:
    if len(sys.argv) >= 3 and sys.argv[1] == "info":
        for path in sys.argv[2:]:
            info(path)
    elif len(sys.argv) == 4 and sys.argv[1] == "embed":
        embed(sys.argv[2], sys.argv[3])
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
