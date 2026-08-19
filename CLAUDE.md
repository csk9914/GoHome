# GoHome Documentation Guide

This repo is GoHome (a deep-sea Lethal Company-like co-op survival game), UE5.7. All docs/tools live under `Docs/`, split into **three systems**: `Docs/Design/` (md/git, files 01·02·03·05 — low change frequency, cross-indexing matters), Notion (high-change-frequency items like owners/schedule/DoD that fit checkboxes/DBs), `Docs/Dev/` (md/git, developer docs that change with the code — architecture/coding conventions/AI agent guide/git workflow).

Content rules specific to `Docs/Design/` (single-source-of-truth, editing procedure, notation labels, md↔Notion linking) live in [Docs/Design/DOC_MANAGEMENT.md](Docs/Design/DOC_MANAGEMENT.md) — read that file when editing a Design doc. This file only covers what's needed in every session: the doc map and link validation.

## Document Lifecycle

- **Now (design phase)**: `Docs/Dev/` (`ARCHITECTURE.md`/`CODING_CONVENTIONS.md`) is the primary reference for the main dev loop — `Docs/Design/` (01/02/03/05) is opened only when Dev docs don't cover the question (e.g. tracing a decision's original rationale), and edited only on the user's explicit request, never as a side effect of other work. When editing, `DOC_MANAGEMENT.md` rules apply in full.
- **After prototype kickoff**: tag that commit and treat `Docs/Design/` as a frozen reference — this changes nothing about the rule above, since Design was already read-mostly. New Design decisions after the freeze still go through `DOC_MANAGEMENT.md`'s single-source-of-truth rules, but should be rare.
- **Documented decisions are a baseline, not gospel**: if development surfaces a better design, don't silently follow a stale decision — use judgment, propose the improvement, and reflect it in the doc. Keep an "old decision X, rejected because Y" trail only when a different approach was actually considered and turned down (so it isn't proposed again later); a plain status update (not started → done, planned → implemented) just overwrites — no legacy trail needed. Implementation-status tags themselves ("design confirmed, not built yet", "unimplemented") shouldn't linger once code catches up — code is the single source of truth for what's built (see Document Map's `Docs/Dev/*.md` role), so drop the tag rather than flip it to "done". Keep the tag only while it flags something non-obvious (e.g. "half-wired — one path implemented, the other still a stub" traps a reader would otherwise miss); a plain "not started yet" tag on work that hasn't begun adds no information a `grep`/file-existence check wouldn't give faster. See `ARCHITECTURE.md`'s own note on this.

## Document Map

**Read any `Docs/Dev/*.md` file by section, not whole-file**: these are organized under distinct headers per system/topic. For a normal dev-loop question (e.g. "what class do I need for X", or a routing question answered by `AI_AGENT_GUIDE.md`'s table), `Grep -n "^#"` first to find the right header, then read from that header to the next same-level header — not the whole file. Whole-file reads are fine for meta questions about the docs themselves (e.g. "what's in Docs/Dev overall").

| Doc | Location | Role |
|---|---|---|
| `Docs/Design/01_GoHome_기획서.md` | md/git | Core game design (core loop, system specs, phase 1/2 scope, map plan) |
| `Docs/Design/02_GoHome_기술분석서.md` | md/git | Per-system technical approach, complexity/person-day estimates |
| `Docs/Design/03_GoHome_리스크_미결정사항.md` | md/git | ① Actually-open decisions, ② index of decisions already made |
| GoHome 착수 로드맵 | **Notion** (search workspace by name) | Pre-kickoff checklist, DoD, dependencies, submission prep |
| GoHome 담당자 배정 | **Notion DB** (search workspace by name) | Owner/target period/start conditions/status per system |
| `Docs/Design/05_GoHome_참고문서_교차매핑.md` | md/git | Which source (teammate draft/mission guide) each design decision traces to; gap-check vs. mission guide |
| `Docs/Design/DOC_MANAGEMENT.md` | md/git | Content-integrity rules for `Docs/Design/` (single source of truth, editing procedure, notation labels) |
| `Docs/Dev/ARCHITECTURE.md` | md/git (dev) | Maps design-doc systems to UE classes/components/`Source/GoHome/` folders |
| `Docs/Dev/CODING_CONVENTIONS.md` | md/git (dev) | C++ coding standards, folder rules, C++/Blueprint boundary |
| `Docs/Dev/AI_AGENT_GUIDE.md` | md/git (dev) | Procedure teammates use to ask their AI coding agent about their assigned work |
| `Docs/Dev/GIT_WORKFLOW.md` | md/git (dev) | GitHub Desktop workflow, branch strategy, when to adopt PRs |
| `Docs/Dev/NODECASTER_GUIDE.md` | md/git (dev) | NodeCaster install check + Blueprint graph JSON procedure — only opened for Blueprint node-graph questions |
| `Docs/Dev/BP_COMMENT_COLORS.md` | md/git (dev) | Blueprint comment-box color semantics (HSV/Hex table) — only opened when adding comment boxes to a BP graph |

## Validation Scripts

```
node Docs/tools/check_doc_links.js
```

Reproduces GitHub's anchor-generation rule for every header in this file, `Docs/Design/*.md`, and `Docs/Dev/*.md`, then checks that every markdown cross-link with an anchor (relative path, including Dev → `../Design/...`) resolves to a real header. Run this habitually after any header-text change, in this file or Design/Dev docs alike. Assumes the repo is viewed on GitHub.

```
node Docs/tools/check_architecture_symbols.js
```

Checks that backtick-quoted UE class/interface/enum names and `.h`/`.cpp` filenames in `ARCHITECTURE.md` actually exist under `Source/GoHome/`. Run this after renaming/deleting a class the doc names, or after editing `ARCHITECTURE.md`'s class references. Items tagged "(신설 제안)" in the doc are expected to be missing — treat those as noise, not failures.
