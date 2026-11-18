# The official Design Tokens Format schemas

`2025.10/format.json` and `2025.10/resolver.json` are the JSON Schemas of the Design Tokens Format Module and Resolver Module, version 2025.10.
They are unchanged copies of the files that https://www.designtokens.org/schemas/2025.10/format.json and `.../resolver.json` serve, fetched 2026-11-18.
The source files are in `schemas/src/2025.10/` of https://github.com/design-tokens/community-group, at commit 882ebd6.

The licence is in `LICENSE.md` (a copy of the repository's own).
It puts the specifications under the W3C Software and Document License and the software under the W3C 3-clause BSD License.
Both allow a copy with the notice kept.
The schema files carry no notice of their own, so the copy of `LICENSE.md` stays next to them.

`tools/check_tokens.py` reads these files and needs no network: each schema holds every file it refers to.
To move to a newer version, add its directory next to `2025.10` and change `SCHEMAS` in the tool.
