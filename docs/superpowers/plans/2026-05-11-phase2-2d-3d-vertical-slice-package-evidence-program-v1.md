# Phase 2 2D And 3D Vertical Slice Package Evidence Program v1 (2026-05-11)

**Plan ID:** `phase2-2d-3d-vertical-slice-package-evidence-program-v1`  
**Gaps:** `2d-playable-vertical-slice`, `3d-playable-vertical-slice`  
**Parent:** [2026-05-10-unsupported-production-gaps-orchestration-program-v1.md](2026-05-10-unsupported-production-gaps-orchestration-program-v1.md)  
**Status:** Active program ledger  

## Goal

Expand **generated / desktop package smoke** (`--require-*` flags, fail-closed counters) for 2D and 3D vertical slices while respecting `hostGates`—no Metal or Vulkan readiness inflation from Windows-only runs.

## Dependencies

- Phase 1 foundations for streaming, uploads, frame graph, and scene/cook authoring reduce duplicate work when adding new smokes.

## 2D stream status

- Closed for the Engine 1.0 Windows-default generated package surface by [2026-05-18-2d-playable-vertical-slice-1-0-closeout-v1.md](2026-05-18-2d-playable-vertical-slice-1-0-closeout-v1.md).
- Atlas **authoring** workflow in editor, full tilemap **editing canvas** UX, and broader renderer-quality sprite batching are future/editor/renderer follow-ups, not blockers for the supported 1.0 generated 2D package claim.

## 3D stream status

- Closed for the Engine 1.0 Windows-default generated package surface by [2026-05-18-3d-playable-vertical-slice-1-0-closeout-v1.md](2026-05-18-3d-playable-vertical-slice-1-0-closeout-v1.md).
- Dependency cook + package streaming **execution**, material graph + **live shader generation**, Metal / broad backend parity, native handles, and general renderer quality are future/host-gated follow-ups, not blockers for the supported 1.0 generated 3D package claim.

## Constraints

- Each new smoke flag must be documented in `docs/testing.md` or game manifest validation recipes as appropriate.
- Generated games remain lowercase snake_case under `games/`.

## Done when (program checkpoint)

- `requiredBeforeReadyClaim` bullets shrink only with matching manifest notes and green recipe commands recorded in dated child plans.

## Linked slices

| Gap id | Plan | Status |
| --- | --- | --- |
| `2d-playable-vertical-slice` | [2026-05-18-2d-playable-vertical-slice-1-0-closeout-v1.md](2026-05-18-2d-playable-vertical-slice-1-0-closeout-v1.md) | Completed |
| `3d-playable-vertical-slice` | [2026-05-18-3d-playable-vertical-slice-1-0-closeout-v1.md](2026-05-18-3d-playable-vertical-slice-1-0-closeout-v1.md) | Completed |
