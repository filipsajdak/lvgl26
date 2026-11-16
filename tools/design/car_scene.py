#!/usr/bin/env python3
"""Renders the cluster's 3D showpiece from a car model: one still, or an animation.

    python3 tools/design/car_scene.py still  car.glb out.png  --t 0.56 --size 1208x680
    python3 tools/design/car_scene.py frames car.glb out.webp --n 48   --size 604x340

The scene is the one of the instrument cluster design: the car in its lane, the
neighbouring lanes, a car ahead, the low beam as a pool of light on the road.
`--t` runs from 0 to 1 over one loop:

    0.00 to 0.25  the driving view
    0.25 to 0.50  the camera travels to the rear left wheel, which turns to the caution colour
    0.50 to 0.85  it holds, and a ring pulses on the road around the wheel
    0.85 to 1.00  the camera travels back

The model is one mesh with no named parts, so the wheel is found by its place:
the lowest vertices on the left at the rear. The car's front must point to +z,
its left to +x and up to +y, as the models from Rodin 2.5 do.

It is a template for other showpieces: copy it and change the scene.

Needs Chrome, a network connection for three.js 0.160.0 from jsDelivr, and
`img2webp` (from libwebp) for `frames`. Rendering is in software, about 4 s a frame.
"""

import argparse
import base64
import pathlib
import subprocess
import tempfile

import chrome_shot

PAGE = r"""<!doctype html><html><head><meta charset="utf-8">
<style>html,body{margin:0;background:transparent}canvas{display:block}</style>
<script type="importmap">{"imports":{"three":"https://cdn.jsdelivr.net/npm/three@0.160.0/build/three.module.js","three/addons/":"https://cdn.jsdelivr.net/npm/three@0.160.0/examples/jsm/"}}</script>
</head><body><script type="module">
import * as THREE from 'three';
import {GLTFLoader} from 'three/addons/loaders/GLTFLoader.js';
const Q = Object.fromEntries(location.hash.slice(1).split('&').map(s => s.split('=')));
const W = +Q.w, H = +Q.h, t = +Q.t;
const ease = x => x < 0 ? 0 : x > 1 ? 1 : x * x * (3 - 2 * x), mix = (a, b, k) => a + (b - a) * k;
const s = t < 0.85 ? ease((t - 0.25) / 0.25) : 1 - ease((t - 0.85) / 0.15);   // 0 driving view, 1 at the wheel
const DRIVE = {cam: [0, 2.2, -5.5], look: [0, 0, 3.1], fov: 30};
const WHEEL = {cam: [2.5, 1.7, -3.3], look: [0.2, -0.12, -0.15], fov: 19.3};
const GROUND = '#8a8d98', OTHER = '#7B7E8A', CAUTION = '#FFC531';
const r = new THREE.WebGLRenderer({antialias: true, alpha: true, preserveDrawingBuffer: true});
r.setSize(W, H); r.setClearColor(0x000000, 0); document.body.appendChild(r.domElement);
const g = await new GLTFLoader().parseAsync(Uint8Array.from(atob("__GLB__"), c => c.charCodeAt(0)).buffer, '');
const scene = new THREE.Scene();

// A matte copy of the model, centred, with the rear left wheel tinted by `tint` (0 to 1).
function car(base, tint) {
  const o = g.scene.clone(true), box = new THREE.Box3().setFromObject(o), c = box.getCenter(new THREE.Vector3());
  o.position.sub(c); o.updateMatrixWorld(true);
  const miny = box.min.y - c.y, maxz = box.max.z - c.z, v = new THREE.Vector3();
  let wx = 0, wz = 0, n = 0;
  o.traverse(m => { if (!m.isMesh) return; const p = m.geometry.attributes.position;
    for (let i = 0; i < p.count; i++) { v.fromBufferAttribute(p, i).applyMatrix4(m.matrixWorld);
      if (v.y < miny + 0.02 && v.x > 0.15 && v.z < -0.2) { wx += v.x; wz += v.z; n++; } } });
  wx /= n; wz /= n; const wy = miny + 0.15;
  const plain = new THREE.Color(base), marked = new THREE.Color(CAUTION).lerp(plain, 1 - tint);
  o.traverse(m => { if (!m.isMesh) return; const ge = m.geometry.clone(); m.geometry = ge;
    const p = ge.attributes.position, col = new Float32Array(p.count * 3);
    for (let i = 0; i < p.count; i++) { v.fromBufferAttribute(p, i).applyMatrix4(m.matrixWorld);
      const hit = tint > 0 && Math.abs(v.x - wx) < 0.15 && Math.hypot(v.y - wy, v.z - wz) < 0.165;
      const k = hit ? marked : plain; col[i * 3] = k.r; col[i * 3 + 1] = k.g; col[i * 3 + 2] = k.b; }
    ge.setAttribute('color', new THREE.BufferAttribute(col, 3));
    m.material = new THREE.MeshStandardMaterial({vertexColors: true, roughness: 0.9, metalness: 0}); });
  return {o, miny, maxz, wx, wz};
}

const me = car(GROUND, s); scene.add(me.o);
const ahead = car(OTHER, 0); ahead.o.position.z += 16; scene.add(ahead.o);
for (const [x, k] of [[-2.49, 0.45], [-0.83, 1], [0.83, 1], [2.49, 0.45]]) {      // lane lines; they fade at the wheel
  const line = new THREE.Mesh(new THREE.BoxGeometry(0.035, 0.004, 80),
    new THREE.MeshBasicMaterial({color: OTHER, transparent: true, opacity: (1 - s) * k}));
  line.position.set(x, me.miny, 34); scene.add(line);
}
{ // the low beam: a pool of light on the road ahead
  const y = me.miny + 0.004, z0 = me.maxz + 0.02, a = 0.34 * (1 - s), ge = new THREE.BufferGeometry();
  ge.setAttribute('position', new THREE.BufferAttribute(new Float32Array(
    [-0.40, y, z0, 0.40, y, z0, 1.45, y, z0 + 7.5, -1.45, y, z0 + 7.5]), 3));
  ge.setAttribute('color', new THREE.BufferAttribute(new Float32Array([1, 1, 1, a, 1, 1, 1, a, 1, 1, 1, 0, 1, 1, 1, 0]), 4));
  ge.setIndex([0, 1, 2, 0, 2, 3]);
  scene.add(new THREE.Mesh(ge, new THREE.MeshBasicMaterial(
    {vertexColors: true, transparent: true, depthWrite: false, side: THREE.DoubleSide})));
}
if (t >= 0.5 && t < 0.85) { // the mark that pulses on the road around the wheel, twice
  const p = ((t - 0.5) / 0.35 * 2) % 1, outer = 0.20 + 0.16 * p;
  const ring = new THREE.Mesh(new THREE.RingGeometry(outer - 0.03, outer, 64), new THREE.MeshBasicMaterial(
    {color: CAUTION, transparent: true, opacity: (1 - p) * 0.9, depthWrite: false, side: THREE.DoubleSide}));
  ring.rotation.x = -Math.PI / 2; ring.position.set(me.wx, me.miny + 0.006, me.wz); scene.add(ring);
}
scene.add(new THREE.HemisphereLight(0xffffff, 0x202028, 1.7));
const sun = new THREE.DirectionalLight(0xffffff, 2.2); sun.position.set(1.5, 4, -2); scene.add(sun);
const cam = new THREE.PerspectiveCamera(mix(DRIVE.fov, WHEEL.fov, s), W / H, 0.1, 200);
cam.position.set(...DRIVE.cam.map((d, i) => mix(d, WHEEL.cam[i], s)));
const turn = 1 - Math.pow(1 - s, 2.5);   // the camera turns to the car before it has finished travelling
cam.lookAt(...DRIVE.look.map((d, i) => mix(d, WHEEL.look[i], turn)));
r.render(scene, cam);
</script></body></html>
"""


