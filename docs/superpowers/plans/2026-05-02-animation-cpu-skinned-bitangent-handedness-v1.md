# Animation CPU Skinned Bitangent Handedness v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the existing `mirakana_animation` CPU skinning contract so it can deterministically reconstruct optional skinned bitangents from skinned normals, skinned tangents, and per-vertex tangent handedness signs without adding renderer/RHI execution, glTF import, GPU skinning, morph deformation, or broad skeletal-animation readiness.

**Architecture:** Keep the slice inside `mirakana_animation` and `mirakana_math`. Reuse the already validated optional normal and tangent streams, add an optional `bind_tangent_handedness` sign stream, and reconstruct each bitangent as `normalize(cross(skinned_normal, skinned_tangent)) * sign` after CPU skinning. The handedness stream is a CPU data contract only; importer mapping, tangent-space material use, renderer upload, and bitangent storage in cooked mesh packages remain separate work.

**Tech Stack:** C++23, `mirakana_animation`, `mirakana_math`, existing `mirakana_core_tests`, docs/manifest synchronization, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

---

## Next-Production-Gap Selection

- Latest completed slice: `docs/superpowers/plans/2026-05-02-animation-cpu-skinned-tangent-stream-v1.md`.
- That slice records focused RED -> GREEN evidence, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` PASS, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` PASS, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` PASS, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS with 28/28 CTest entries passed.
- Host gates remain explicit: Apple/iOS/Metal requires macOS/Xcode/metal/metallib, strict clang-tidy remains diagnostic-only without a compile database for the active Visual Studio generator, and broad Vulkan claims stay strict host/runtime/toolchain-gated.
- Manifest/registry still block `recommendedNextPlan` as `next-production-gap-selection`.
- Selected focused slice: from the `3d-playable-vertical-slice` animation prerequisites, implement only CPU bitangent reconstruction from already-skinned normal/tangent streams plus explicit handedness signs. This is Windows-verifiable through normal CTest and does not claim morph deformation, skeletal animation production readiness, GPU skinning/upload, renderer execution, glTF skin import, tangent-space material rendering, cooked mesh schema changes, or generated visible 3D game readiness.

## Goal

Make `skin_animation_vertices_cpu` optionally emit deterministic skinned bitangents when callers provide bind normals, bind tangents, and bind tangent handedness signs.

## Context

- `Animation CPU Skinned Normal Stream v1` added optional skinned normals.
- `Animation CPU Skinned Tangent Stream v1` added optional skinned `Vec3` tangents but deliberately left tangent handedness and bitangent reconstruction unsupported.
- Bitangent reconstruction can be cut smaller than morph deformation, GPU skinning/upload, renderer integration, or glTF import because it depends only on the existing CPU skinned normal/tangent outputs and `mirakana_math::cross`.

## Constraints

- Keep `engine/core` independent from OS, GPU, asset formats, editor, SDL3, Dear ImGui, and native handles.
- Do not add dependencies or touch renderer/RHI, runtime scene upload, platform, editor, SDL3, Dear ImGui, shader/material graph, importer, cooked mesh schema, generated-game scaffold behavior, or package scripts.
- Do not claim morph deformation, GPU skinning/upload, skeletal animation production readiness, glTF animation/skin import, renderer execution, tangent-space material rendering, package streaming, Metal readiness, public native/RHI handles, editor productization, production UI/importer/platform adapters, or broad generated 3D production readiness.
- Public API changes must stay in namespace `mirakana::`, follow `docs/cpp-style.md`, and run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- [x] A RED test is added before implementation and fails because `AnimationCpuSkinningDesc` has no tangent-handedness stream, `AnimationCpuSkinnedVertexStream` has no bitangent output, and the new diagnostics do not exist yet.
- [x] `AnimationCpuSkinningDesc` accepts optional `bind_tangent_handedness`; empty means existing positions-only, positions-plus-normals, and positions-plus-normals-plus-tangents behavior remains valid and emits no bitangents.
- [x] Validation rejects non-empty handedness streams when normal or tangent streams are missing, when handedness count does not match `bind_positions`, and when handedness values are not finite `-1` or `+1` signs.
- [x] `skin_animation_vertices_cpu` emits normalized skinned bitangents only when handedness is supplied with valid normal and tangent streams, preserves deterministic vertex order, and keeps existing position/normal/tangent output unchanged.
- [x] Focused tests cover positive and negative handedness, weighted normal/tangent blending, translation ignoring, replay determinism, missing prerequisite streams, count mismatch, invalid handedness values, invalid-input throwing, and existing tangent-only behavior.
- [x] `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, `engine/agent/manifest.json`, and relevant static guidance describe this as CPU skinned-bitangent reconstruction support only.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` has PASS evidence or a concrete environment blocker recorded here.
- [x] Registry and manifest are re-read after completion to select the next focused slice or stop on host-gated/broad-only work.

## Implementation Tasks

- [x] Add RED assertions to `tests/unit/core_tests.cpp` near the existing animation CPU skinning tests:
  - construct a palette that translates one joint and rotates another,
  - set `bind_normals`, `bind_tangents`, and `bind_tangent_handedness` on `AnimationCpuSkinningDesc`,
  - assert skinned bitangents are emitted, normalized, deterministic, sign-correct, and unaffected by translation,
  - assert tangent-only input still emits no bitangents,
  - assert missing normal/tangent prerequisites, handedness count mismatch, and invalid handedness signs produce deterministic diagnostics,
  - assert invalid handedness input throws before deformation.
- [x] Run a focused `mirakana_core_tests` build/test and record the expected RED failure.
- [x] Add `bind_tangent_handedness` to `AnimationCpuSkinningDesc`, `bitangents` to `AnimationCpuSkinnedVertexStream`, and deterministic diagnostic codes for missing handedness prerequisites, handedness count mismatch, and invalid handedness.
- [x] Update validation and no-alloc validity checks to accept an empty handedness stream and reject malformed non-empty streams.
- [x] Update `skin_animation_vertices_cpu` to compute bitangents only when handedness is present, using final skinned normal/tangent rows and the canonical handedness sign.
- [x] Run focused tests, then update docs/registry/manifest/static guidance.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record final validation evidence and next-step decision in this plan.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| Focused `mirakana_core_tests` after RED test | RED | Wrapper-normalized `cmake --build --preset dev --target mirakana_core_tests` failed to compile as expected because `AnimationCpuSkinningDesc::bind_tangent_handedness`, `AnimationCpuSkinnedVertexStream::bitangents`, `tangent_handedness_missing_streams`, `tangent_handedness_count_mismatch`, and `invalid_tangent_handedness` did not exist yet. |
| Focused `mirakana_core_tests` after implementation | PASS | Wrapper-normalized `cmake --build --preset dev --target mirakana_core_tests` passed, then wrapper-normalized `ctest --preset dev --output-on-failure -R "mirakana_core_tests"` reported `100% tests passed, 0 tests failed out of 1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; 28/28 CTest entries passed. Diagnostic-only gates remain explicit: Metal shader/library tools missing on this Windows host, Apple packaging blocked by no macOS/Xcode, Android release signing/device smoke not configured/connected, and strict clang-tidy blocked by missing `compile_commands.json` for the active Visual Studio generator. |

## Next-Step Decision

After completion, `docs/superpowers/plans/README.md`, `docs/current-capabilities.md`, `docs/roadmap.md`, and `engine/agent/manifest.json` were re-read. The manifest still routes through blocked `next-production-gap-selection`; no further Windows-host-verifiable focused production slice is selected now. Remaining work is either broad and needs a new dated focused plan before implementation (`morph deformation`, `GPU skinning/upload`, `glTF animation/skin import`, editor productization, production UI/importer/platform adapters, broader generated 3D production readiness) or host/toolchain-gated (`Apple/iOS/Metal` on macOS/Xcode/metal/metallib, strict clang-tidy enforcement with a generator that emits `compile_commands.json`).

## Non-Goals

- Tangent-space material rendering, cooked mesh schema changes, glTF tangent import, glTF animation/skin import, authored animation asset workflow support, IK, root motion, animation graph, or morph-target deformation.
- GPU skinning/upload, renderer/RHI integration, native backend resource ownership changes, or public native/RHI handle exposure.
- Runtime package mutation, broad package cooking, package streaming, or runtime source parsing.
- Generated 3D production readiness, renderer execution, renderer quality, material/shader graphs, live shader generation, Metal readiness, editor productization, or production UI/importer/platform adapters.
