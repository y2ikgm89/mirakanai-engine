# Candidate Backlog Burn-down v1 (2026-05-23)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `candidate-backlog-burn-down-v1`

**Status:** In progress.

**Parent index:** [Production Completion Master Plan v1](../master-plans/2026-05-03-production-completion-master-plan-v1.md)

## Goal

Implement all 7 canonical `candidate` rows in [04-developer-owned-engine-capability-backlog.md](../master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md) as `implemented-1x-foundation` slices with clean-break contracts, deterministic tests, package counters, merged PR evidence, and honest non-goals while keeping `unsupportedProductionGaps = []`.

## Context

- Post-1.0 selection ledger owns capability boundaries; one capability = one dated child plan = one PR.
- Proven slice pattern: [2026-05-23-engine-gameplay-interaction-framework-v1.md](2026-05-23-engine-gameplay-interaction-framework-v1.md).
- Long-running autonomous drive: linked worktree per capability, `validate.ps1` at slice close, PR merge before next worktree.

## Constraints

- No compatibility shims; replace blocking contracts in the same validated slice.
- Agent-surface drift closes in the same PR (no follow-up docs PR).
- Edit `engine/agent/manifest.fragments/*.json` only; compose manifest with `tools/compose-agent-manifest.ps1 -Write`.
- Full `tools/validate.ps1` once per capability closeout.

## Subagent dispatch (Tasks A–E)

| Task | Mode | Agents |
| --- | --- | --- |
| A kickoff | parallel readonly | `explorer`, `docs-researcher`/Context7, capability-specific auditor |
| B implement | sequential write | parent / `gameplay-builder` in linked worktree, TDD |
| C review | parallel readonly | `cpp-reviewer`, `agent-surface-auditor`, capability auditor |
| D surface + validate | parent | docs/manifest/static checks, compose, `validate.ps1` |
| E publication | babysit | `ci-watcher`, `build-fixer`, `babysit` skill until MERGED |

## Capability burn-down checklist

- [ ] 1. [sprite-sorting-layer-v1](2026-05-23-sprite-sorting-layer-v1.md) — `MK_scene`, `MK_scene_renderer`, `MK_renderer`
- [ ] 2. [sprite-9slice-and-tiled-v1](2026-05-23-sprite-9slice-and-tiled-v1.md) — assets/UI/scene renderer/tools
- [ ] 3. [sprite-collision-hitbox-v1](2026-05-23-sprite-collision-hitbox-v1.md) — runtime, physics, gameplay interaction
- [ ] 4. [sprite-effects-particles-v1](2026-05-23-sprite-effects-particles-v1.md) — runtime, scene renderer, renderer stats
- [ ] 5. [sprite-editor-preview-diagnostics-v1](2026-05-23-sprite-editor-preview-diagnostics-v1.md) — editor core, runtime diagnostics
- [ ] 6. [navigation-hierarchical-world-v1](2026-05-23-navigation-hierarchical-world-v1.md) — navigation, world-region streaming refs
- [ ] 7. [simulation-persistence-v1](2026-05-23-simulation-persistence-v1.md) — session/snapshot layer on `MK_runtime`

## Done when

- All seven child plans merged; backlog rows promoted to `implemented-1x-foundation`.
- Milestone closed; manifest/registry pointers synced; no orphan worktrees; root `main` ff-synced with `origin/main`.

## Evidence log

| Capability | PR | Merge SHA | validate.ps1 | Notes |
| --- | --- | --- | --- | --- |
| program start | | | | milestone + registry + manifest pointer |
