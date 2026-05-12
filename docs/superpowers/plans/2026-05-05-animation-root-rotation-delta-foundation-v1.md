# Animation Root Rotation Delta Foundation v1 Implementation Plan (2026-05-05)

**Plan ID:** `animation-root-rotation-delta-foundation-v1`
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 6
**Status:** Completed on 2026-05-05. This slice is limited to deterministic `mirakana_animation` root-joint Z-rotation delta sampling over the existing root-motion contract.

## Goal

Extend the existing root-motion sampling contract so gameplay and later animation-graph work can query the local root joint `rotation_z_radians` start, end, and delta between two animation times without depending on renderer, runtime host, platform, editor, glTF, or asset package code.

## Context

- `Animation Root Motion Foundation v1` already exposes deterministic root-joint translation start/end/delta sampling over `AnimationSkeletonDesc` and `AnimationJointTrackDesc`.
- `JointLocalTransform` currently models orientation as Z-rotation-only radians, so the clean next step is a narrow delta for that existing representation rather than introducing a broader quaternion/full-3D orientation contract.
- `root-motion-ik-and-morph-foundation-v1` remains an umbrella backlog id. This child plan covers root rotation only; IK and broader morph production stay separate.

## Constraints

- Keep implementation inside `mirakana_animation` and `mirakana_math`.
- Do not add dependencies, asset-format coupling, renderer/RHI execution, runtime host wiring, platform APIs, editor APIs, SDL3, Dear ImGui, public native handles, or compatibility shims.
- Preserve the existing root-motion validation inputs and diagnostics. Invalid skeletons, invalid tracks, invalid times, and out-of-range root joints must keep deterministic behavior.
- Support the existing Z-rotation-only transform model. Do not claim full 3D joint orientation, quaternion interpolation, wrap-normalized angular deltas, loop accumulation, motion warping, IK, broader morph production, cooked animation assets, or broad skeletal animation production readiness.
- Public API changes must stay in namespace `mirakana::` and pass `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- [x] A RED test is added before implementation and fails because `AnimationRootMotionSample` does not expose root rotation fields.
- [x] `sample_animation_root_motion` returns root-joint `start_rotation_z_radians`, `end_rotation_z_radians`, and `delta_rotation_z_radians` using the same clamped interpolation/rest fallback semantics as `sample_animation_local_pose`.
- [x] Focused tests cover interpolated rotation deltas, clamped rotation deltas, rest fallback when the root has no rotation track, deterministic replay, and the existing invalid-input diagnostics still passing through the same validation path.
- [x] `docs/current-capabilities.md`, `docs/architecture.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, and `engine/agent/manifest.json` describe root motion as translation plus Z-rotation delta sampling, while explicitly excluding loop accumulation, IK, broader morph production, and broad skeletal animation production readiness.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or concrete local blockers are recorded here.

## Implementation Tasks

- [x] Add RED assertions to `tests/unit/core_tests.cpp` near the existing animation root-motion tests.
- [x] Run a focused `mirakana_core_tests` build/test and record the expected RED failure.
- [x] Add root rotation fields to `engine/animation/include/mirakana/animation/skeleton.hpp`.
- [x] Populate those fields in `engine/animation/src/skeleton.cpp` by composing the existing local-pose sampler.
- [x] Run focused `mirakana_core_tests`, then update docs/registry/manifest/static guidance.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record final validation evidence and next-slice state in this plan.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| Wrapper-normalized focused `mirakana_core_tests` after RED test | RED | `Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests` failed to compile because `AnimationRootMotionSample` did not expose `start_rotation_z_radians`, `end_rotation_z_radians`, or `delta_rotation_z_radians`. |
| Focused `mirakana_core_tests` after implementation | PASS | Wrapper-normalized `cmake --build --preset dev --target mirakana_core_tests` passed, then wrapper-normalized `ctest --preset dev --output-on-failure -R mirakana_core_tests` reported `100% tests passed, 0 tests failed out of 1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| First `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` after docs/manifest sync | Transient host failure | Static checks passed, but the full build hit MSVC `C1041` PDB contention on `sample_2d_playable_foundation`; the target rebuilt successfully by itself immediately afterward. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` rerun | PASS | `validate: ok`; default CTest reported 29/29 passed. Diagnostic-only blockers remained explicit for missing Metal `metal`/`metallib`, Apple packaging on this Windows host, Android release signing not configured, and no connected Android device smoke. |

## Next-Step Decision

`engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` returns to the production-completion master plan until the next dated Phase 6 child slice is filed. `recommendedNextPlan` remains the `root-motion-ik-and-morph-foundation-v1` umbrella, but the next implementation must still be split into one narrow host-verifiable plan such as loop accumulation, IK, broader morph production, cooked animation asset workflows, or a 3D camera/controller gameplay template.

## Non-Goals

- Full 3D joint orientation, quaternion APIs, angular wrap normalization, loop accumulation, motion warping, IK, broader morph production, animation graph authoring, cooked animation asset schema, glTF-specific API changes, renderer/RHI integration, GPU skinning changes, runtime host wiring, editor UI, package streaming, or broad skeletal animation production readiness.
