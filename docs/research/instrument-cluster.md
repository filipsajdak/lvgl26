# Instrument cluster

What research says about a digital instrument cluster, and the layout that follows. Researched on 2026-11-15. Marks follow [README.md](README.md).

## Findings

| Finding | What it means for the layout | Source | Mark |
|---|---|---|---|
| A centrally placed speedometer gave the best reaction time and accuracy, digital or analog. Position mattered more than display type. | The speed sits alone in the centre. | [Applied Sciences 2025](https://www.mdpi.com/2076-3417/15/4/1879) | `summary` |
| A numeric readout is read fastest for an exact value. An analog gauge is better for trend and for "over the limit". | Show the speed as a number and put the limit beside it. | same, and [SAE 810826](https://saemobilus.sae.org/papers/ergonomics-electronic-displays-810826) | `summary` |
| Reading an analog speedometer takes about 0.5 to 1.2 s. | A glance is short; one number per glance. | cited in the 2025 paper | `summary` |
| The steering wheel can hide up to half of a cluster, as often as 47 percent of the time. | Critical items go to the top centre. Corners hold the least important items. | A US patent: [10093182](https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/10093182) or [12280667](https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/12280667). The summary did not say which. | `summary` |
| Obscuration must be checked early, so that critical displays are not hidden. SAE J1050 is the procedure and J941 defines the eye positions. | Check the real panel behind the real wheel. | [SAE 750359](https://saemobilus.sae.org/papers/methods-determining-unobscured-areas-instrument-panels-750359) | `summary` |
| NHTSA's voluntary guidelines: a single glance away from the road of 2 s or less, and 12 s in total for a task. Displays sit close to the line of sight. | An alert is read in well under 2 s. | [US DOT](https://www.transportation.gov/briefing-room/us-dot-releases-guidelines-minimize-vehicle-distractions) | `summary` |
| Telltales stay visible at all times and keep their place. | A fixed row of telltales. An unlit lamp keeps its slot. | [ustwo](https://ustwo.com/blog/ustwo-reimagines-the-in-car-cluster/) | `summary` |
| Telltale colours: red for danger, yellow for a fault, green for direction indicators and passing beam, blue for driving beam. | Two lamp colours beside the alarm colours. | [EU annex](https://legislation.gov.uk/eur/2014/3/annex/1/annex/VIII/adopted), indicative for [UN R121](https://www.mlit.go.jp/jidosha/un/UN_R121.pdf) | `summary` |
| Symbols not listed in UN R121 should follow ISO 2575. | Use the standard symbols in a product. Ours are simplified drawings. | UN R121 | `summary` |
| ISO/TS 16951:2021 ranks on-board messages by criticality and urgency. It does not apply to legally required messages. | A medium-priority alert takes the context zone. A high-priority alert may take the centre. | [ISO](https://www.iso.org/standard/81103.html) | `summary` |
| A low tyre pressure lamp is required. Showing which tyre is believed to be optional. | The lamp is always there. The car view is a supplement. | [FMVSS 138 final rule](https://www.federalregister.gov/documents/2005/04/08/05-6741/federal-motor-vehicle-safety-standards-tire-pressure-monitoring-systems-controls-and-displays) | `recalled` |
| ISO 15008:2017 sets legibility for driver displays: character height, contrast, colour, viewing angle. A fourth edition is in draft. | See the text size rule in [device-ui.md](device-ui.md). | [ISO 15008](https://standards.iteh.ai/catalog/standards/iso/d07adfe7-f551-4557-bb09-7236d8491cf6/iso-15008-2017) | `summary` |

## What was wrong with the first design

1. The speed sat left of centre, one of four equal columns.
2. The alert was in three places: a strip at the bottom, its text 1,400 px away, and a box by the car.
3. Labels and captions were 16 px, about 8 minutes of arc at 750 mm.
4. Power, at 104 px, competed with the speed.
5. There were no telltales.
6. The gear and the clock sat in the top corners at the weight of everything else.
7. The car was a small picture that did nothing a picture could not.

## The layout that follows

| Place | What goes there | Size |
|---|---|---|
| Centre | The speed, with the limit and the cruise speed beside it | 208 px, then 40 px |
| Top centre | Telltales, each in its fixed place | 40 px |
| One side | The context zone: the 3D showpiece, and the alert that takes it over | 40 px for the value that matters |
| Other side | Energy: range and battery first, power below | 72 px, then 40 px |
| Bottom centre | The gear | 24 px |
| Corners | Time, temperature, trip, odometer, drive mode | 24 px, muted |

Rules:

1. An alert keeps its three parts together. The block says what. The showpiece under it shows where, with the value beside the part. One line says what to do.
2. Nothing appears twice, except a state and its telltale.
3. Nothing is smaller than 24 px.
4. A value in its normal range has no colour.

## What the 3D car does

A 3D model must do what a picture cannot. In the cluster it does three things:

- It highlights the affected wheel in the caution colour.
- Its camera travels from the driving view to that wheel when the alert is raised, and back.
- It shows light: the low beam is a pool of light on the road, matching the lit telltale.

The driving view shows the car in its lane, the neighbouring lanes and the car ahead.

## Not confirmed

- The character heights of ISO 15008. The standard is paywalled.
- Whether UN R141 or FMVSS 138 require the lamp to name the tyre.
- The full colour table of UN R121. The colours above come from a related EU text.
- Whether the layout survives a real steering wheel. It has not been checked on hardware.
