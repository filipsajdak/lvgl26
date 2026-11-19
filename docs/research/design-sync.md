# Checking and syncing Signal's design from Claude Design

Research written 2026-11-19. Question: can a tool check, and sync, the Signal design from Claude Design in a build step, or in a special step?

Marks follow `docs/research/README.md`.
Every mark is dated 2026-11-19.
The link of the design system is a claude.ai artifact shared with anyone who has the link. It is not written in this note.

## Findings in short

- A program without a login cannot read the design system's files. The shared link serves a page shell, and the files load by script. `measured`
- No documented API, command-line tool or token reads a system's files. The routes that exist run inside Claude Code or Claude, under a person's login. `source`
- The repository already holds a copy of the design file, `design/signal/tokens.design.json`, and a tool that writes the token files from it.
  So a check can compare two copies of one file, and only the fetch needs a login.
- Recommendation: option (a), a recorded version and hash with a drift report, now.
  Option (b), a fetch by a person with a login, is the sync. Options (c) and (d) have no documented way to hold the credential.

## What a program without a login gets

Read only. No cookie, token or personal detail was sent; the responses set cookies and none was stored or returned.

| Request | Result | Mark |
|---|---|---|
| `GET` the artifact link | 200, `text/html`, 37 KB: the artifact shell with the frame address and scripts. No token of the design is in it (`lamp-green`, a colour added in version 5, is absent). | `measured` |
| `GET` the link with `/project/tokens.json`, `/tokens.json` or `/project/design-system.json` appended | 200 and the same 174 KB page of the claude.ai application, whatever the path. A route of the application, not a file. | `measured` |
| `GET` the same paths on the artifact's own frame host | 404 `not found`. | `measured` |

The type's documentation has a recipe that reads a design system with a plain `fetch` of the artifact address.
It works for a one-file page, whose content sits in a script block of the HTML.
It says to skip the recipe for a system "kept in files", and Signal's system is kept in files: its files are read by path through an Artifact tool. `source` (the type's `format.md`)

## What reads the files, and with which login

| Route | What it needs | Can CI hold it | Mark |
|---|---|---|---|
| The Artifact tool of Claude Code or Claude: `read` of `project/tokens.json` by path | The person's claude.ai login in a session | No documented token or service account | `source` (the type's `SKILL.md`: "no API token") |
| `/design-sync` in Claude Code, which builds a system from a code base and publishes its files with `"editing": "code"` | Claude Code, signed in, plus the Artifact tool | Not documented | `source` (`from-code.md`) |
| The skill `appifacts-design-system-from-code` of the appifacts plugin: commits a producer folder to the repository "so anyone, or CI, rebuilds this system's files with one command" | The plugin, and the login to publish | The text says CI can run it. It does not say how CI signs in. | `source` (`from-code.md`) |
| An export of the page (a download) | A person at the page | No | `source` (the type lists `downloads`) |

The direction repository to design system is therefore supported, and a pipeline that publishes marks the index `lastChange.via` with "CI", which shows the page banner that edits may be overwritten. `source` (`format.md`)
The direction design system to repository has no such route. A person or an agent reads the files and writes the copy.

## The options

| | What it is | Needs | Can break | Offline, in CI |
|---|---|---|---|---|
| (a) Check only | The repository records the `version` and a SHA-256 of the design file (or of its canonical JSON) in a small file. A tool compares the repository's copy with a copy a person or agent hands it, and reports drift. | Python, no network, no login | Someone edits the copy without the design system, or the reverse. The report says which differs. | Yes. It can be a ctest test like `signal_tokens_check`. |
| (b) Special target | A script, run by a person signed in to Claude Code, that reads the design file through the Artifact tool, writes the copy and runs `tools/signal_tokens.py`. | The person's login | The layout of the design file changes. The `tokens.design.json` shape is the tool's list format, not a standard. | No. It is not part of the build. |
| (c) Scheduled job | A job that opens a pull request when the design changed. | A credential for the Artifact tool | No documented non-interactive login. A session token in CI secrets would break at expiry. | Only with that credential. |
| (d) Build step | The build fetches the file. | Network and a login at every build | Breaks every offline build and every contributor without access. | No. The repository's rule is that a build needs no network. |

The cost of (a) is small: a file of three fields, one Python script and one test.
The cost of (b) is a person's time at each change, which today is one conversation.
(c) and (d) cost a credential that no documentation describes.

## The other direction

Repository to design system is today an agent's publish under the owner's login, with `lastChange.via` naming the surface.
It could be a script on the same footing as (b).
The letter spacings of `title` and `label` are an example: they exist only in the repository's copy until a publish carries them over.

## Standalone Claude Design closes

The task brief says the standalone tool closes on 2026-12-14 and moves into Claude, and attributes this to the Design System type's own instructions. `recalled`
That text was read through an Artifact tool in this session, not from a page of the product.
The six reference files of the type that were read here (`SKILL.md`, `format.md`, `craft.md`, `cover.md`, `from-code.md`, `from-design-tool.md`) do not contain the date.
It may be in a file that was not read, so the date is unconfirmed.
The link is an artifact, and an artifact is a claude.ai object, so it should outlive the standalone page.
Nothing found says whether the address, the shared-with-link setting or the file layout stays.
Before the date, record the version and hash (option a) and keep the repository's copy as the one that builds, which it already is.

## Recommendation

1. Do (a) now: record `version` and hash beside the design file, and add a test that the written token files match the copy.
   The second half exists already as `signal_tokens_check`.
2. Keep (b) as a manual step, written as a short procedure for an agent: read `project/tokens.json`, compare, replace the copy, run `tools/signal_tokens.py`, open a pull request.
3. Do not build (c) or (d) until a documented non-interactive way to read a system exists.
4. Come back after 2026-12-14 and read the link again with the same requests as above.
