# Design scripts

Scripts for the design work around lvgl26: looking at a screen design, inspecting a 3D model, and rendering a 3D showpiece. They are design aids. The library and its tests do not use them.

| Script | What it does |
|---|---|
| `chrome_shot.py` | Takes a screenshot of a page with headless Chrome. The others use it. |
| `preview_artboard.py` | Renders one artboard of a Claude Design canvas to a PNG. |
| `glb.py info` | Prints triangles, vertices, textures and bounds of a `.glb` model. |
| `glb.py embed` | Rewrites a `.glb` as one glTF text file with its data inside. |
| `car_scene.py` | Renders the cluster's 3D car as a still, or as an animated WebP. |

## What they need

- Python 3.10 or newer; the standard library only.
- Google Chrome. Set `CHROME` to its path if it is not in the default place.
- `img2webp` from libwebp, for `car_scene.py frames`.
- A network connection for `car_scene.py`: it loads three.js 0.160.0 from jsDelivr.

## Examples

Look at an artboard:

```sh
python3 tools/design/preview_artboard.py Main.dc.html cluster.png 1920x720 \
    --fonts site/media/fonts --asset 28a5469261d092496aebf62e98eb9709=car_alert.png
```

Inspect a model:

```sh
python3 tools/design/glb.py info car.glb
```

Render the showpiece at the moment the mark pulses, at double size:

```sh
python3 tools/design/car_scene.py still car.glb car_alert.png --t 0.56 --size 1208x680
```

Render the whole loop as an animation:

```sh
python3 tools/design/car_scene.py frames car.glb car_motion.webp --n 48 --size 604x340
```

## Things that cost time to find

1. Headless Chrome clamps the window when a browser profile is used twice. The picture then has the wrong size. `chrome_shot.py` makes a fresh profile for every picture.
2. An artboard file does not run without the canvas runtime. `preview_artboard.py` takes the markup out of it. That works for static artboards only.
3. A generated model is one mesh with no named parts. A part is found and coloured by its place in space.
4. Rendering in Chrome is in software. A frame takes about 4 s, so 48 frames take about 3 minutes.

The research behind the designs is in [docs/research](../../docs/research/README.md).
