# Animation CPU Skinned Normal Stream v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the existing `mirakana_animation` CPU skinning contract so it can deterministically skin an optional bind-normal stream alongside bind positions without adding renderer/RHI execution or broad skeletal-animation readiness.

**Architecture:** Keep the slice inside `mirakana_animation` and `mirakana_math`. Reuse the existing `AnimationCpuSkinningDesc` validation and palette-weight accumulation path, add optional normal-stream validation, and transform normals with the palette matrix direction part rather than point translation. This is a small 3D-playable prerequisite only; generated 3D production readiness remains blocked.

**Tech Stack:** C++23, `mirakana_animation`, `mirakana_math`, existing `mirakana_core_tests`, docs/manifest synchronization, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

---

## Next-Production-Gap Selection

- Latest completed slice: `docs/superpowers/plans/2026-05-02-generated-3d-mesh-telemetry-scaffold-v1.md`.
- That slice records `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` PASS with 16/16 CTest entries passed and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS with 28/28 CTest entries passed.
- Host gates remain explicit: Apple/iOS/Metal requires macOS/Xcode/metal/metallib, strict clang-tidy remains diagnostic-only without a compile database for the active Visual Studio generator, and broad Vulkan claims stay strict host/runtime/toolchain-gated.
- Manifest/registry still block `recommendedNextPlan` as `next-production-gap-selection`; broad gaps are `3d-playable-vertical-slice`, `editor-productization`, and `production-ui-importer-platform-adapters`.
- Selected focused slice: from `3d-playable-vertical-slice`, implement only CPU skinned normal stream output in the already dependency-free `mirakana_animation` CPU skinning path. This is Windows-verifiable through normal CTest and does not claim skeletal animation production readiness, GPU skinning/upload, renderer execution, glTF skin import, or generated visible 3D game readiness.

## Goal

Make `skin_animation_vertices_cpu` preserve deterministic vertex order while optionally emitting skinned normals for callers that supply bind normals.

## Context

- `Animation CPU Skinning Foundation v0` already validates bind positions, skin influence rows, and palette matrices, then emits skinned positions.
- Its explicit follow-up list includes normal/tangent deformation. Normals are the smallest useful next step because cooked lit 3D meshes already carry normal data, while GPU skinning/upload and renderer integration remain separate work.
- `mirakana_math` currently exposes `transform_point`; this slice needs a direction transform that ignores translation.

## Constraints

- Keep `engine/core` independent from OS, GPU, asset formats, editor, SDL3, Dear ImGui, and native handles.
- Do not add dependencies or touch renderer/RHI, runtime scene upload, platform, editor, SDL3, Dear ImGui, shader/material graph, importer, cooked mesh schema, or generated-game scaffold behavior.
- Do not claim GPU skinning/upload, skeletal animation production readiness, glTF animation/skin import, tangent or morph deformation, renderer execution, package streaming, Metal readiness, public native/RHI handles, editor productization, production UI/importer/platform adapters, or broad generated 3D production readiness.
- Public API changes must stay in namespace `mirakana::`, follow `docs/cpp-style.md`, and run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- [x] A RED test is added before implementation and fails because `AnimationCpuSkinningDesc` has no bind-normal stream and `AnimationCpuSkinnedVertexStream` has no normal output.
- [x] `mirakana::transform_direction` exists for applying a `Mat4` to vectors without translation.
- [x] `AnimationCpuSkinningDesc` accepts optional `bind_normals`; empty means positions-only behavior remains valid.
- [x] Validation rejects non-empty normal streams whose count does not match `bind_positions`, and rejects non-finite bind normals with deterministic diagnostics.
- [x] `skin_animation_vertices_cpu` emits normalized skinned normals when bind normals are supplied, preserves deterministic vertex order, and keeps existing position output unchanged.
- [x] Focused tests cover weighted normal blending, translation ignoring, replay determinism, normal count mismatch, invalid normals, and invalid-input throwing.
- [x] `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, and `engine/agent/manifest.json` describe this as CPU skinned-normal stream support only.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` has PASS evidence or a concrete environment blocker recorded here.
- [x] Registry and manifest are re-read after completion to select the next focused slice or stop on host-gated/broad-only work.

