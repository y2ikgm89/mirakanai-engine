# Production documentation stack v1 (2026-05-11)

## Goal

Define a **clean-break, no-backward-compatibility** documentation authority stack so production execution does not require bulk-rewriting the master plan ledger whenever repository layout or agent surfaces change.

## Context

- The master plan file is intentionally a **long-form ledger** (verdict snapshots, completed slice references). Treat it as historical evidence plus high-level gap strategy, not as the only place to record physical tree rules.
- Recent `engine/tools` layout work is a **foundation cross-cut**: it constrains CMake, validation needles, and agent skills, but it is not a new `unsupportedProductionGaps` row.
- AGENTS.md plan-volume policy discourages huge pointer churn inside completed child plans and discourages creating micro-plans for pure docs sync; this file is the **single governance slice** for “how plans relate to specs/ADRs.”

## Authority stack (read order)

1. **`engine/agent/manifest.json`** — `aiOperableProductionLoop.currentActivePlan`, `recommendedNextPlan`, `unsupportedProductionGaps`, readiness recipes.
2. **`docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`** — production completion definition, gap burn-down strategy, current verdict pointers (must stay consistent with manifest for *active gap* and *ready claims*).
3. **`docs/superpowers/plans/README.md`** — registry: what is active vs completed; links to latest slices.
4. **Dated child plans** (`docs/superpowers/plans/2026-*-*.md`) — scoped Goal / Done when / validation tables; **do not rewrite** completed files to “fix” old path strings; add a short note in the registry or this stack doc instead.
5. **Specs and ADRs** — durable layout and invariant truth (example: [directory layout target v1](../../specs/2026-05-11-directory-layout-target-v1.md), [ADR 0003](../../adr/0003-directory-layout-clean-break.md)).
6. **`docs/architecture.md`**, **`docs/workflows.md`**, **`docs/current-capabilities.md`**, **`docs/roadmap.md`** — human-facing summaries; must not contradict (1)–(5).

## Clean-break rules

- **No compatibility shims** for removed physical layouts (for example do not reintroduce a flat `engine/tools/src/` tree “for old docs or scripts”).
- When CMake paths, needles in `tools/check-json-contracts.ps1` / `tools/check-ai-integration.ps1`, or agent skills change, update **spec/ADR + skills + scripts in one task**, per [directory layout tools split v1](2026-05-11-directory-layout-tools-split-v1.md).
- **Do not** expand the master plan’s “Previous Verdict Snapshot” or historical paragraphs to enumerate every path migration; link **spec invariant 4** for `MK_tools` link policy instead.

## Done when

- [x] Master plan **Context** references this stack and the directory layout spec/ADR as foundation authority.
- [x] Plan registry lists the directory layout slice as completed foundation evidence and points readers here for stack rules.
- [x] [Directory layout tools split v1](2026-05-11-directory-layout-tools-split-v1.md) links back to this document under integration.

## Validation

| Check | Result |
| --- | --- |
| Manifest `currentActivePlan` unchanged unless a deliberate execution switch | N/A (docs-only governance); manifest still points at master plan for gap burn-down. |
| No contradiction between spec invariant 4 and CMake skills | Verified at authoring time. |
