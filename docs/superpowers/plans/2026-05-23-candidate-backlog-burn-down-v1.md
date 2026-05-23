# Candidate Backlog Burn-down v1 (2026-05-23)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `candidate-backlog-burn-down-v1`

**Status:** Completed.

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

- [x] 1. [sprite-sorting-layer-v1](2026-05-23-sprite-sorting-layer-v1.md) — `MK_scene`, `MK_scene_renderer`, `MK_renderer` (PR #196)
- [x] 2. [sprite-9slice-and-tiled-v1](2026-05-23-sprite-9slice-and-tiled-v1.md) — assets/UI/scene renderer/tools (PR #197)
- [x] 3. [sprite-collision-hitbox-v1](2026-05-23-sprite-collision-hitbox-v1.md) — runtime, physics, gameplay interaction
- [x] 4. [sprite-effects-particles-v1](2026-05-23-sprite-effects-particles-v1.md) — runtime, scene renderer, selected package counters
- [x] 5. [sprite-editor-preview-diagnostics-v1](2026-05-23-sprite-editor-preview-diagnostics-v1.md) — editor core, runtime diagnostics
- [x] 6. [navigation-hierarchical-world-v1](2026-05-23-navigation-hierarchical-world-v1.md) — navigation, world-region streaming refs (PR #203)
- [x] 7. [simulation-persistence-v1](2026-05-24-simulation-persistence-v1.md) — session/snapshot layer on `MK_runtime` (PR #204)

## Done when

- All seven child plans merged; backlog rows promoted to `implemented-1x-foundation`.
- Milestone closed; manifest/registry pointers synced; no orphan worktrees; root `main` ff-synced with `origin/main`.

## Evidence log

| Capability | PR | Merge SHA | validate.ps1 | Notes |
| --- | --- | --- | --- | --- |
| sprite-sorting-layer-v1 | #196 | `e587e1a45fd3e750d79364a17764735d81b3496f` | pass | deterministic sorting layers and package counters |
| sprite-9slice-and-tiled-v1 | #197 | `e4dcbbac2dd2114b8441d5de8f967f2f1b03d1f8` | pass | 9-slice/tiled expansion and package counters |
| sprite-collision-hitbox-v1 | #200 | `8f94c16624b0677870248ed256a07a9e2cd730f5` | pass | `RuntimeSpriteCollisionHitboxRequest`, deterministic hit/gameplay rows, selected package counters, and 75/75 `validate.ps1` tests |
| sprite-effects-particles-v1 | #201 | `817915966a293fca645bb593a9714efda558419a` | pass | `RuntimeSpriteEffectParticleRequest`, deterministic active/spawn/render rows, scene renderer bridge, selected package counters, focused test/smoke evidence, and 76/76 `validate.ps1` tests |
| sprite-editor-preview-diagnostics-v1 | #202 | `0ec60fa33ed2915aee5603c92b4f1243c4be9346` | pass | `EditorSpritePreviewDiagnosticsModel`, host-owned `EditorSpritePreviewExecutionSnapshot`, selected sprite/atlas/animation/sorting/dependency/collision diagnostics, retained UI rows, focused `MK_editor_core_tests`/`check-tidy` evidence, full `validate.ps1` 76/76 test evidence, and D3D12 queue-qualified lifetime CI fix from PR #202 |
| navigation-hierarchical-world-v1 | #203 | `01d87b1e13691585aac6b27572b1c05d278896cf` | pass | `NavigationHierarchicalWorldPathRequest`, `plan_navigation_hierarchical_world_path`, `RuntimeWorldRegionNavigationRefReviewRequest`, `RuntimeWorldRegionNavigationPathCacheReviewRequest`, `review_runtime_world_region_navigation_refs`, `review_runtime_world_region_navigation_path_cache`, focused `MK_navigation_tests`/`MK_runtime_world_region_streaming_tests`, focused static checks, full `validate.ps1` 76/76 test evidence, hosted Windows cache restore/save hardening, and merged PR evidence |
| simulation-persistence-v1 | #204 | `971cee3f6c5b42965721c06974bc506f1b35508c` | pass | `RuntimeSimulationPersistenceRequest`, `RuntimeSimulationPersistencePlan`, `RuntimeSimulationPersistentEntityRow`, `RuntimeSimulationPersistenceMigrationStep`, `RuntimeSimulationPersistenceRemediationAction`, and `plan_runtime_simulation_persistence` for value-only save-slot/world/snapshot/tick review, deterministic entity rows, reachable schema migration chains, non-save document recovery, corrupt/unsupported save remediation, docs/manifest/static-check synchronization, focused validation, full `validate.ps1` 76/76 test evidence, hosted PR Gate success, and merged PR evidence |