## Implementation Tasks

- [x] Add RED assertions to `tests/unit/core_tests.cpp` near the existing animation CPU skinning tests:
  - construct a two-joint palette that translates one joint and rotates another,
  - set `bind_normals` on `AnimationCpuSkinningDesc`,
  - assert skinned normals are emitted, normalized, deterministic, and unaffected by translation,
  - assert invalid normal count and non-finite normal diagnostics,
  - assert invalid normal input throws before deformation.
- [x] Run a focused `mirakana_core_tests` build/test and record the expected RED failure.
- [x] Add `transform_direction` and a small normalization helper in the math/animation implementation path.
- [x] Add `bind_normals` to `AnimationCpuSkinningDesc`, `normals` to `AnimationCpuSkinnedVertexStream`, and `normal_count_mismatch` / `invalid_bind_normal` diagnostics.
- [x] Update validation and no-alloc validity checks to accept empty normal streams and reject malformed non-empty streams.
- [x] Update `skin_animation_vertices_cpu` to emit skinned normals only when bind normals are present.
- [x] Run focused tests, then update docs/registry/manifest.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record final validation evidence and next-step decision in this plan.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| `cmake --build --preset dev --target mirakana_core_tests` after RED test | Blocked before compile | Direct CMake/MSBuild from the ordinary shell hit the known Windows environment duplicate-key issue: `The item "Path" already exists under the key "PATH"`. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests` after RED test | RED | Failed to compile as expected because `AnimationCpuSkinningDesc::bind_normals`, `AnimationCpuSkinnedVertexStream::normals`, `normal_count_mismatch`, and `invalid_bind_normal` did not exist yet. |
| Focused `mirakana_core_tests` after implementation | PASS | Wrapper-normalized `cmake --build --preset dev --target mirakana_core_tests` passed, then wrapper-normalized `ctest --preset dev --output-on-failure -R "mirakana_core_tests"` reported `100% tests passed, 0 tests failed out of 1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Initial run caught an invalid `validationRecipeMap` entry for this API-only slice; after removing the non-production recipe map entry, `json-contract-check: ok`. |
| Touched-file clang-format dry-run | PASS | `clang-format --dry-run --Werror` passed for `engine/animation/include/mirakana/animation/skin.hpp`, `engine/animation/src/skin.cpp`, `engine/math/include/mirakana/math/mat4.hpp`, and `tests/unit/core_tests.cpp`. Repository-wide `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` still reports pre-existing unrelated files outside this slice. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; 28/28 CTest entries passed. Diagnostic-only gates remain explicit: Metal shader tools missing on this Windows host, Apple packaging blocked by no macOS/Xcode, and strict clang-tidy blocked by missing `compile_commands.json` for the active Visual Studio generator. |

## Next-Step Decision

After completion, `docs/superpowers/plans/README.md` and `engine/agent/manifest.json` were re-read. The manifest still routes through blocked `next-production-gap-selection`. A further Windows-verifiable focused animation prerequisite remains cuttable from `3d-playable-vertical-slice`: optional CPU skinned tangent stream output. It is narrower than broad skeletal animation readiness and remains separate from morph deformation, GPU skinning/upload, renderer/RHI integration, glTF import, generated 3D production readiness, Metal, editor productization, and production UI/importer/platform adapters.

## Non-Goals

- Tangent, bitangent, morph-target, IK, root-motion, animation graph, glTF animation/skin import, or authored animation asset workflow support.
- GPU skinning/upload, renderer/RHI integration, native backend resource ownership changes, or public native/RHI handle exposure.
- Cooked mesh schema changes, runtime package mutation, broad package cooking, package streaming, or runtime source parsing.
- Generated 3D production readiness, renderer execution, renderer quality, material/shader graphs, live shader generation, Metal readiness, editor productization, or production UI/importer/platform adapters.
