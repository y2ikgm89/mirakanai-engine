# Animation Root Motion Foundation v1 Implementation Plan (2026-05-04)

**Plan ID:** `animation-root-motion-foundation-v1`
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 6
**Status:** Completed on 2026-05-04. This slice is limited to deterministic `mirakana_animation` root-joint translation delta sampling.

## Goal

Add a dependency-free root-motion sampling contract to `mirakana_animation` so gameplay or later animation graph work can query the local root joint translation delta between two animation times without depending on renderer, runtime host, platform, editor, glTF, or asset package code.

## Context

- `Animation Skeleton Pose Foundation v0` already provides `AnimationSkeletonDesc`, `AnimationJointTrackDesc`, validated local-pose sampling, clamped keyframe interpolation, and rest-pose fallback for missing tracks.
- `gltf-animation-skin-import-v1` can import translation joint tracks into these engine types, but no engine API currently exposes root motion extraction.
- `gpu-skinning-upload-and-rendering-v1` is complete for the reviewed D3D12/package proof, but broad skeletal animation production readiness remains a separate claim.

## Constraints

- Keep implementation inside `mirakana_animation` and `mirakana_math`.
- Do not add dependencies, asset-format coupling, renderer/RHI execution, runtime host wiring, platform APIs, editor APIs, SDL3, Dear ImGui, or public native handles.
- Support translation delta only. Do not claim full 3D joint orientation, root rotation, accumulation across loops, IK, morph deformation, animation graph authoring, cooked animation assets, or broad skeletal animation production readiness.
- Reuse existing skeleton and joint-track validation. Invalid skeletons, invalid tracks, invalid times, and out-of-range root joints must produce deterministic diagnostics and throw before sampling when using the throwing sample API.
- Public API changes must stay in namespace `mirakana::` and pass `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- [x] A RED test is added before implementation and fails because the root-motion API and diagnostics do not exist yet.
- [x] `mirakana_animation` exposes a small root-motion descriptor/result API for sampling local root-joint translation at `from_time_seconds` and `to_time_seconds`.
- [x] Validation reports deterministic diagnostics for invalid skeletons/tracks, non-finite times, and out-of-range root joints.
- [x] Sampling returns start translation, end translation, and delta using the same clamped interpolation/rest fallback semantics as `sample_animation_local_pose`.
- [x] Focused tests cover interpolated deltas, clamped deltas, rest fallback when the root has no translation track, deterministic replay, invalid root index, invalid tracks, and invalid times.
- [x] `docs/current-capabilities.md`, `docs/architecture.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, and `engine/agent/manifest.json` describe this as translation-only root motion support.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or concrete local blockers are recorded here.

## Implementation Tasks

- [x] Add RED assertions to `tests/unit/core_tests.cpp` near the existing animation skeleton tests.
- [x] Run a focused `mirakana_core_tests` build/test and record the expected RED failure.
- [x] Add root-motion descriptor/result/diagnostic types and validation declarations to `engine/animation/include/mirakana/animation/skeleton.hpp`.
- [x] Implement validation and sampling in `engine/animation/src/skeleton.cpp` by composing the existing local-pose sampler.
- [x] Run focused `mirakana_core_tests`, then update docs/registry/manifest/static guidance.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record final validation evidence and the next focused-slice decision in this plan.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| Direct focused `mirakana_core_tests` after RED test | Blocked before compile | `cmake --build --preset dev --target mirakana_core_tests` hit the known direct MSBuild `Path`/`PATH` duplicate environment failure before compiling the new test. |
| Wrapper-normalized focused `mirakana_core_tests` after RED test | RED | `Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests` failed to compile because `AnimationRootMotionSampleDesc`, `AnimationRootMotionDiagnosticCode`, `validate_animation_root_motion_sample`, `is_valid_animation_root_motion_sample`, and `sample_animation_root_motion` did not exist yet. |
| Focused `mirakana_core_tests` after implementation | PASS | Wrapper-normalized `cmake --build --preset dev --target mirakana_core_tests` passed, then wrapper-normalized `ctest --preset dev --output-on-failure -R mirakana_core_tests` reported `100% tests passed, 0 tests failed out of 1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| Full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` | Timed out | Full tidy reached existing broad warnings and timed out after 180 seconds; no new root-motion-specific failure was identified before timeout. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1` | PASS | `tidy-check: ok (1 files)` using the generated CMake File API compile database. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; CTest reported 29/29 passed. Diagnostic-only blockers remained explicit for Metal tools and Apple packaging on this Windows host. |

## Next-Step Decision

`engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` returns to the production-completion master plan until the next dated Phase 6 child slice is filed. `recommendedNextPlan` remains the `root-motion-ik-and-morph-foundation-v1` umbrella, but the next implementation should still be split into one narrow host-verifiable plan, such as IK, morph deformation, root rotation/loop accumulation, cooked animation asset workflows, or animation graph authoring.

## Non-Goals

- Root rotation extraction, loop accumulation, motion warping, IK, morph deformation, animation graph authoring, cooked animation asset schema, glTF-specific API changes, renderer/RHI integration, GPU skinning changes, runtime host wiring, editor UI, package streaming, or broad skeletal animation production readiness.
