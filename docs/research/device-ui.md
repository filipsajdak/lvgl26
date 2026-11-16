# Device UI: what drives each screen

What research says about the eight device classes of the showcase, and the rules that follow. The cluster has its own file, [instrument-cluster.md](instrument-cluster.md). Marks follow [README.md](README.md).

Most claims here are `summary`: a search engine's summary of a vendor guide or blog. Read the standard itself before a product relies on one.

## Per device

| Device | What drives the design | Source | Mark |
|---|---|---|---|
| Thermostat | It is read from across a room. The interface should recede and show a result, not a menu. Intelligence runs on the device. | [ambient computing](https://developex.com/blog/ambient-computing-in-consumer-electronics/) | `summary` 2026-11-15 |
| Smartwatch | Design for the smallest round screen first, 192 dp. Show at most two levels of hierarchy. Power is the main limit. Round corners clip content, so keep a safe area. | [Wear OS](https://developer.android.com/design/ui/wear/guides/foundations/adaptive-design), [wearable trends](https://www.mikroe.com/blog/wearable-technology-trends) | `summary` 2026-11-15 |
| Kitchen appliance | Touch replaces buttons. Fewer, simpler screens reduce errors. | [consumer electronics 2026](https://www.opd-design.com/consumer-electronics-design-trends-2026/) | `summary` 2026-11-15 |
| Machine panel | A neutral base, colour only for alarms and abnormal states. Live values look different from labels and carry their unit. | [ISA-101 overview](https://industrialmonitordirect.com/blogs/knowledgebase/isa-101-high-performance-hmi-design-principles-color-strategy) | `summary` 2026-11-15 |
| EV charger | In the EU a public charger of 50 kW or more shows the price before a session. See below. | [AFIR guide](https://www.ampeco.com/guides/afir-compliance-guide-charge-point-operators/) | `summary` 2026-11-15 |
| Medical device | Alarm priority is shown by colour and by a mark. See below. | [IEC 60601-1-8 notes](https://standards.har-el.com/Projects/181701/60601-1-8/html/A2SubCl-6-3-2-2-Visual-Characteristics.htm) | `summary` 2026-11-15 |
| E-bike display | Not researched on its own. It shares glove use and sunlight with the charger. | | |

## Machine panel: ISA-101

- ISA-101 is the vendor-neutral lifecycle standard for process HMIs. It builds on the high-performance HMI method of the ASM Consortium. `summary` 2026-11-15.
- It does not mandate hex values. It requires a structured, level-based palette that designers and operators agree on.
- A study guide cites a background grey of `#D0D0D0`. That is not confirmed against the standard.
- Guidance for modern touch panels was thin in the results. Vendor style guides are the next place to look.

## Medical alarms: IEC 60601-1-8

| Priority | Colour | Mark |
|---|---|---|
| High | Red, or on a red background | Three triangles |
| Medium | Yellow | Two triangles |
| Low | Cyan is allowed | One triangle |

- The triangles are a suggestion of the standard, not a requirement. `summary` 2026-11-15.
- An alarm must be legible at 4 m, and at 1 m or from the operator's position.
- The flash rate of visual alarms was not found. The timing figures in the results apply to audible alarms.

## EV charger: AFIR price display

- At 50 kW and above the ad hoc price is per kWh and is shown before a session starts. `summary` 2026-11-15.
- The price parts appear in a fixed order: per kWh, per minute, per session, then any other.
- Sources differ on where: on the charger's display, or also on a sticker.
- At 50 kW and above a contactless card or NFC reader is required. Below 50 kW a QR code is enough.
- Sunlight readability and outdoor brightness were not found.

More: [Schneider on AFIR](https://blog.se.com/infrastructure-and-grid/automotive-mobility/2024/04/30/afir-how-can-ev-charge-point-operators-comply-with-the-eus-new-payment-requirements/), [Open Charge Alliance](https://openchargealliance.org/wp-content/uploads/2024/01/AFIR-OCPP.pdf).

## Touch targets

| Rule | Size | Source | Mark |
|---|---|---|---|
| WCAG 2.2, target size minimum (2.5.8, level AA) | 24 by 24 CSS px | [touch target spec](https://specification.website/spec/accessibility/touch-target-size/) | `summary` 2026-11-15 |
| WCAG 2.2, target size enhanced (2.5.5, level AAA) | 44 by 44 CSS px | | `recalled` |
| Apple | 44 pt | | `summary` 2026-11-15 |
| Material Design | 48 dp, about 9 mm on any screen | | `summary` 2026-11-15 |
| MIL-STD-1472, bare finger | 15 mm square | [Springer chapter](https://springerlink.bibliotecabuap.elogim.com/content/pdf/10.1007/978-3-319-40406-6_7) | `summary` 2026-11-15 |
| MIL-STD-1472, gloves | 20 mm square | same | `summary` 2026-11-15 |

- Vendor blogs give 19 to 40 mm for gloves and cite ISO 9241-9, IEC 61131 and ISA-101. Those attributions look doubtful. Do not repeat them.
- Manufacturer recommendations are often smaller than MIL-STD-1472.

Rule in Signal: anything a finger uses is at least 48 px and 9 mm, with 8 px between targets. Where gloves are worn it is at least 80 px, about 20 mm.

## Text size by viewing distance

Text is sized for the eye. The method:

1. Take the viewing distance `d` and the visual angle `a` the text needs.
2. The capital height is `d * tan(a)`.
3. Divide by the pixel pitch of the panel, then by the cap-height ratio of the font, to get the font size in px.

Worked for a 12.3-inch 1920 by 720 panel at 750 mm. `measured` 2026-11-15, a calculation with a cap-height ratio of 0.7 assumed for Barlow Condensed.

| Font size | Capital height | Visual angle |
|---|---|---|
| 16 px | 1.7 mm | 7.8 minutes of arc |
| 24 px | 2.6 mm | 11.7 minutes of arc |
| 40 px | 4.3 mm | 19.6 minutes of arc |

The thresholds are `recalled`: ISO 15008 is remembered as 12 minutes of arc minimum, 16 acceptable and 20 recommended. The standard is paywalled and two searches did not confirm the figures.

Rule in Signal, for a screen read from an arm's length or more: nothing is smaller than 24 px. What is read in one glance is 40 px or larger.

## Trends, for context

- Physical buttons are giving way to touch with haptic feedback. `summary` 2026-11-15.
- Voice and gesture supplement touch where touch is impractical.
- Wearables act as controllers for other devices.
- Matter is the connectivity layer for cross-brand control in the home.

These come from vendor blogs and are directional only.

## Not found

- Glance time and information density for small consumer screens.
- A design-token architecture across device classes.
- Outdoor luminance for chargers.
- The visual flash rate of medical alarms.
