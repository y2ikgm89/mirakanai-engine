# Animation CPU Morph Tangent Deltas v1 Implementation Plan (2026-05-05)

**Plan ID:** `animation-cpu-morph-tangent-deltas-v1`
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 6
**Status:** Completed on 2026-05-05. This slice documents CPU-only glTF 2.0-style **additive TANGENT (xyz)** morph targets alongside existing POSITION and NORMAL morph paths in `mirakana_animation`.

## Goal

Expose deterministic CPU morph evaluation for **tangent deltas** per vertex: same linear blend model as positions (`bind + sum weight * delta`), then **unit-length normalize** morphed tangents with a **bind-tangent fallback** when the morphed vector degenerates, matching the existing normal morph policy.

## Context

- `AnimationMorphMeshCpuDesc` / `AnimationMorphTargetCpuDesc` already carried POSITION and NORMAL additive streams with validation diagnostics.
- glTF 2.0 allows morph targets to affect TANGENT accessors; engine work stays CPU-only and renderer-agnostic.
- `root-motion-ik-and-morph-foundation-v1` remains an umbrella id; animated morph weights, glTF morph cook/import, and GPU morph paths stay out of scope here.

## Constraints

- Keep changes inside `mirakana_animation` (`morph.hpp`, `morph.cpp`, `CMakeLists.txt` sources) and `mirakana_core` tests only for this slice.
- No new third-party dependencies, no asset-format import in this slice, no RHI/renderer/runtime host/editor coupling.
- **Breaking change allowed:** `bind_tangents` is required iff any target has non-empty `tangent_deltas`; unused `bind_tangents` rows remain invalid (symmetric with bind normals).
- Public API stays in namespace `mirakana::`; run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` when touching public headers.

## Done When

- [x] `AnimationMorphTargetCpuDesc` carries `tangent_deltas`; `AnimationMorphMeshCpuDesc` carries optional `bind_tangents` with the same cardinality gate as normals.
- [x] `validate_animation_morph_mesh_cpu_desc` reports `missing_bind_tangents_for_tangent_morph`, `bind_tangents_without_tangent_morph`, `invalid_bind_tangent`, and `invalid_tangent_delta` as appropriate.
- [x] `apply_animation_morph_targets_tangents_cpu` throws when the desc is invalid or has no tangent morph deltas.
- [x] `mirakana_core_tests` cover tangent morph math, diagnostics, and throw paths.
- [x] `docs/current-capabilities.md`, `docs/roadmap.md`, `engine/agent/manifest.json` (`mirakana_animation` purpose, `currentAnimation`, `recommendedNextPlan.reason` where relevant), `tools/check-ai-integration.ps1`, and this master plan reference POSITION+NORMAL+**TANGENT** CPU morph.
- [x] Focused CTest / `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` or equivalent evidence recorded below.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| `cmake --build out/build/dev --parallel 1 --target RUN_TESTS` | PASS | Full dev CTest lane: **29/29** tests passed including `mirakana_core_tests` (Windows MSVC; `--parallel 1` avoids PDB `C1041` contention). |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok` (includes manifest needle checks for `apply_animation_morph_targets_tangents_cpu`). |

## Non-Goals

- Animated morph weight curves, morph channel import from glTF into cook artifacts, GPU morph, renderer tangent-space consumption changes, quaternion/full 3D joint orientation, IK, loop accumulation, or broad morph production readiness claims.

## Next-Step Decision

Keep `aiOperableProductionLoop.currentActivePlan` on the production-completion master plan until the next narrow Phase 6 child is filed in `manifest.json`. Prefer the next dated slice from the umbrella: loop accumulation, IK, morph weight curves + glTF morph cook/import, or `3d-camera-controller-and-gameplay-template-v1`.
