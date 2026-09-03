# GoHome Documentation Guide

This repo is GoHome (a deep-sea Lethal Company-like co-op survival game), UE5.7. Docs/tools live under `Docs/`, split into **two systems**: **Notion** (content that's fundamentally for the team, not the agent — game design, tech/estimate breakdown, kickoff roadmap, owner assignment, day-to-day Git branching procedure — high change frequency, checkbox/DB-friendly), `Docs/Dev/` (md/git, developer docs read directly by agents — architecture, coding conventions, the AI-agent routing guide, NodeCaster, BP comment colors, binary-merge-conflict CLI steps). **The goal is that `Docs/Dev/` + the current code are sufficient for normal dev-loop work** — not a rule against reading Notion. Notion isn't part of the agent's default working set simply because well-maintained `Docs/Dev/` shouldn't need it for routine work: if a question genuinely needs Notion content, ask the user for it rather than fetching it yourself, unless the user has explicitly asked you to go read/edit Notion.

## Document Lifecycle

- `Docs/Dev/` (`ARCHITECTURE.md`/`CODING_CONVENTIONS.md`/`AI_AGENT_GUIDE.md`) is the primary reference for the main dev loop and is read directly by agents. Design content (game design, system rationale, person-day estimates) lives in Notion and is opened only when Dev docs don't cover the question — default to asking the user to paste the relevant section rather than fetching it yourself.
- **Documented decisions are a baseline, not gospel**: if development surfaces a better design, don't silently follow a stale decision — use judgment, propose the improvement, and reflect it in the doc (Dev doc directly; Notion via the user). Keep an "old decision X, rejected because Y" trail only when a different approach was actually considered and turned down (so it isn't proposed again later); a plain status update (not started → done, planned → implemented) just overwrites — no legacy trail needed. Implementation-status tags themselves ("design confirmed, not built yet", "unimplemented") shouldn't linger once code catches up — code is the single source of truth for what's built (see Document Map's `Docs/Dev/*.md` role), so drop the tag rather than flip it to "done". Keep the tag only while it flags something non-obvious (e.g. "half-wired — one path implemented, the other still a stub" traps a reader would otherwise miss); a plain "not started yet" tag on work that hasn't begun adds no information a `grep`/file-existence check wouldn't give faster.

## Document Map

**Read any `Docs/Dev/*.md` file by section, not whole-file**: `Grep -n "^#"` first to find the right header, then read from that header to the next same-level header.

| Doc | Location | Role |
|---|---|---|
| GoHome 기획서 | **Notion** (search workspace by name) | Core game design (core loop, system specs, phase 1/2 scope, map plan) |
| GoHome 기술분석서 | **Notion** (search workspace by name) | Per-system technical approach, complexity/person-day estimates |
| GoHome 착수 로드맵 | **Notion** (search workspace by name) | Pre-kickoff checklist, open decisions, DoD, dependencies, submission prep |
| GoHome 담당자 배정 | **Notion DB** (search workspace by name) | Owner/target period/start conditions/status per system |
| GoHome Git 워크플로우 | **Notion** (search workspace by name) | GitHub Desktop branch/commit/PR-timing procedure — the CLI binary-conflict-resolution steps live in `CODING_CONVENTIONS.md` instead (agent needs those live, mid-conflict) |
| `Docs/Dev/ARCHITECTURE.md` | md/git (dev) | Maps design systems to UE classes/components/`Source/GoHome/` folders — boundary rules and decisions the code doesn't enforce |
| `Docs/Dev/CODING_CONVENTIONS.md` | md/git (dev) | C++ coding standards, folder rules, C++/Blueprint boundary, binary (`.uasset`/`.umap`) merge-conflict CLI resolution |
| `Docs/Dev/AI_AGENT_GUIDE.md` | md/git (dev) | Procedure teammates use to ask their AI coding agent about their assigned work, incl. the question → doc routing table |
| `Docs/Dev/UI_GUIDE.md` | md/git (dev) | Blueprint UI 화면 구조 — 레이어·카탈로그·HUD 소유·라우터·연출·네이밍 — only opened when adding/restructuring a UI screen |
| `Docs/Dev/NODECASTER_GUIDE.md` | md/git (dev) | NodeCaster install check + Blueprint graph JSON procedure — only opened for Blueprint node-graph questions |
| `Docs/Dev/BP_COMMENT_COLORS.md` | md/git (dev) | Blueprint comment-box color semantics (HSV/Hex table) — only opened when adding comment boxes to a BP graph |

## Starting Work

Before implementing, modifying, or advising on a system — including step-by-step guidance — do all three:

1. **Read the relevant `ARCHITECTURE.md` section(s)** for the systems the task touches, plus the cross-cutting rule sections (`공유 헤더 규칙`, `Replication 권한 원칙`). Section-by-section per the Document Map note.
2. **Read the full header of every class you'll name**, and the specific `.cpp` functions you'll change or that your advice depends on (delegate signatures, existing overrides). Headers whole; `.cpp` selectively — not whole-file.
3. **If `git status` shows a modified `Docs/Dev/*.md`, read its `git diff`.** A doc under edit is someone revising the design for this exact work; for continuation work also check `git log --oneline origin/dev -10` and `git diff origin/dev...HEAD -- Docs/Dev`.

Memory notes and prior-session summaries are pointers, not substitutes for the above — they go stale and may predate a written decision. Do not produce a plan from them alone.

## Validation Scripts

```
node Docs/tools/check_doc_links.js
```

Validates markdown anchor links in this file and `Docs/Dev/*.md`. Run after any header-text change.

```
node Docs/tools/check_architecture_symbols.js
```

Checks that backtick-quoted UE class/interface/enum names and `.h`/`.cpp` filenames in `ARCHITECTURE.md` exist under `Source/GoHome/`. Run after renaming/deleting a class the doc names. Items tagged "(신설 제안)" are expected to be missing — treat as noise.
