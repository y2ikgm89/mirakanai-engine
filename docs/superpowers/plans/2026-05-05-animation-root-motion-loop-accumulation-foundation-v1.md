# Animation Root Motion Loop Accumulation Foundation v1 Implementation Plan (2026-05-05)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `animation-root-motion-loop-accumulation-foundation-v1`
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 6
**Status:** Completed on 2026-05-05. This slice is limited to deterministic `mirakana_animation` root-motion delta accumulation over an explicit positive clip duration.

## Goal

Add a host-independent root-motion accumulation contract so gameplay and later animation-graph code can query the total local root-joint translation and Z-rotation delta across forward time intervals that may cross clip loop boundaries.

## Architecture

Keep the feature in `mirakana_animation` by composing the existing `sample_animation_root_motion` function. The new API accepts explicit `from_time_seconds`, `to_time_seconds`, and `clip_duration_seconds`, validates them deterministically, decomposes the forward interval into start partial, compressed full loops, and end partial segments, then sums translation and Z-rotation deltas without exposing renderer, runtime host, platform, editor, asset importer, or native handles.

## Tech Stack

C++23, `mirakana_animation`, `mirakana_math`, the existing lightweight unit test target `mirakana_core_tests`, and the repository validation wrappers.

## Context

- `Animation Root Motion Foundation v1` provides translation root-motion start/end/delta sampling between two times.
- `Animation Root Rotation Delta Foundation v1` extends that sample with Z-rotation start/end/delta over the current `JointLocalTransform` model.
- The production-completion master plan names loop accumulation as a narrow next candidate under the `root-motion-ik-and-morph-foundation-v1` umbrella.

## Constraints

- Keep implementation inside `mirakana_animation` and `mirakana_math`.
- Do not add dependencies, asset-format coupling, renderer/RHI execution, runtime host wiring, platform APIs, editor APIs, SDL3, Dear ImGui, public native handles, or compatibility shims.
- Require finite forward non-negative interval times and a finite positive clip duration. Backward accumulation, negative-time wrapping, and inferred clip duration are out of scope.
- Accumulate the current Z-rotation-only delta model. Do not claim full 3D joint orientation, quaternion interpolation, angular wrap normalization, motion warping, IK, broader morph production, cooked animation assets, or broad skeletal animation production readiness.
- Public API changes must stay in namespace `mirakana::` and pass `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- [x] A RED test is added before implementation and fails because loop-accumulation API types/functions do not exist.
- [x] The API validates skeleton, joint tracks, root joint, finite forward non-negative times, and finite positive clip duration with deterministic diagnostics.
- [x] The API accumulates same-cycle, partial-loop, exact-boundary multi-loop, and rest-fallback intervals by summing the existing translation and Z-rotation deltas.
- [x] Focused tests cover deterministic replay and invalid-input diagnostics.
- [x] `docs/current-capabilities.md`, `docs/architecture.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, the production-completion master plan, and `engine/agent/manifest.json` describe loop accumulation without broadening unsupported claims.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or concrete local blockers are recorded here.

## Implementation Tasks

- [x] Add RED tests to `tests/unit/core_tests.cpp` near the existing animation root-motion tests.
- [x] Run a focused `mirakana_core_tests` build/test and record the expected RED failure.
- [x] Add loop accumulation desc/result/diagnostic API declarations to `engine/animation/include/mirakana/animation/skeleton.hpp`.
- [x] Implement validation and accumulation in `engine/animation/src/skeleton.cpp` by composing `sample_animation_root_motion`.
- [x] Run focused `mirakana_core_tests`, then update docs/registry/manifest/static guidance.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record final validation evidence and next-slice state in this plan.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| Direct `cmake --build --preset dev --target mirakana_core_tests` after RED test | Host env blocker | Direct MSBuild launch failed before compiling `mirakana_core_tests` because the process environment contained both `Path` and `PATH`; repository `Invoke-CheckedCommand` normalized the environment for the actual RED proof. |
| Wrapper-normalized focused `mirakana_core_tests` after RED test | RED | `Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests` failed to compile because `mirakana::AnimationRootMotionAccumulationDesc`, `mirakana::accumulate_animation_root_motion`, `mirakana::validate_animation_root_motion_accumulation`, `mirakana::is_valid_animation_root_motion_accumulation`, and `AnimationRootMotionDiagnosticCode::invalid_clip_duration` did not exist yet. |
| Focused `mirakana_core_tests` after implementation | PASS | Wrapper-normalized `cmake --build --preset dev --target mirakana_core_tests` passed, then wrapper-normalized `ctest --preset dev --output-on-failure -R mirakana_core_tests` reported `100% tests passed, 0 tests failed out of 1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` | PASS | `format: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` | Timed out | Full-repository tidy generated the existing broad warning stream and timed out after 300 seconds. Targeted direct `clang-tidy --quiet -p out/build/dev engine/animation/src/skeleton.cpp` and `tests/unit/core_tests.cpp` both exited 0; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` also ran its configured one-file tidy smoke successfully. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; default CTest reported 29/29 passed. Diagnostic-only blockers remained explicit for missing Metal `metal`/`metallib`, Apple packaging on this Windows host, Android release signing not configured, and no connected Android device smoke. |

## Next-Step Decision

After this slice completes, `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` should return to the production-completion master plan until another dated Phase 6 child slice is filed. `recommendedNextPlan` should remain the `root-motion-ik-and-morph-foundation-v1` umbrella or move to another narrow host-verifiable child such as IK, broader morph production, cooked animation asset workflows, or `3d-camera-controller-and-gameplay-template-v1`.

## Non-Goals

- Full 3D joint orientation, quaternion APIs, angular wrap normalization, backward accumulation, negative-time wrapping, motion warping, IK, broader morph production, animation graph authoring, cooked animation asset schema, glTF-specific API changes, renderer/RHI integration, GPU skinning changes, runtime host wiring, editor UI, package streaming, or broad skeletal animation production readiness.
