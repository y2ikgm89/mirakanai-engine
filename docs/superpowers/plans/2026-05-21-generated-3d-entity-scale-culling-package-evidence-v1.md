# Generated 3D Entity Scale Culling Package Evidence v1 (2026-05-21)

**Plan ID:** `generated-3d-entity-scale-culling-package-evidence-v1`
**Status:** Completed.
**Current pointer rule:** Keep `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` on the production master plan after closeout. Keep `unsupportedProductionGaps = []`.

## Goal

Extend Entity Scale Culling Package Evidence v1 to `sample_generated_desktop_runtime_3d_package` with a lightweight installed package smoke that reuses the same `entity_scale_culling_*` counters as the 2D package proof.

## Context

- `docs/superpowers/plans/2026-05-21-engine-entity-scale-and-culling-v1.md` completed MK_runtime planning APIs and `sample_2d_desktop_runtime_package --require-entity-scale-culling`.
- Generated 3D still lacked package-visible entity scale/culling counters.
- Non-goal: scene mutation, GPU culling, renderer/RHI execution, or broad high-object-count performance claims.

## Constraints

- Preserve `unsupportedProductionGaps = []`.
- Reuse existing `entity_scale_culling_*` field names and validate-installed gating.
- Keep the smoke lightweight: config + scene package + entity-scale flag only.

## Phase 1: Generated 3D Entity Scale Package Flag

**Status:** Completed.

### Done When

- `--require-entity-scale-culling` exists on `sample_generated_desktop_runtime_3d_package`.
- Smoke stdout reports planned entity scale/culling counters when the flag is set.
- Smoke exit fails closed when probe readiness is false.

## Phase 2: Package Smoke And Validation

**Status:** Completed.

### Done When

- `installed-d3d12-3d-entity-scale-culling-smoke` exists in game and engine validation recipes.
- `tools/validate-installed-desktop-runtime.ps1` validates entity scale/culling fields when the flag is set.
- Agent surfaces stay aligned.

## Validation Evidence

| Check | Result |
| --- | --- |
| `tools/check-ai-integration.ps1` | Pass |
| `tools/validate.ps1 -StaticOnly` | Pass |

## Non-Goals

- Wiring entity scale/culling into scene GPU draw scheduling.
- GPU/occlusion culling or native handle exposure.
- Broad generated 3D production readiness.