def write_page(glb: str) -> pathlib.Path:
    """The scene as one HTML file with the model inside it."""
    model = base64.b64encode(pathlib.Path(glb).read_bytes()).decode()
    with tempfile.NamedTemporaryFile("w", suffix=".html", delete=False) as file:
        file.write(PAGE.replace("__GLB__", model))
    return pathlib.Path(file.name)


def render(page: pathlib.Path, t: float, size: tuple[int, int], out: str) -> None:
    url = f"{page.as_uri()}#w={size[0]}&h={size[1]}&t={t}"
    chrome_shot.shot(url, out, size, webgl=True, transparent=True, wait_ms=20000)


def frames(page: pathlib.Path, count: int, size: tuple[int, int], out: str, fps: int) -> None:
    with tempfile.TemporaryDirectory() as folder:
        names = [f"{folder}/f{i:03d}.png" for i in range(count)]
        for i, name in enumerate(names):
            render(page, i / count, size, name)
        subprocess.run(["img2webp", "-loop", "0", "-d", str(1000 // fps), "-lossy", "-q", "82", "-m", "4",
                        *names, "-o", out], check=True, capture_output=True)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("mode", choices=["still", "frames"])
    parser.add_argument("glb")
    parser.add_argument("out")
    parser.add_argument("--t", type=float, default=0.0)
    parser.add_argument("--n", type=int, default=48)
    parser.add_argument("--fps", type=int, default=12)
    parser.add_argument("--size", default="604x340")
    args = parser.parse_args()
    page, size = write_page(args.glb), chrome_shot.parse_size(args.size)
    try:
        if args.mode == "still":
            render(page, args.t, size, args.out)
        else:
            frames(page, args.n, size, args.out, args.fps)
    finally:
        page.unlink()


if __name__ == "__main__":
    main()
