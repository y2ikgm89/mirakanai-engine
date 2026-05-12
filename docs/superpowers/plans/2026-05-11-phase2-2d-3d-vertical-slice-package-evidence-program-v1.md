# Phase 2 2D And 3D Vertical Slice Package Evidence Program v1 (2026-05-11)

**Plan ID:** `phase2-2d-3d-vertical-slice-package-evidence-program-v1`  
**Gaps:** `2d-playable-vertical-slice`, `3d-playable-vertical-slice`  
**Parent:** [2026-05-10-unsupported-production-gaps-orchestration-program-v1.md](2026-05-10-unsupported-production-gaps-orchestration-program-v1.md)  
**Status:** Active program ledger  

## Goal

Expand **generated / desktop package smoke** (`--require-*` flags, fail-closed counters) for 2D and 3D vertical slices while respecting `hostGates`—no Metal or Vulkan readiness inflation from Windows-only runs.

## Dependencies

- Phase 1 foundations for streaming, uploads, frame graph, and scene/cook authoring reduce duplicate work when adding new smokes.

## 2D stream targets

- Atlas **authoring** workflow in editor (not only runtime metadata counters).
- Full tilemap **editing canvas** UX beyond runtime UX counters.
- Broader native sprite batching readiness with package-visible `sprite_batch_*` style evidence.

## 3D stream targets

- Dependency cook + package streaming **execution** evidence (not only planning APIs).
- Material graph + **live shader generation** pipeline with audited tool dispatch.
- Metal / broad backend parity only with `metal-apple` / `vulkan-strict` recipes.

## Constraints

- Each new smoke flag must be documented in `docs/testing.md` or game manifest validation recipes as appropriate.
- Generated games remain lowercase snake_case under `games/`.

## Done when (program checkpoint)

- `requiredBeforeReadyClaim` bullets shrink only with matching manifest notes and green recipe commands recorded in dated child plans.

## Linked slices

| Gap id | Plan | Status |
| --- | --- | --- |
| (pending) | — | Add row per landed Phase 2 child plan |
