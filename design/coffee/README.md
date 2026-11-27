# The coffee machine: the design the example builds

`mockup.html` is the specification of the coffee machine in `examples/coffee`: a 480 × 320 touch panel in Signal. The programs of the tutorial build these screens, and their tests check them against it. Open the page in a browser; it shows the live panel, every screen at 1 : 1, the review that led to it, and its motion.

The pictures the page shows are the files the device uses, in `examples/coffee/assets/`. The page reads them by relative paths, so open it from a clone of the repository: the steam mask is read by a script, which a browser allows from a local web server (`python3 -m http.server` at the repository root, then `/design/coffee/mockup.html`). Opened from a file, the page masks the steam by luminance instead. The page loads Barlow and Barlow Condensed from Google Fonts.

## The screens

| Screen | What it shows | Leaves to |
|---|---|---|
| Choose a drink (1 / 4) | Four tiles: a photo, the name and the price. The chosen tile fills with `active` and gets a tick | Next: Your cup. The gear: Service |
| Your cup (2 / 4) | Strength, 1 to 5, with its word (Mild ... Intense), and the size in mL within the drink's range, each between − and +. A summary: the photo, the name, the beans, the price | Back: Choose a drink. Brew: Brewing |
| Brewing (3 / 4) | A ring that is the progress, the seconds left as the numeral, the stage (grinding, extracting, finishing), and the drink, strength and size | Enjoy when done. Cancel: hold to cancel |
| Enjoy (4 / 4) | The drink's hero picture, with steam rising over a hot drink; the ready mark, the name, and when the panel returns to the drinks | New drink, or by itself after 10 s |
| Service | Rows that end in their control: Descale (days left, a chevron), Water hardness (3 of 5, a chevron), Brightness (− bar +), Eco mode (a switch) | Back: Choose a drink |

Decided after the mock-up (2026-11-26) and not drawn in it yet: Service gets a row "Display" that switches between Signal's Day and Dark themes in place.

## The rules

**Tokens for everything.** Every size, space, colour, font, stroke and duration is a token of Signal or of the coffee machine's own token file, or a formula of them. No number is written in a screen.

The layout, as the page writes it:

| What | Formula | At 480 × 320 |
|---|---|---|
| Margin left and right | `space-6` | 24 px |
| Gap between blocks | `space-3` | 12 px |
| Padding of a tile | `space-4` | 16 px |
| Header height | `target` | 48 px |
| Footer height | `target + space-4` | 64 px |
| Content height | `screen height − header − footer − gap` | 196 px |
| Thumbnail (tiles, summary) | `1.5 × target` | 72 px |
| Hero (Enjoy) | `3 × target` | 144 px |
| Brewing ring | `4 × target`, stroke `stroke-ring` | 192 px, 16 px |

- One pattern on every screen: the title top left, the step (dots and "2 / 4") top right, going back bottom left, going on bottom right.
- Every control is at least `target` (48 px) in both directions.
- The header and footer heights are named tokens of the coffee machine's token file (`header-height`, `footer-height`), as is the crema colour; Signal's tokens stay Signal's.

**Gestures.**

- Swipe right goes back, swipe left goes on, swipe down opens Service.
- Brewing takes no swipe: it is cancelled only by holding Cancel to the end, and a release before the end does nothing.

**Motion.** The page's names are `--motion-press`, `--motion-enter`, `--motion-settle`; Signal's tokens are `duration.press`, `duration.enter`, `duration.settle`, and the curves are `ease.out` and `ease.back`.

| Where | What moves | Tokens |
|---|---|---|
| Every step | The content slides in from the side it came from, each block 50 ms after the last | enter 220 ms, ease-out |
| Any control | A pressed tile or button shrinks a little and darkens | press 120 ms |
| Choose a drink | The tick pops on the chosen tile | enter, ease-back |
| Brewing | The ring fills, the numeral counts down, the stage changes | the brew time of the drink |
| Enjoy | The cup settles with a small overshoot; three steam wisps rise and fade in a loop; the mark pops; the name slides up | settle 480 ms, ease-back; the steam's loop 5.2 s |

- The steam is one mask, `steam.png`, used three times and tinted from a token, so it works on both themes. A frame player was compared and left out (the table on the page says why).

## Files

| File | What it is |
|---|---|
| `mockup.html` | The specification: every screen, live and at 1 : 1 |
| `panel.css` | The panel's stylesheet from before the review, with drink glyphs instead of photos and the crema tone for both themes. `mockup.html` does not read it: it carries its own panel styles |
| `../../examples/coffee/assets/<drink>-72.png`, `<drink>-144.png` | The four drinks as thumbnail and hero, cut out, with soft edges on any ground |
| `../../examples/coffee/assets/steam.png` | The steam mask: white smoke on black, 200 × 200, its brightness the mask |

The pictures were generated for this repository (`NOTICE.md`).
