Signal is the look of one family of eight devices. They are a thermostat, a smartwatch, a kitchen appliance, an e-bike display, a machine panel, an EV charger, a medical device and an instrument cluster. It is made to be drawn by a microcontroller and read at a glance.

## Principles

- **One number first.** Every screen has one reading that matters. Set it in `numeral-xl` or `numeral-l`, in `reading`, and let nothing else compete with it.
- **One loud colour.** `active` marks what the person chose or can do now: the primary action, the selected mode, progress. Use it once per screen where you can.
- **Normal has no colour.** A value in its normal range is `reading` on `ground`. There is no green. Colour appears when something needs attention, so that it is seen.
- **Square and solid.** Corners are `radius-0`. Shapes are filled blocks and bars on the 4 px module (`space-1`). No shadows, no gradients, no blur.

## Themes

- **Dark** is the theme of every indoor device and of anything with an OLED screen.
- **Day** is for screens read in sunlight: the EV charger and the e-bike display. The ground is white and the ink is black, at the highest contrast the screen can give.
- The two themes share every name, size and rule. In Day, `active` and `on-active` swap, so the primary action is a black block with lime text, and a `caution` block gets a `stroke-bar` border in `reading`.
- A device has one theme. Do not switch by the time of day unless the device has a light sensor.

## Colour

- Put screens on `ground` and group content on `surface`. Show a pressed control or the selected row with `surface-raised`.
- Set numbers and primary text in `reading`; units, labels and secondary text in `muted`.
- Draw tracks, separators and the normal band of a gauge in `line`. `line` is decoration: a control without a fill gets a `line-strong` border.
- Fill the primary action and the selected mode with `active` and set their text in `on-active`.
- Do not use `active` for a state of the machine, the vehicle or the patient. States use the alarm colours below.

## Alarms and states

A state is never colour alone. Each priority has a colour, a number of triangles and a word.

| Priority | Colour | Mark | Example |
|---|---|---|---|
| High: act now | `alarm` block, `on-alarm` text | three triangles | OCCLUSION |
| Medium: act soon | `caution` block, `on-caution` text | two triangles | HIGH PRESSURE |
| Low or information | `advice` text or outline | one triangle | SERVICE DUE |

- On the machine panel and the medical device, an alarm block is the only filled colour on the screen besides one `active` control.
- On the cluster and the e-bike display, follow the vehicle's own telltale colours for the lamps the law defines; use these tokens for everything else.

## Type

- Numbers and titles are Barlow Condensed (`display`); everything else is Barlow (`text`). Both are under the SIL Open Font Licence, so they may be embedded in a device.
- `numeral-2xl` (208 px): only the speed of a wide instrument cluster and the one reading of a large outdoor panel.
- `numeral-xl` (104 px): the one reading of a watch face, a thermostat or a small display. `numeral-l` (72 px): the reading of a tile. `numeral-m` (40 px): a second value.
- `title` and `label` are set in capitals with wide spacing: `TANK 2 LEVEL`, `ECO`. The spacing is 8 percent of the size, in whole pixels: 2 px for `title`, 1 px for `label`. `body` and `caption` are sentence case.
- A number always has its unit, in `muted`, one step smaller: `87 KM/H`, `4.8 bar`, `21.5°`.
- Decimals: write a leading zero (`0.5`), never a trailing one (`5`, not `5.0`), on the medical device above all.

## Shape and layout

- Lay everything out on `space-1`. Pad a tile with `space-4`, separate tiles with `space-6`, set the screen margin to `space-6` on small screens and `space-12` on large panels.
- A linear bar is `stroke-track` thick; its marker is a `stroke-bar` line that stands taller than the track. A ring on a round face is `stroke-ring` thick.
- A gauge shows its normal range as a `line` band inside the track, so that "outside normal" is seen without reading the number.
- Anything a finger uses is at least `target` in both directions, on every screen however small, with at least `space-2` between two targets. That is above the 24 px minimum and the 44 px enhanced level of WCAG 2.2 (target size). Where gloves are worn (machine panel, EV charger, e-bike) it is at least `target-glove`.
- Check the size in millimetres on the real panel too: `target` must come out at 9 mm or more. On a dense screen, raise the pixel size until it does.
- A `chip` shows a state and is never touched. If a mode can be chosen by touch, it is a button of `target` height, not a chip.
- A control looks like one: a filled block or a `line-strong` border. Text alone is never a control.
- A round display (`radius-face`) keeps text inside the middle 70 percent of its width; rings hug the edge.

## Motion

- Motion says what changed, and then stops. Nothing loops except an alarm.
- A value moves to its new place in 150 ms, a page slides in 250 ms, both starting fast and ending slowly. A high-priority alarm blinks; nothing else does.
- A device may turn motion off without losing meaning: every change is also visible when it is still.

## Words

- Short, in capitals for labels, with the unit. `START`, `HOLD`, `RANGE 42 KM`.
- Say what happened and what to do: `DOOR OPEN. CLOSE TO START.` No exclamation marks, no apologies.

## Icons

- Signal ships no icon set yet. Draw icons as strokes of `stroke-bar` weight with square ends on a 24 px grid, in the colour of the text beside them.
- Never use an icon without a word for an alarm or a mode.

## For the renderer

These rules keep a screen cheap for a microcontroller to draw.

- Fill, do not blend: no translucent layers over moving content.
- Keep `numeral-xl` and `numeral-l` in a font file of their own that holds only the digits, the degree sign, the colon, the point and the minus.
- A screen in 16-bit colour shows bands in a gradient, which is one more reason Signal has none.
- Redraw the number, not the screen: a reading sits in its own block so that only that block changes.
