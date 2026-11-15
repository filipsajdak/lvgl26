# Giving library documentation to AI coding agents

What the evidence says about how an open-source library should hand its documentation to coding agents. Researched on 2026-11-15. Marks follow [README.md](README.md).

## Measurements

| Finding | Link | Mark |
|---|---|---|
| Next.js 16 API evals: baseline 53%, skill 53%, skill plus explicit instruction 79%, 8 KB docs index in AGENTS.md 100%. | [Vercel](https://vercel.com/blog/agents-md-outperforms-skills-in-our-agent-evals) | `source` |
| The skill was never invoked in 56% of cases. The index shrank from 40 KB to 8 KB with no loss. | [Vercel](https://vercel.com/blog/agents-md-outperforms-skills-in-our-agent-evals) | `source` |
| Live evals, pass@4, 16 agents: Sonnet 5 82% to 100% with AGENTS.md; Opus 5.5 100% both ways. | [nextjs.org/evals](https://nextjs.org/evals) | `source` |
| 2,400 runs, Claude Code and Codex: 404s per task 2.23 (HTML), 1.42 (Markdown), 0.11 (Markdown linking llms.txt). | [Mintlify benchmark](https://www.mintlify.com/blog/llms-txt-agent-benchmark) | `source` |
| Same benchmark: accuracy mid-to-high 90s in every format; inlining the whole llms.txt "cost more tokens every time". | [Mintlify benchmark](https://www.mintlify.com/blog/llms-txt-agent-benchmark) | `source` |
| Context files do not generally raise task success and raise cost over 20%. Repository overviews do not help. | [arXiv 2602.11988](https://arxiv.org/abs/2602.11988) | `source` |
| One repository, Copilot CLI, five runs: a 12-line AGENTS.md cut time 27%. A single run showed the opposite. | [AAIF](https://aaif.io/blog/measuring-agents-md-what-five-runs-show-that-one-doesn-t) | `source` |
| One page: HTML 180,573 tokens, Markdown 478. | [Checkly](https://www.checklyhq.com/blog/state-of-ai-agent-content-negotation/) | `source` |

## Formats and who reads them

| Finding | Link | Mark |
|---|---|---|
| llms.txt is still a proposal (v2, edited 2026-08-10). It gives no size limit and never mentions `llms-full.txt`. | [llmstxt.org](https://llmstxt.org/) | `source` |
| The proposal asks for a Markdown twin of each page at `page.md`. | [llmstxt.org](https://llmstxt.org/) | `source` |
| 97% of llms.txt files got no request in May 2026. Claude Code was the second named AI fetcher. | [Ahrefs](https://ahrefs.com/blog/llmstxt-study/) | `source` |
| No AI bot requested a missing llms.txt: agents do not look for it unprompted. | [Ahrefs](https://ahrefs.com/blog/llmstxt-study/) | `source` |
| Agent traffic by route: direct `.md` 54.4%, plain text 34.5%, `Accept: text/markdown` 8.3%. | [Mintlify traffic](https://mintlify.com/blog/state-of-docs-traffic) | `source` |
| Only 3 of 7 agents send `Accept: text/markdown`: Claude Code, Cursor, OpenCode. Codex, Gemini CLI, Copilot, Windsurf do not. | [Checkly](https://www.checklyhq.com/blog/state-of-ai-agent-content-negotation/) | `source` |
| AGENTS.md is stewarded by the Agentic AI Foundation. The page lists 23 tools; Claude Code is not listed. | [agents.md](https://agents.md/) | `source` |
| Claude Code v2.1.277+ reads AGENTS.md only when no CLAUDE.md exists; otherwise CLAUDE.md must import `@AGENTS.md`. | [Claude Code docs](https://code.claude.com/docs/en/memory) | `source` |
| Skill spec: description up to 1024 characters, body under 500 lines and 5000 tokens, references one level deep. | [agentskills.io](https://agentskills.io/specification) | `source` |
| Anthropic: write at least three evaluations before the skill; test on every model tier. | [Anthropic](https://platform.claude.com/docs/en/agents-and-tools/agent-skills/best-practices) | `source` |
| Context7 indexes any public repository. `context7.json` sets folders, versions and `rules` for agents. | [Context7](https://context7.com/docs/library-owners.md) | `source` |

## What comparable projects ship

| Project | Ships | Link | Mark |
|---|---|---|---|
| Next.js | Docs inside the package; `next dev` writes a marked AGENTS.md block; MCP for compile errors. | [docs](https://unpkg.com/next@16.4.0/dist/docs/01-app/02-guides/ai-agents.md) | `source` |
| Svelte | MCP with docs tools and `svelte-autofixer`; skills; plugins for six agents. | [svelte.dev](https://svelte.dev/docs/ai/tools) | `source` |
| Astro | llms.txt and llms-full.txt as fallback; hosted docs MCP preferred. | [Astro](https://v5.docs.astro.build/en/guides/build-with-ai) | `source` |
| shadcn/ui | One skill (`npx skills add shadcn/ui`) and an MCP inside its CLI. | [shadcn](https://ui.shadcn.com/docs/skills.md) | `source` |
| Stripe | Plugin bundling MCP and skills; `.md` on every URL; `stripe agent setup`. | [Stripe](https://docs.stripe.com/building-with-llms.md) | `source` |
| TanStack | Skills inside npm packages, pinned to the installed version; an install command wires them in. | [TanStack](https://tanstack.com/blog/from-docs-to-agents) | `source` |
| Qt | SKILL.md skills for Claude Code, Codex CLI, Gemini CLI; hosted docs MCP per Qt version. | [Qt](https://doc.qt.io/agentictools/) | `source` |
| Slint | Hosted docs MCP (`search_docs`, `read_doc`); AGENTS.md for contributors only. | [Slint](https://docs-mcp.slint.dev/) | `source` |
| Zephyr | Kapa.ai docs MCP; nothing else for coding agents. | [Zephyr](https://docs.zephyrproject.org/latest/develop/tools/kapa_ai.html) | `source` |
| LVGL | A 115,021-character llms.txt with "undefined" entries; no AGENTS.md in the repository. | [LVGL](https://lvgl.io/docs/open/llms.txt) | `source` |
| Tailwind | Closed the llms.txt pull request; ships none. | [PR 2388](https://github.com/tailwindlabs/tailwindcss.com/pull/2388) | `summary` |

## What it means for us

1. Do first: ship the Markdown docs with the library, and a short marked block for the user's AGENTS.md. The block holds a compressed index and "read these before writing code". Rests on Vercel and nextjs.org/evals.
2. Write that block into CLAUDE.md as well, or as an `@AGENTS.md` import. Claude Code skips AGENTS.md when CLAUDE.md exists. Deliver it with one install command, as Next.js and TanStack do.
3. Keep the block small and hand-written: rules that differ from plain LVGL, nothing else. Rests on arXiv 2602.11988 and the 8 KB result.
4. Publish every site page at a `.md` URL and link `llms.txt` from each page. Keep `llms.txt` a small map. Rests on the Mintlify benchmark and traffic shares.
5. Keep `build-ui` for the workflow, not for API knowledge. Stay under 500 lines; name it in the block. Rests on the 56% non-invocation.
6. Build a small eval set before adding more formats: UI tasks, several runs each, with and without each aid. Rests on AAIF, Anthropic and Vercel.
7. Give the MCP server a tool that checks code or explains a compile error. Svelte and Next.js ship one; nobody published a measurement.
8. Do not bother with: `Accept` negotiation (GitHub Pages cannot do it; 8.3% of traffic), a hosted docs MCP, a large `llms-full.txt`. Registering in Context7 costs one file.

## Not confirmed

- No published measurement compares error-to-fix tables, or complete compilable examples, against prose. The error catalogue rests on reasoning only.
- No measurement compares a documentation MCP against static files. Astro prefers MCP; Next.js prefers bundled docs.
- No size guidance for `llms-full.txt` exists in the proposal. Mintlify's inlining result is the only data.
- The agentskills.io client list did not render. Qt names Claude Code, Codex CLI and Gemini CLI; counts of "30+" are from blogs.
- Vercel's post names neither the model nor the eval count. Its result covers APIs newer than training data, as ours are.
- Vercel and the arXiv paper measure different things: library API knowledge against repository guidance.
- Mintlify sells docs hosting; Ahrefs sells SEO tools. Both samples come from their own customers.
- All evidence is from JavaScript projects. Nothing was found for C++ libraries or CMake delivery.
- Whether Context7 accepts a website or llms.txt as a source was not confirmed.
- Tailwind's reasons are from secondary reports.
