# 3D, shaders, shadows and blur

What LVGL offers for 3D and effects, how we plan to serve microcontrollers, and how the 3D models of the showcase were made. Marks follow [README.md](README.md).

## 3D in LVGL

Checked in both pinned trees, v9.6.0 and the v10 pin. `source` 2026-11-15.

| Fact | Detail |
|---|---|
| A glTF viewer widget exists and is public | `include/lvgl/widgets/lv_gltf.h`; enable `LV_USE_GLTF`, which needs `LV_USE_3DTEXTURE` |
| A widget shows any OpenGL texture | `include/lvgl/widgets/lv_3dtexture.h` |
| Both need OpenGL ES | `LV_USE_OPENGLES`, driven through GLFW or EGL |
| There is no public "shader on a widget" API | The shader manager is internal to the OpenGL driver |
| Draw units exist for OpenGL ES and NanoVG | `LV_USE_DRAW_OPENGLES`, `LV_USE_DRAW_NANOVG` |

What it means for us:

- A custom shader effect is rendered into a texture with our own GL code and shown with `lv_3dtexture`.
- 3D and shaders belong on devices with a graphics processor.
- macOS has no native OpenGL ES, so the path may only run on Linux or in CI. `recalled`, not tried.

## Microcontrollers

A microcontroller without a graphics processor draws no real 3D. The plan, not built:

1. The creator places one 3D element with its animation in the screen.
2. For a target with a graphics processor, lvgl26 uses LVGL's live glTF viewer.
3. For a microcontroller target, the build renders the same model and animation to a frame sequence and embeds it.
4. The build renders with LVGL's own viewer, so both tiers show the same pixels.

A 4 s loop of 48 frames at 604 by 340 px packed to 200 KB as animated WebP in our test. `measured` 2026-11-15. That is a design preview, not a firmware format.

## When 3D is worth it

A 3D model must do what a picture cannot: move its camera to the part that matters, highlight that part, or show light. If it does none of these, use a picture.

## Generating 3D models

Models can be generated from text or from an image through fal. `source` 2026-11-15, the fal catalogue and five runs.

| Model on fal | Kind |
|---|---|
| `fal-ai/hyper3d/rodin/v2.5/text-to-3d` | Text to 3D; the one we used |
| `tripo3d/h3.1/text-to-3d`, `tripo3d/h3.1/image-to-3d` | Text or image to 3D |
| `fal-ai/hunyuan-3d/v3.1/pro/text-to-3d`, `.../image-to-3d` | Text or image to 3D |
| `meshy/v7.1/text-to-3d`, `meshy/v7.1/image-to-3d` | Text or image to 3D, with PBR maps |
| `fal-ai/trellis-2` | Image to 3D |

What we learned from Rodin 2.5:

- The rate shown to us was 0.40 USD per generation. Check it again before a run.
- Useful inputs: `quality_mesh_option: "20K Triangle"`, `material: "PBR"`, `geometry_file_format: "glb"`, `texture_delight: true`, a fixed `seed`.
- A result is one mesh of about 19,000 triangles with three JPEG textures, 1 to 2.7 MB as GLB.
- The mesh has no named parts. A part is highlighted by its region in space, not by name.
- The longest side spans about -0.95 to 0.95. For the car, the front points to +z, left is +x and up is +y.
- A generation takes one to two minutes.

Prompts:

| Subject | Result |
|---|---|
| "A compact modern electric hatchback car, generic design with no brand marks…, uniform matte light grey clay material, single object" | Good at the first try |
| "An industrial stainless steel mixing tank on four legs…, agitator motor on top, …a centrifugal pump at the base" | Good at the first try |
| "A modern electric city bicycle…, battery integrated in the down tube" | Good; the spokes are missing |
| "An electric vehicle fast-charging connector plug with handle, combined AC and DC pin layout…" | A shapeless block |
| "An electric car charging gun: a pistol-shaped handheld plug with a curved grip handle and a trigger latch on top, a round barrel nose with a flat front face…" | Reads as a charging gun; the pin face is generic |

Lessons:

1. Describe the silhouette in everyday words. A technical name such as "connector plug" gave a block.
2. Ask for "single object", "no background", "no brand marks or text" and one matte material.
3. A generated model is generic. Replace it before it explains a real product or a real standard.
4. Look at every result before using it. One in five failed.

## Shadows

No source was found for a rule that ties the blur of a shadow to the height of the object. `summary` 2026-11-14.

- Rive's tutorial keys the shadow as its own group: its horizontal scale grows as the ball reaches the ground, and its opacity is keyed separately. [Rive](https://rive.app/community/doc/bouncing-ball/doc3R2iYi7Cs)
- A perception demo shows that the position of the shadow decides how high the ball seems. [Kersten Lab](https://kerstenlab.psych.umn.edu/demos/shadows)

What we do in the bounce demo: a dark oval under a clear widget with backdrop blur. The blur radius grows and the oval fades as the ball rises.

## Blur in LVGL

- LVGL 9.5 has a blur draw descriptor with `blur_radius`, the intensity, and `corner_radius`. `summary` 2026-11-14, [lv_draw_blur.h](https://docs.lvgl.io/9.5/API/draw/lv_draw_blur_h.html).
- A third-party example says blur needs a GPU back end and may do nothing on the SDL host. `summary` 2026-11-14. Our bounce demo shows backdrop blur on the SDL host with the software renderer, on a branch that is not merged.
- An old forum thread says only box blur was available for speed, and that two passes look close to a Gaussian blur. It refers to the canvas blur, not the style property.
- No benchmark of blur on the software renderer was found. Measure on the target before relying on it.
