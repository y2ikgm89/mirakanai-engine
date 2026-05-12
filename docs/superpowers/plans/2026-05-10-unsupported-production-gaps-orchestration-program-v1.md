# Unsupported Production Gaps Orchestration Program v1 (2026-05-10)

**Plan ID:** `unsupported-production-gaps-orchestration-program-v1`  
**Status:** Active program index. This file is the machine-readable execution index for all eleven `engine/agent/manifest.json.unsupportedProductionGaps` rows; it does not replace gap-by-gap validation or dated implementation child plans.

## Goal

Provide a single repository-owned orchestration ledger that aligns the production-completion master plan with roadmap order **A (contracts and runtime spine) → C (2D/3D vertical slices) → B (editor productization)** and records phase boundaries without broadening ready claims.

## Context

- Master plan: [`2026-05-03-production-completion-master-plan-v1.md`](2026-05-03-production-completion-master-plan-v1.md)
- Roadmap: [`docs/roadmap.md`](../../roadmap.md)
- Manifest gaps: `unsupportedProductionGaps` in [`engine/agent/manifest.json`](../../../engine/agent/manifest.json)

## Constraints

- Burn down **one manifest gap row at a time** until closeout, host-gated evidence, or explicit 1.0 exclusion; do not switch gaps only because a small child slice finished.
- Every implementation slice uses an English dated child plan under [`docs/superpowers/plans/`](./) with Goal, Context, Constraints, Done When, and validation evidence.
- Close coherent slices with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` when the environment allows.

## Phase Map (gap IDs)

| Phase | Gap IDs | Notes |
| --- | --- | --- |
| Phase 0 | `editor-productization` | Continue manifest-preferred burn-down; select next narrow child via `next-production-gap-selection`. |
| Phase 1 | `asset-identity-v2`, `runtime-resource-v2`, `renderer-rhi-resource-foundation`, `upload-staging-v1`, `frame-graph-v1`, `scene-component-prefab-schema-v2` | Foundation elevation; child plans may touch multiple modules in one coherent slice. |
| Phase 2 | `2d-playable-vertical-slice`, `3d-playable-vertical-slice` | Package-visible proofs; respect `hostGates`—do not claim Metal/Vulkan readiness without evidence. |
| Phase 3 | `production-ui-importer-platform-adapters` | Dependency and legal records must move with any new adapters. |
| Phase 4 | `full-repository-quality-gate` | May run parallel to engineering work; do not expand ready claims without manifest alignment. |

## Done When

- `aiOperableProductionLoop.recommendedNextPlan.path` remains the production-completion master plan file while no dated child plan header is active (JSON contracts), with this orchestration ledger referenced from `recommendedNextPlan.completedContext` and [`README.md`](./README.md).
- Phase owners record progress only through completed dated plans, manifest notes, and `docs/superpowers/plans/README.md`—not through silent gap hopping.

## Validation evidence

| Check | Result |
| --- | --- |
| Plan registered in [`README.md`](./README.md) Current Active Work | **PASS** — Active slice row references the master plan and links [2026-05-11-production-completion-gap-stream-plans-index-v1.md](./2026-05-11-production-completion-gap-stream-plans-index-v1.md) as the Phase 0–4 stream index; latest completed slice records nested prefab propagation dry-run counters ([2026-05-11-editor-nested-prefab-propagation-candidate-dry-run-v1.md](./2026-05-11-editor-nested-prefab-propagation-candidate-dry-run-v1.md)); the next `editor-productization` child remains `recommendedNextPlan.id=next-production-gap-selection`. |
| Manifest `recommendedNextPlan.path` / `currentActivePlan` | **PASS** — both paths reference `2026-05-03-production-completion-master-plan-v1.md` per `tools/check-json-contracts.ps1` / `tools/check-ai-integration.ps1`; `completedContext` references this orchestration file |

## Phase execution ledger (orchestration only)

This program file sequences gap IDs (Phase 0–4); it does **not** remove `unsupportedProductionGaps` rows. Each phase closes only through dated child plans, manifest closeout, host-gated evidence, or explicit 1.0 exclusion recorded in `engine/agent/manifest.json` per the production-completion master plan.

| Phase | Gap IDs (manifest) | Execution status |
| --- | --- | --- |
| Phase 0 | `editor-productization` | Active burn-down; stream index [2026-05-11-production-completion-gap-stream-plans-index-v1.md](./2026-05-11-production-completion-gap-stream-plans-index-v1.md). **Latest completed child:** [2026-05-11-editor-nested-prefab-propagation-candidate-dry-run-v1.md](./2026-05-11-editor-nested-prefab-propagation-candidate-dry-run-v1.md) (descendant linked prefab root + distinct nested asset dry-run counters and retained summary rows). **Next child:** `recommendedNextPlan.id=next-production-gap-selection`. |
| Phase 1 | `asset-identity-v2`, `runtime-resource-v2`, `renderer-rhi-resource-foundation`, `upload-staging-v1`, `frame-graph-v1`, `scene-component-prefab-schema-v2` | Tracked — foundation elevation follows roadmap **A** after Phase 0 decisions; no bulk gap removal from this index alone. |
| Phase 2 | `2d-playable-vertical-slice`, `3d-playable-vertical-slice` | Tracked — vertical-slice proofs respect `hostGates`; no Metal/Vulkan parity claims without evidence. |
| Phase 3 | `production-ui-importer-platform-adapters` | Tracked — adapters require dependency/legal/vcpkg alignment per `AGENTS.md`. |
| Phase 4 | `full-repository-quality-gate` | Tracked — tidy/coverage/sanitizer/CI evidence expands only with manifest-aligned ready claims. |
